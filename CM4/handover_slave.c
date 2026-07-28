
// handover_slave.c — Quetzal-2 Portenta H7 (CM4) side of the A3200<->Portenta
// I2C handover prototype. Happy-path only: every command that the A3200's
// handover.c actually sends gets a correct, immediate response; the bench
// stub's failure-injection test modes are not ported (see handover_slave.h).

#include "handover_slave.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "debug.h"
#include "i2c.h"
#include "main.h"
#include <handover_protocol.h>

/* NOTE (unconfirmed hardware mapping): building against I2C1 — see the
 * header-comment in handover_slave.h. Swap every hi2c1/I2C1 reference below
 * (and in stm32h7xx_it.c's I2C1_EV_IRQHandler/I2C1_ER_IRQHandler) for
 * hi2c3/I2C3 if bench continuity testing shows the physical SDA/SCL pins
 * actually route to I2C3 instead. */
static I2C_HandleTypeDef *ho_i2c = &hi2c1;

/* rx_buf[0] = command byte, rx_buf[1..4] = up to HO_PROTO_MAX_ARG_LEN arg
 * bytes. All volatile: written from ISR context, read from the handler task. */
static volatile uint8_t ho_rx_buf[1U + HO_PROTO_MAX_ARG_LEN];
static volatile uint8_t ho_rx_have_len =
    0; /* bytes placed into ho_rx_buf so far   */

/* Prepared response bytes — most commands only use tx_buf[0], but
 * HO_PROTO_CMD_RELAY_POLL responds with all HO_PROTO_MAX_RESP_LEN bytes.
 * How many bytes actually get clocked out is decided by ho_tx_resp_len()
 * at the read-phase AddrCallback, based on which command was received. */
static volatile uint8_t ho_tx_buf[HO_PROTO_MAX_RESP_LEN];

static volatile HO_Slave_State ho_state = HO_SLAVE_STATE_IDLE;
static volatile uint8_t ho_active_subsystems = HO_PROTO_SUBSYS_NONE;

/* "Want to transmit LoRa" flag — see QTZ_HandoverSlave_SetLoraPending()'s
 * doc comment. No real LoRa/SPI logic exists on this prototype yet; this
 * flag and the heartbeat-response substitution below are the full extent
 * of what's implemented for this pass. */
static volatile bool ho_lora_tx_pending = false;

/* Single-slot queued relay command — see
 * QTZ_HandoverSlave_QueueRelayCommand()'s doc comment. No real ADCS/MILO
 * decision logic exists yet on this prototype; this is just the queue +
 * HO_PROTO_CMD_RELAY_POLL wiring. */
static volatile bool ho_relay_queued = false;
static volatile uint8_t ho_relay_subsys = 0;
static volatile uint8_t ho_relay_cmd_id = 0;
static volatile uint8_t ho_relay_param0 = 0;
static volatile uint8_t ho_relay_param1 = 0;

static SemaphoreHandle_t ho_rx_sem = NULL;

/* Number of argument bytes expected after the command byte, per command.
 * Anything not listed here (including HO_PROTO_CMD_EPS_DATA, which the
 * A3200 never actually sends) is treated as 0 extra bytes — the handler
 * task's default case then responds NACK for genuinely unknown commands. */
static uint8_t ho_cmd_arg_len(uint8_t cmd) {
  switch (cmd) {
  case HO_PROTO_CMD_PREPARE:
  case HO_PROTO_CMD_START:
  case HO_PROTO_CMD_ABORT:
  case HO_PROTO_CMD_LORA_GRANT:
    return 1U;
  case HO_PROTO_CMD_PAYLOAD_RELAY:
    return 3U;
  case HO_PROTO_CMD_RELAY_RESULT:
    return 4U;
  case HO_PROTO_CMD_PING:
  case HO_PROTO_CMD_STATUS_REQ:
  case HO_PROTO_CMD_HEARTBEAT_REQ:
  case HO_PROTO_CMD_RELAY_POLL:
  default:
    return 0U;
  }
}

/* Number of response bytes to transmit for a given command — every command
 * responds with 1 byte except HO_PROTO_CMD_RELAY_POLL's fixed 5-byte
 * [status][subsys][cmd_id][param0][param1] response. */
static uint8_t ho_tx_resp_len(uint8_t cmd) {
  switch (cmd) {
  case HO_PROTO_CMD_RELAY_POLL:
    return HO_PROTO_MAX_RESP_LEN;
  default:
    return 1U;
  }
}

/* ── HAL I2C slave ISR callbacks ──────────────────────────────────────────
 * ISR does the minimum possible work (frame the next HAL transfer, or hand
 * off to the task); all command decoding and state-machine logic lives in
 * HandoverSlave_Routine() below. This mirrors the proven bench stub's
 * on_receive/on_request split, adapted to STM32 HAL sequential slave mode. */

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection,
                          uint16_t AddrMatchCode) {
  (void)AddrMatchCode;
  if (hi2c != ho_i2c) {
    return;
  }

  if (TransferDirection == I2C_DIRECTION_TRANSMIT) {
    /* A3200 is writing to us — start of a new command. We don't know the
     * arg length yet, so grab just the command byte first. */
    ho_rx_have_len = 0;
    HAL_I2C_Slave_Seq_Receive_IT(ho_i2c, (uint8_t *)&ho_rx_buf[0], 1U,
                                 I2C_FIRST_FRAME);
  } else {
    /* A3200 is reading from us — the response phase of the same repeated-
     * START transaction. ho_tx_buf must already hold a valid response by
     * this point (the handler task computed it after the write phase's
     * SlaveRxCpltCallback gave the semaphore and was scheduled in time —
     * see the timing note in handover_slave.h / the project plan).
     * Response length depends on which command this is (ho_rx_buf[0] is
     * still valid here — nothing overwrites it until the NEXT write phase). */
    uint8_t tx_len = ho_tx_resp_len(ho_rx_buf[0]);
    HAL_I2C_Slave_Seq_Transmit_IT(ho_i2c, (uint8_t *)ho_tx_buf, tx_len,
                                  I2C_FIRST_AND_LAST_FRAME);
  }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  BaseType_t higher_prio_task_woken = pdFALSE;

  if (hi2c != ho_i2c) {
    return;
  }

  if (ho_rx_have_len == 0) {
    /* Just received the command byte — now that we know which command it
     * is, arm reception of the exact number of argument bytes it takes. */
    ho_rx_have_len = 1U;
    uint8_t arg_len = ho_cmd_arg_len(ho_rx_buf[0]);
    if (arg_len > 0U) {
      HAL_I2C_Slave_Seq_Receive_IT(ho_i2c, (uint8_t *)&ho_rx_buf[1], arg_len,
                                   I2C_NEXT_FRAME);
      return;
    }
    /* No args expected (e.g. PING) — the command is already complete. */
  } else {
    /* Just received the argument bytes for the command already in hand. */
    ho_rx_have_len = (uint8_t)(ho_rx_have_len + ho_cmd_arg_len(ho_rx_buf[0]));
  }

  if (ho_rx_sem != NULL) {
    xSemaphoreGiveFromISR(ho_rx_sem, &higher_prio_task_woken);
    portYIELD_FROM_ISR(higher_prio_task_woken);
  }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  (void)hi2c;
  /* Response already sent — nothing else to do. HAL_I2C_ListenCpltCallback
   * fires once the master issues STOP, and re-arms listening for next time. */
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c != ho_i2c) {
    return;
  }
  /* Whole transaction (write phase + read phase, ended by STOP) is done —
   * re-arm for the next one. */
  HAL_I2C_EnableListen_IT(ho_i2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c != ho_i2c) {
    return;
  }
  /* Defensive recovery: without this, one malformed/aborted transaction
   * (e.g. A3200 reading before we finished preparing ho_tx_buf[0]) could wedge
   * the slave until a hard reset. Reset our receive bookkeeping and re-arm
   * listening so the bus is always ready for the next transaction. */
  QTZ_Debug_Warning("[HO] I2C error 0x%lx - resetting listen state\n",
                    (unsigned long)HAL_I2C_GetError(hi2c));
  ho_rx_have_len = 0;
  HAL_I2C_EnableListen_IT(ho_i2c);
}

/* ── Public init ──────────────────────────────────────────────────────── */

void QTZ_HandoverSlave_Init(void) {
  ho_rx_sem = xSemaphoreCreateBinary();

  /* Priority chosen to satisfy configMAX_SYSCALL_INTERRUPT_PRIORITY (=5 on
   * this target, see FreeRTOSConfig.h) — calling FreeRTOS APIs
   * (xSemaphoreGiveFromISR) from an ISR at a numerically-lower (higher
   * urgency) priority than this would silently corrupt kernel state. */
  HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
  HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

  HAL_I2C_EnableListen_IT(ho_i2c);

  QTZ_Debug_Log("[HO] Portenta handover slave ready at 0x%02X (I2C1)\n",
                HO_PROTO_PORTENTA_ADDR);
}

/**
 * Set/clear the "want to transmit LoRa" flag. No real LoRa/SPI control
 * exists on this prototype yet — this is the hook a future real
 * implementation (or a manual test trigger) would call when it actually
 * has an image/data ready to send via PLD2. While set and ho_state ==
 * ACTIVE, the next HEARTBEAT_REQ response substitutes
 * HO_PROTO_RESP_HEARTBEAT_LORA_PENDING for the plain heartbeat, which is
 * what actually asks A3200 for a grant/deny decision — see
 * HO_PROTO_CMD_LORA_GRANT's doc comment in handover_protocol.h.
 */
void QTZ_HandoverSlave_SetLoraPending(bool pending) {
  ho_lora_tx_pending = pending;
}

/**
 * Queue an ADCS/PLD1 subsystem command for A3200 to pick up on its next
 * HO_PROTO_CMD_RELAY_POLL (issued every ~1s while handover is active — see
 * handover.c's ho_poll_relay_queue()). No real ADCS/MILO decision logic
 * exists yet on this prototype; this is the hook a future implementation
 * (or a manual test trigger) would call.
 *
 * Single-slot queue: calling this again before A3200 has polled the first
 * command overwrites it (first-pass limitation, matches the doc comment on
 * HO_PROTO_CMD_RELAY_POLL in handover_protocol.h).
 */
void QTZ_HandoverSlave_QueueRelayCommand(uint8_t subsys, uint8_t cmd_id,
                                         uint8_t param0, uint8_t param1) {
  ho_relay_subsys = subsys;
  ho_relay_cmd_id = cmd_id;
  ho_relay_param0 = param0;
  ho_relay_param1 = param1;
  ho_relay_queued = true; /* set last so a concurrent RELAY_POLL read sees
                           * fully-populated fields once it observes true */
}

/* ── Handler task ─────────────────────────────────────────────────────── */

void HandoverSlave_Routine(void *argument) {
  (void)argument;

  for (;;) {
    if (xSemaphoreTake(ho_rx_sem, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    if (ho_rx_have_len == 0) {
      continue; /* spurious wake guard */
    }

    uint8_t cmd = ho_rx_buf[0];
    uint8_t subcmd = (ho_rx_have_len > 1U) ? ho_rx_buf[1] : 0U;
    uint8_t arg0 = (ho_rx_have_len > 2U) ? ho_rx_buf[2] : 0U;
    uint8_t arg1 = (ho_rx_have_len > 3U) ? ho_rx_buf[3] : 0U;
    uint8_t arg2 = (ho_rx_have_len > 4U) ? ho_rx_buf[4] : 0U;

    switch (cmd) {
    case HO_PROTO_CMD_PING:
      ho_tx_buf[0] = HO_PROTO_RESP_ACK;
      QTZ_Debug_Log("[HO] PING -> ACK\n");
      break;

    case HO_PROTO_CMD_STATUS_REQ:
      switch (ho_state) {
      case HO_SLAVE_STATE_PREPARING:
        ho_tx_buf[0] = HO_PROTO_RESP_READY;
        break;
      case HO_SLAVE_STATE_ACTIVE:
        ho_tx_buf[0] = HO_PROTO_RESP_ACK;
        break;
      case HO_SLAVE_STATE_IDLE:
      default:
        ho_tx_buf[0] = HO_PROTO_RESP_NACK;
        break;
      }
      QTZ_Debug_Log("[HO] STATUS_REQ -> 0x%02X\n", ho_tx_buf[0]);
      break;

    case HO_PROTO_CMD_PREPARE:
      if (ho_state != HO_SLAVE_STATE_IDLE) {
        ho_tx_buf[0] = HO_PROTO_RESP_BUSY;
        QTZ_Debug_Warning("[HO] PREPARE rejected - not IDLE (state=%d)\n",
                          (int)ho_state);
      } else {
        ho_active_subsystems = subcmd; /* PREPARE's single arg byte */
        ho_state = HO_SLAVE_STATE_PREPARING;
        ho_tx_buf[0] = HO_PROTO_RESP_ACK;
        QTZ_Debug_Log("[HO] PREPARE subsys=0x%02X -> ACK\n", subcmd);
      }
      break;

    case HO_PROTO_CMD_START:
      if (ho_state != HO_SLAVE_STATE_PREPARING) {
        ho_tx_buf[0] = HO_PROTO_RESP_NACK;
        QTZ_Debug_Warning("[HO] START rejected - not PREPARING (state=%d)\n",
                          (int)ho_state);
      } else {
        ho_active_subsystems = subcmd; /* START's single arg byte */
        ho_state = HO_SLAVE_STATE_ACTIVE;
        ho_tx_buf[0] = HO_PROTO_RESP_ACK;
        QTZ_Debug_Log("[HO] START subsys=0x%02X -> ACK ** HANDOVER ACTIVE **\n",
                      subcmd);
      }
      break;

    case HO_PROTO_CMD_ABORT:
      ho_active_subsystems = HO_PROTO_SUBSYS_NONE;
      ho_state = HO_SLAVE_STATE_IDLE;
      ho_tx_buf[0] = HO_PROTO_RESP_ACK;
      QTZ_Debug_Log("[HO] ABORT reason=0x%02X -> IDLE\n", subcmd);
      break;

    case HO_PROTO_CMD_HEARTBEAT_REQ:
      if (ho_lora_tx_pending && ho_state == HO_SLAVE_STATE_ACTIVE) {
        ho_tx_buf[0] = HO_PROTO_RESP_HEARTBEAT_LORA_PENDING;
        QTZ_Debug_Log("[HO] HEARTBEAT_REQ -> HEARTBEAT_LORA_PENDING\n");
      } else {
        ho_tx_buf[0] = HO_PROTO_RESP_HEARTBEAT;
        QTZ_Debug_Log("[HO] HEARTBEAT_REQ -> HEARTBEAT\n");
      }
      break;

    case HO_PROTO_CMD_LORA_GRANT:
      /* ACK + log only for this prototype pass — no real LoRa/SPI TX
       * control exists yet (blocked on the unresolved SPI-arbitration /
       * SAMD21-vs-direct-SPI topology question for PLD2). On APPROVED,
       * clear the pending flag (simulating "done" since there's nothing
       * real to transmit); on DENIED, leave it set so the next heartbeat
       * naturally asks again. */
      ho_tx_buf[0] = HO_PROTO_RESP_ACK;
      if (subcmd == HO_PROTO_RESP_LORA_APPROVED) {
        ho_lora_tx_pending = false;
        QTZ_Debug_Log("[HO] LORA_GRANT -> APPROVED -> ACK (would transmit)\n");
      } else if (subcmd == HO_PROTO_RESP_LORA_DENIED) {
        QTZ_Debug_Log("[HO] LORA_GRANT -> DENIED -> ACK (will retry)\n");
      } else {
        QTZ_Debug_Warning("[HO] LORA_GRANT unexpected arg=0x%02X -> ACK\n",
                          subcmd);
      }
      break;

    case HO_PROTO_CMD_PAYLOAD_RELAY:
      /* ACK + log only for this prototype pass — no real RS485/MILO
       * forwarding yet. Will be wired to QTZ_OBC_SendCommand() once the
       * ground command system is active, same deferral pattern as ADM's
       * prototype-to-flight transition. */
      ho_tx_buf[0] = HO_PROTO_RESP_ACK;
      QTZ_Debug_Log("[HO] PAYLOAD_RELAY subcmd=0x%02X arg0=%u arg1=%u -> ACK "
                    "(would forward)\n",
                    subcmd, arg0, arg1);
      break;

    case HO_PROTO_CMD_RELAY_POLL:
      /* 5-byte response: ho_tx_resp_len() at the read-phase AddrCallback
       * already knows this command needs all 5 bytes, not just tx_buf[0]. */
      if (ho_relay_queued) {
        ho_tx_buf[0] = HO_PROTO_RESP_RELAY_PENDING;
        ho_tx_buf[1] = ho_relay_subsys;
        ho_tx_buf[2] = ho_relay_cmd_id;
        ho_tx_buf[3] = ho_relay_param0;
        ho_tx_buf[4] = ho_relay_param1;
        ho_relay_queued = false; /* consumed — single-slot queue */
        QTZ_Debug_Log("[HO] RELAY_POLL -> PENDING subsys=0x%02X cmd_id=0x%02X "
                      "param0=%u param1=%u\n",
                      ho_tx_buf[1], ho_tx_buf[2], ho_tx_buf[3], ho_tx_buf[4]);
      } else {
        ho_tx_buf[0] = HO_PROTO_RESP_NACK;
        ho_tx_buf[1] = ho_tx_buf[2] = ho_tx_buf[3] = ho_tx_buf[4] = 0U;
      }
      break;

    case HO_PROTO_CMD_RELAY_RESULT:
      /* ACK + log only — this prototype has no real ADCS/MILO command
       * logic to hand the result to yet, since QTZ_HandoverSlave_
       * QueueRelayCommand() has no real caller either (see its doc
       * comment). arg naming here: subcmd=subsys, arg0=cmd_id,
       * arg1=result0, arg2=result1 (RELAY_RESULT's 4 arg bytes). */
      ho_tx_buf[0] = HO_PROTO_RESP_ACK;
      QTZ_Debug_Log(
          "[HO] RELAY_RESULT subsys=0x%02X cmd_id=0x%02X r0=%u r1=%u -> ACK\n",
          subcmd, arg0, arg1, arg2);
      break;

    default:
      ho_tx_buf[0] = HO_PROTO_RESP_NACK;
      QTZ_Debug_Warning("[HO] UNKNOWN cmd=0x%02X -> NACK\n", cmd);
      break;
    }

    ho_rx_have_len = 0;
  }
}
