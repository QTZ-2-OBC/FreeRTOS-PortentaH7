#ifndef __rs485_framed_H
#define __rs485_framed_H
#include <common.h>
#include <rs485_framed_protocol.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @file rs485_framed.h
 * @brief Frame-level (CRC16 + ACK + retry) layer on top of the raw byte
 *        transport in rs485.c/rs485.h — H7 (CM4) side.
 *
 * Ports the A3200's rs485_link.c wire protocol to this side, so that
 * whichever SAMD21 board is wired to H7's own RS485 pair (see
 * RS485_dual_bus_wiring_reference.md) sees the exact same framing as one
 * wired to A3200's pair. Unlike rs485_link.c, this is a BLOCKING
 * implementation, not a scheduler-driven state machine — H7 has no single
 * shared cooperative scheduler the way A3200 does; each FreeRTOS task
 * (ADCS_Routine, MILO_Routine, ...) already blocks on its own RS485 calls,
 * so a blocking send-and-wait-for-ACK / wait-for-frame pair is the natural
 * fit here, not a regression from what obc.c already did.
 *
 * NOT yet bench-validated against a real framed-protocol SAMD21 peer — no
 * such firmware exists yet (same caveat as the A3200 side's rs485_link.c).
 */

typedef enum {
  QTZ_RS485F_OK,
  QTZ_RS485F_TX_ERROR,       /* underlying QTZ_RS485_Send failed */
  QTZ_RS485F_NO_ACK,         /* sent frame, no matching ACK within timeout/retries */
  QTZ_RS485F_TIMEOUT,        /* waited for an inbound frame, none arrived in time */
  QTZ_RS485F_CRC_ERROR,      /* a frame arrived but failed CRC (after retries) */
  QTZ_RS485F_BUFFER_TOO_SMALL,
} QTZ_RS485F_Result;

/**
 * Build a frame from `payload`/`len`, send it, and block until its ACK
 * arrives (or all retries are exhausted). Manages its own sequence counter
 * internally (one per QTZ_RS485F_SendAcked() call site's UART — see .c file
 * — since each physical bus only has one framed-link user on the H7 side
 * today, this is a single global counter, mirroring rs485_link.c's
 * single-counter design).
 */
QTZ_RS485F_Result QTZ_RS485F_SendAcked(const uint8_t *payload, uint8_t len,
                                        uint32_t ack_timeout_ms,
                                        uint8_t max_retries);

/**
 * Block waiting for the next inbound DATA frame (not an ACK frame) within
 * `timeout_ms`. On QTZ_RS485F_OK, `out_payload`/`*out_len` holds the
 * received payload and this function has already sent that frame's ACK
 * back on the wire — mirrors rs485_link.c's poll_rx() auto-ACK-on-DATA
 * behavior, so the sender doesn't need a separate acknowledgement path.
 */
QTZ_RS485F_Result QTZ_RS485F_RecvFrame(uint8_t *out_payload, uint8_t *out_len,
                                        uint8_t max_payload,
                                        uint32_t timeout_ms);

/**
 * Last-known health of this RS485 bus — was the most recent
 * QTZ_RS485F_SendAcked()/QTZ_RS485F_RecvFrame() call successful?
 *
 * Purely passive/observational: reflects whatever RS485 traffic already
 * happened (e.g. ADCS_Routine's periodic ping) — calling this does NOT
 * itself generate any RS485 traffic. Defaults to true at boot (no
 * transaction attempted yet) so a not-yet-exercised bus doesn't read as
 * "already failed."
 *
 * ISR-safe: a plain volatile-bool read, safe to call from
 * handover_slave.c's ho_process_command_isr().
 */
bool QTZ_RS485F_IsHealthy(void);

#endif
