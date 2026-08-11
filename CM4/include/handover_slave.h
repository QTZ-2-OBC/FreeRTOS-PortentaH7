#ifndef __handover_slave_H
#define __handover_slave_H

#include <stdbool.h>
#include <stdint.h>

/**
 * I2C-slave handover prototype — Portenta H7 (CM4) side.
 *
 * Implements the Portenta side of the A3200<->Portenta handover protocol
 * (see Common/include/handover_protocol.h) over STM32 HAL interrupt-driven
 * I2C slave mode on I2C1 (PB6=SCL, PB7=SDA — see CM4/i2c.c). This is a
 * happy-path-only prototype: PAYLOAD_RELAY and RELAY_RESULT are ACK'd and
 * logged but not actually acted upon yet (deferred until the ground command
 * system / real ADCS-MILO decision logic is active), and the bench stub's
 * failure-injection test modes (SLOW / MISS_HB / ERROR / SILENT) are not
 * ported — only NORMAL-mode behavior.
 *
 * Also implements the LoRa-arbitration heartbeat-pending mechanism
 * (QTZ_HandoverSlave_SetLoraPending()) and the ADCS/PLD1 relay-poll queue
 * (QTZ_HandoverSlave_QueueRelayCommand()) — both are hooks for future real
 * logic to call; neither has a real caller on this prototype yet.
 *
 * NOTE (unconfirmed hardware mapping): this targets I2C1, on the assumption
 * that Arduino's default `Wire` (what the reference bench stub used) maps to
 * I2C1 on this Portenta variant. Verify by continuity test before bench
 * wiring; if the physical SDA/SCL pins actually route to I2C3 instead, swap
 * every hi2c1/I2C1 reference in handover_slave.c and the new IRQ handlers in
 * stm32h7xx_it.c for hi2c3/I2C3.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Portenta-internal handover state. Mirrors (but is not numerically
 *  identical to) the A3200's 5-state machine — the Portenta only ever needs
 *  to track these 3; ABORTING/ERROR are A3200-only states. */
typedef enum {
  HO_SLAVE_STATE_IDLE = 0,
  HO_SLAVE_STATE_PREPARING,
  HO_SLAVE_STATE_ACTIVE,
} HO_Slave_State;

/**
 * Initialise the I2C1 slave role and spawn the handler task's prerequisites.
 *
 * Call once from MX_FREERTOS_Init(), AFTER MX_I2C1_Init() has run (so hi2c1
 * is already configured with OwnAddress1 = HO_PROTO_PORTENTA_ADDR) and
 * BEFORE spawning the HandoverSlave_Routine task. Creates the RX-ready
 * binary semaphore, sets NVIC priority for the I2C1 EV/ER IRQs, and arms
 * HAL_I2C_EnableListen_IT() so the first transaction from A3200 is caught.
 */
void QTZ_HandoverSlave_Init(void);

/**
 * Set/clear the "want to transmit LoRa" flag (see handover_slave.c's doc
 * comment for the full mechanism). No real LoRa/SPI logic exists yet — this
 * is the hook a future implementation or manual test trigger would call.
 */
void QTZ_HandoverSlave_SetLoraPending(bool pending);

/**
 * Queue an ADCS/PLD1 subsystem command for A3200 to pick up on its next
 * HO_PROTO_CMD_RELAY_POLL (see handover_slave.c's doc comment). No real
 * ADCS/MILO decision logic exists yet — this is the hook a future
 * implementation or manual test trigger would call. Single-slot queue: a
 * second call before the first is polled overwrites it.
 *
 * @param subsys  HO_PROTO_SUBSYS_ADCS or HO_PROTO_SUBSYS_PAYLOAD.
 * @param cmd_id  Opaque, subsystem-specific command code.
 * @param param0  First command parameter.
 * @param param1  Second command parameter.
 */
void QTZ_HandoverSlave_QueueRelayCommand(uint8_t subsys, uint8_t cmd_id,
                                        uint8_t param0, uint8_t param1);

/**
 * FreeRTOS task entry point — spawn via osThreadNew in freertos.c.
 *
 * Blocks on the RX-ready semaphore, decodes each received command, updates
 * the handover state machine, and prepares the single response byte that
 * the I2C ISR sends back during the read phase of the same transaction.
 */
void HandoverSlave_Routine(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __handover_slave_H */
