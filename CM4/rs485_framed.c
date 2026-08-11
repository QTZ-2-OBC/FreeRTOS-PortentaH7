#include "cmsis_os2.h"
#include "debug.h"
#include <rs485.h>
#include <rs485_framed.h>
#include <string.h>

/* ── CRC-16/CCITT (poly 0x1021, init 0xFFFF) — ported verbatim from the
 * A3200 side's rs485_link.c so both ends compute an identical checksum. ── */

static uint16_t crc16_update(uint16_t crc, uint8_t byte) {
  crc ^= (uint16_t)byte << 8;
  for (int i = 0; i < 8; i++) {
    crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                          : (uint16_t)(crc << 1);
  }
  return crc;
}

static uint16_t crc16_buf(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFFU;
  for (size_t i = 0; i < len; i++) {
    crc = crc16_update(crc, buf[i]);
  }
  return crc;
}

/* ── Frame builder — ported verbatim from rs485_link.c ── */

static size_t frame_build(uint8_t *out, uint8_t seq, const uint8_t *payload,
                          uint8_t len) {
  size_t i = 0;
  out[i++] = QTZ_RS485F_FRAME_START;
  out[i++] = seq;
  out[i++] = len;
  memcpy(&out[i], payload, len);
  i += len;
  uint16_t crc = crc16_buf(&out[1], 2u + len);
  out[i++] = (uint8_t)(crc >> 8);
  out[i++] = (uint8_t)(crc & 0xFFU);
  out[i++] = QTZ_RS485F_FRAME_END;
  return i;
}

static size_t frame_build_ack(uint8_t *out, uint8_t seq) {
  uint8_t ack_payload = QTZ_RS485F_ACK_TYPE;
  return frame_build(out, seq, &ack_payload, 1u);
}

/* Single global TX sequence counter — this H7 build only ever has one
 * framed-link user per physical bus at a time (ADCS_Routine / MILO_Routine
 * don't run concurrently against the same wire), mirroring rs485_link.c's
 * own single-counter design. Revisit if a second concurrent framed sender
 * is ever added on this bus. */
static uint8_t s_tx_seq = 0;

/* Passive health tracker, consumed by handover_slave.c's HEARTBEAT_REQ
 * handling via QTZ_RS485F_IsHealthy() — see that function's doc comment in
 * rs485_framed.h. Defaults to true: no transaction attempted yet at boot
 * shouldn't read as "already failed." Updated at the end of every
 * QTZ_RS485F_SendAcked()/QTZ_RS485F_RecvFrame() call, success or failure. */
static volatile bool s_rs485_last_ok = true;

/* ── Byte-level receive helper — wraps QTZ_RS485_ReceiveRaw() (NOT
 * QTZ_RS485_Receive()) in a fresh QTZ_ByteArray each time, since each call
 * here reads a distinct, independent chunk of a frame. Deliberately the
 * "Raw" variant: read_frame() below calls QTZ_RS485_BeginReception() itself
 * exactly once per frame before making any of these calls — see that
 * function's comment and rs485.h's QTZ_RS485_ReceiveRaw() doc comment for
 * why (QTZ_RS485_Receive() pays a ~100ms settle delay on every call, which
 * would multiply per chunk — or per byte, in the START-search loop — and
 * blow through short ACK-wait budgets before a single byte is read). ── */

static QTZ_RECEIVERS485_Result recv_bytes(uint8_t *dst, uint16_t count,
                                          uint32_t timeout_ms) {
  QTZ_ByteArray arr;
  QTZ_ByteArray_Init(&arr, dst, count);
  QTZ_ByteArray_Reset(&arr);
  return QTZ_RS485_ReceiveRaw(&arr, count, timeout_ms);
}

static uint32_t ms_remaining(uint32_t deadline) {
  uint32_t now = HAL_GetTick();
  if ((int32_t)(deadline - now) <= 0) {
    return 0;
  }
  return deadline - now;
}

typedef struct {
  uint8_t seq;
  uint8_t payload[QTZ_RS485F_MAX_PAYLOAD];
  uint8_t len;
  int is_ack;
} rs485f_frame_t;

/* Reads exactly one full frame (searching for START first, discarding
 * anything else) within timeout_ms. This is the shared core used by both
 * QTZ_RS485F_SendAcked() (waiting for an ACK) and QTZ_RS485F_RecvFrame()
 * (waiting for a DATA frame) — mirrors rs485_link.c's rx_feed(), but as a
 * blocking read over several HAL_UART_Receive calls instead of a
 * byte-at-a-time non-blocking state machine, since this side has no single
 * shared cooperative scheduler tick to hang a state machine off of. */
static QTZ_RS485F_Result read_frame(rs485f_frame_t *out, uint32_t timeout_ms) {
  uint32_t deadline = HAL_GetTick() + timeout_ms;

  /* Enter RX mode exactly once for this whole frame read (RE-pin settle,
   * ~100ms) — every sub-read below uses QTZ_RS485_ReceiveRaw(), which
   * skips that settle delay. See recv_bytes()'s comment. */
  __HAL_UART_CLEAR_NEFLAG(QTZ_RS485_UART_HANDLE);
  __HAL_UART_CLEAR_OREFLAG(QTZ_RS485_UART_HANDLE);
  QTZ_RS485_BeginReception();

  /* 1. Search for START. */
  uint8_t byte;
  for (;;) {
    uint32_t remaining = ms_remaining(deadline);
    if (remaining == 0) {
      return QTZ_RS485F_TIMEOUT;
    }
    if (QTZ_RECEIVERS485_OK != recv_bytes(&byte, 1, remaining)) {
      continue; /* re-check the deadline on the next loop iteration */
    }
    if (byte == QTZ_RS485F_FRAME_START) {
      break;
    }
  }

  /* 2. SEQ + LEN. */
  uint8_t hdr[2];
  {
    uint32_t remaining = ms_remaining(deadline);
    if (remaining == 0 || QTZ_RECEIVERS485_OK != recv_bytes(hdr, 2, remaining)) {
      return QTZ_RS485F_TIMEOUT;
    }
  }
  uint8_t seq = hdr[0];
  uint8_t plen = hdr[1];
  if (plen > QTZ_RS485F_MAX_PAYLOAD) {
    /* Can't trust a LEN this large — either noise or a foreign sender. */
    QTZ_Debug_Warning("[RS485F] LEN=%u exceeds max payload, dropping\n", plen);
    return QTZ_RS485F_CRC_ERROR;
  }

  /* 3. PAYLOAD + CRC(2) + END(1). */
  uint8_t tail[QTZ_RS485F_MAX_PAYLOAD + 3];
  {
    uint32_t remaining = ms_remaining(deadline);
    uint16_t tail_len = (uint16_t)plen + 3u;
    if (remaining == 0 ||
        QTZ_RECEIVERS485_OK != recv_bytes(tail, tail_len, remaining)) {
      return QTZ_RS485F_TIMEOUT;
    }
  }

  if (tail[plen + 2] != QTZ_RS485F_FRAME_END) {
    QTZ_Debug_Warning("[RS485F] bad END byte, dropping frame\n");
    return QTZ_RS485F_CRC_ERROR;
  }

  uint16_t rx_crc = ((uint16_t)tail[plen] << 8) | (uint16_t)tail[plen + 1];
  uint8_t crc_input[2 + QTZ_RS485F_MAX_PAYLOAD];
  crc_input[0] = seq;
  crc_input[1] = plen;
  memcpy(&crc_input[2], tail, plen);
  uint16_t calc_crc = crc16_buf(crc_input, 2u + plen);

  if (rx_crc != calc_crc) {
    QTZ_Debug_Warning("[RS485F] CRC mismatch, dropping frame\n");
    return QTZ_RS485F_CRC_ERROR;
  }

  out->seq = seq;
  out->len = plen;
  if (plen > 0) {
    memcpy(out->payload, tail, plen);
  }
  out->is_ack = (plen == 1u && tail[0] == QTZ_RS485F_ACK_TYPE);
  return QTZ_RS485F_OK;
}

QTZ_RS485F_Result QTZ_RS485F_SendAcked(const uint8_t *payload, uint8_t len,
                                        uint32_t ack_timeout_ms,
                                        uint8_t max_retries) {
  if (len > QTZ_RS485F_MAX_PAYLOAD) {
    return QTZ_RS485F_BUFFER_TOO_SMALL;
  }

  uint8_t frame[QTZ_RS485F_MAX_FRAME_SIZE];
  uint8_t seq = s_tx_seq++;
  size_t frame_len = frame_build(frame, seq, payload, len);

  for (uint8_t attempt = 0; attempt <= max_retries; attempt++) {
    if (attempt > 0) {
      osDelay((uint32_t)attempt * QTZ_RS485F_RETRY_BACKOFF_MS);
      QTZ_Debug_Warning("[RS485F] retry %u/%u for seq=%u\n", attempt,
                        max_retries, seq);
    }

    QTZ_ByteArray tx_arr;
    QTZ_ByteArray_InitWithLength(&tx_arr, frame, frame_len, frame_len);
    /* 100 ms is a generous raw-TX timeout for a <=135-byte frame at
     * 115200 baud (~12ms worst case) — this bounds only the HAL_UART_
     * Transmit() call itself, not the ACK wait below. */
    QTZ_SENDRS485_Result send_result = QTZ_RS485_Send(&tx_arr, 100);
    if (QTZ_SENDRS485_OK != send_result) {
      QTZ_Debug_Warning("[RS485F] TX error %d on attempt %u\n", send_result,
                        attempt);
      continue;
    }

    rs485f_frame_t rx;
    QTZ_RS485F_Result rx_result = read_frame(&rx, ack_timeout_ms);
    if (QTZ_RS485F_OK == rx_result && rx.is_ack && rx.seq == seq) {
      s_rs485_last_ok = true;
      return QTZ_RS485F_OK;
    }
    /* Timeout, CRC error, a stray non-ACK frame, or an ACK for a different
     * seq (e.g. a leftover from a prior exchange) — retry. */
  }

  s_rs485_last_ok = false;
  return QTZ_RS485F_NO_ACK;
}

QTZ_RS485F_Result QTZ_RS485F_RecvFrame(uint8_t *out_payload, uint8_t *out_len,
                                        uint8_t max_payload,
                                        uint32_t timeout_ms) {
  uint32_t deadline = HAL_GetTick() + timeout_ms;

  for (;;) {
    uint32_t remaining = ms_remaining(deadline);
    if (remaining == 0) {
      s_rs485_last_ok = false;
      return QTZ_RS485F_TIMEOUT;
    }

    rs485f_frame_t rx;
    QTZ_RS485F_Result result = read_frame(&rx, remaining);
    if (QTZ_RS485F_TIMEOUT == result) {
      s_rs485_last_ok = false;
      return QTZ_RS485F_TIMEOUT;
    }
    if (QTZ_RS485F_OK != result) {
      continue; /* CRC/framing error — keep listening within budget */
    }
    if (rx.is_ack) {
      continue; /* stray ACK, not what we're waiting for */
    }
    if (rx.len > max_payload) {
      /* A valid frame DID arrive (RS485 is working) — it just doesn't fit
       * the caller's buffer, a caller/protocol mismatch, not a link issue. */
      s_rs485_last_ok = true;
      return QTZ_RS485F_BUFFER_TOO_SMALL;
    }

    /* Auto-ACK the DATA frame — mirrors rs485_link.c's poll_rx()
     * PARSE_OK_DATA behavior, so the sender doesn't need a separate
     * acknowledgement path. */
    uint8_t ack_frame[8];
    size_t ack_len = frame_build_ack(ack_frame, rx.seq);
    QTZ_ByteArray ack_arr;
    QTZ_ByteArray_InitWithLength(&ack_arr, ack_frame, ack_len, ack_len);
    QTZ_RS485_Send(&ack_arr, 100);

    memcpy(out_payload, rx.payload, rx.len);
    *out_len = rx.len;
    s_rs485_last_ok = true;
    return QTZ_RS485F_OK;
  }
}

bool QTZ_RS485F_IsHealthy(void) { return s_rs485_last_ok; }
