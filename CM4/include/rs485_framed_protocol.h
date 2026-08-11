#ifndef QTZ_LIB_RS485_FRAMED_PROTOCOL
#define QTZ_LIB_RS485_FRAMED_PROTOCOL

/**
 * @file rs485_framed_protocol.h
 * @brief Wire-format constants for the RS485 framed link, mirrored byte-for-
 *        byte from the A3200 side's `rs485_link.h`/`rs485_link.c`
 *        (Main OBC/102824 A3200 C&M SDK SW/7_0_0_SRC_CTRL/src_wip/).
 *
 * Two separate physical RS485 buses exist in this design (confirmed by the
 * team this session): A3200 <-> its own SAMD21 subsystems on one pair, H7
 * <-> its own SAMD21 subsystems (ADCS, MILO/PLD1) on a second, physically
 * separate pair — see `RS485_dual_bus_wiring_reference.md`. They do NOT
 * share a wire, so there is no bus-arbitration concern between A3200 and H7.
 * What they DO share, by decision, is this exact byte-level protocol, so
 * any SAMD21 board sees identical framing regardless of which OBC it talks
 * to, and firmware/tooling written for one bus works unmodified on the
 * other.
 *
 * Frame wire format (identical to rs485_link.h):
 *   [0xAA][SEQ:1][LEN:1][PAYLOAD:LEN][CRC16_HI:1][CRC16_LO:1][0x55]
 *
 * ACK frame (zero-length payload, type byte 0xAC):
 *   [0xAA][SEQ:1][0x01][0xAC][CRC16_HI:1][CRC16_LO:1][0x55]
 *
 * CRC16: CCITT, poly 0x1021, init 0xFFFF, MSB-first, no reflection, no
 * final XOR — computed over SEQ+LEN+PAYLOAD only (not the START/END bytes).
 *
 * If you change any constant here, change it in `rs485_link.h` too — a
 * mismatch between the two sides silently breaks CRC verification.
 */

#define QTZ_RS485F_FRAME_START      0xAAU
#define QTZ_RS485F_FRAME_END        0x55U
#define QTZ_RS485F_ACK_TYPE         0xACU
#define QTZ_RS485F_MAX_PAYLOAD      128U
#define QTZ_RS485F_MAX_FRAME_SIZE   (QTZ_RS485F_MAX_PAYLOAD + 7U)

/** Default ACK-wait timeout — deliberately NOT set to A3200's
 *  RS485_ACK_TIMEOUT_MS (50ms). Confirmed by reading CM4/rs485.c: entering
 *  RX mode (QTZ_RS485_BeginReception()) has a fixed ~100ms settle delay
 *  baked in, paid once per frame read on the H7 side (see rs485_framed.c's
 *  read_frame()) — a 50ms budget would time out before a single byte could
 *  ever be read. 200ms leaves headroom above that floor plus real
 *  transmission time; still NOT bench-confirmed against a real peer, but
 *  no longer a number that's mathematically guaranteed to fail. Every
 *  current call site (CM4/obc.c's command tables) passes its own
 *  generous send_timeout/recv_timeout (>=1000ms) instead of this default,
 *  so this constant only matters for a future caller that doesn't. */
#define QTZ_RS485F_ACK_TIMEOUT_MS   200U

/** Default max retries, matches A3200's RS485_MAX_RETRIES. */
#define QTZ_RS485F_MAX_RETRIES      3U

/** Default per-retry backoff, matches A3200's RS485_RETRY_BACKOFF_MS. */
#define QTZ_RS485F_RETRY_BACKOFF_MS 20U

#endif
