#ifndef QTZ_HANDOVER_PROTOCOL_H
#define QTZ_HANDOVER_PROTOCOL_H

/**
 * Shared wire protocol for the A3200 <-> Portenta H7 handover I2C link.
 *
 * This is the Portenta-side counterpart of the A3200 SDK's
 * `handover_protocol.h` / `handover.h` (HANDOVER_* constants). The two files
 * are maintained independently and must be kept numerically in sync by hand
 * — see the A3200 header's own note about this. Every constant below was
 * cross-checked byte-for-byte against the current A3200 `handover.h` and the
 * existing Arduino/Wire bench stub `portenta_slave.cpp`, which implements
 * this exact protocol.
 *
 * A3200 is always I2C master, Portenta is always I2C slave at
 * HO_PROTO_PORTENTA_ADDR. There is never unsolicited traffic from the
 * Portenta. Every command reads back exactly ONE response byte, EXCEPT
 * HO_PROTO_CMD_RELAY_POLL, which reads back HO_PROTO_MAX_RESP_LEN (5)
 * bytes (the A3200 side never requests the richer multi-byte status blob
 * from HO_PROTO_CMD_STATUS_REQ, which still responds with just 1 byte).
 */

#include <stdint.h>

/** 7-bit I2C address of this Portenta H7 (slave role). */
#define HO_PROTO_PORTENTA_ADDR      0x50U

/* ── Commands: A3200 (master) -> Portenta (slave) ────────────────────────── */
#define HO_PROTO_CMD_PING           0xA0U   /* no arg                              */
#define HO_PROTO_CMD_STATUS_REQ     0xA1U   /* no arg                              */
#define HO_PROTO_CMD_PREPARE        0xA2U   /* +1 arg: subsystem bitmask           */
#define HO_PROTO_CMD_START          0xA3U   /* +1 arg: subsystem bitmask           */
#define HO_PROTO_CMD_ABORT          0xA4U   /* +1 arg: reason code                 */
/* HO_PROTO_CMD_EPS_DATA (0xA5) intentionally NOT handled here — confirmed the
 * A3200 side (handover.c) never actually sends it (no call site exists).
 * Falls through to the unknown-command NACK path if ever received. */
#define HO_PROTO_CMD_HEARTBEAT_REQ  0xA6U   /* no arg                              */
/**
 * 0xA7 — repurposed as HO_PROTO_CMD_LORA_GRANT (A3200 -> Portenta only).
 *
 * Original 3-session design sketch had Portenta *sending* 0xA7
 * (LORA_REQUEST) to A3200 — physically impossible on this bus (A3200 is
 * always I2C master, Portenta never sends unsolicited traffic, per the file
 * header above). Real mechanism: this firmware signals "want to transmit
 * LoRa" by responding to a routine HO_PROTO_CMD_HEARTBEAT_REQ with
 * HO_PROTO_RESP_HEARTBEAT_LORA_PENDING (0xBA) instead of the plain
 * HO_PROTO_RESP_HEARTBEAT — still counts as a valid heartbeat on the A3200
 * side. A3200 then checks its own AX100 TX state and sends this command
 * with arg = HO_PROTO_RESP_LORA_APPROVED (0xB6) or _DENIED (0xB7).
 */
#define HO_PROTO_CMD_LORA_GRANT     0xA7U
/* 0xA8 LORA_COMPLETE — vestigial under the mechanism above (this firmware
 * just stops responding with HEARTBEAT_LORA_PENDING once done transmitting;
 * A3200 never polls for or expects an explicit completion signal). Defined
 * for documentation parity with the original spec, not sent or handled. */
#define HO_PROTO_CMD_LORA_COMPLETE  0xA8U
/* 0xA9 AX100_STATUS — reserved, not used (A3200 checks AX100 itself). */
#define HO_PROTO_CMD_PAYLOAD_RELAY  0xAAU   /* +3 arg: [subcmd][arg0][arg1]        */
/**
 * 0xAB — HO_PROTO_CMD_RELAY_POLL (A3200 -> Portenta, no arg).
 *
 * Same direction fix as HO_PROTO_CMD_LORA_GRANT: the original spec's
 * ADCS_COMMAND/PLD1_COMMAND were Portenta-initiated, impossible here. A3200
 * polls every scheduler tick (1s) while active; this firmware responds with
 * a fixed 5 bytes: [status][subsys][cmd_id][param0][param1]. status =
 * HO_PROTO_RESP_NACK (nothing queued) or HO_PROTO_RESP_RELAY_PENDING (a
 * queued command follows, see QTZ_HandoverSlave_QueueRelayCommand()).
 * Single-slot queue — a second queue attempt before the first is polled
 * overwrites it (first-pass limitation).
 */
#define HO_PROTO_CMD_RELAY_POLL     0xABU
/** A3200 -> Portenta, +4 arg: [subsys][cmd_id][result0][result1] — delivers
 *  the SAMD21 RS485 response for a command this firmware previously queued
 *  via RELAY_POLL. Respond with a plain ACK. */
#define HO_PROTO_CMD_RELAY_RESULT   0xACU

/* ── Responses: Portenta (slave) -> A3200 (master) ───────────────────────── */
#define HO_PROTO_RESP_ACK           0xB0U
#define HO_PROTO_RESP_NACK          0xB1U
#define HO_PROTO_RESP_READY         0xB2U
#define HO_PROTO_RESP_BUSY          0xB3U
#define HO_PROTO_RESP_ERROR         0xB4U
#define HO_PROTO_RESP_HEARTBEAT     0xB5U
/** Also reused as the arg byte A3200 sends with HO_PROTO_CMD_LORA_GRANT. */
#define HO_PROTO_RESP_LORA_APPROVED 0xB6U
/** Also reused as the arg byte A3200 sends with HO_PROTO_CMD_LORA_GRANT. */
#define HO_PROTO_RESP_LORA_DENIED   0xB7U
/* 0xB8 AX100_IDLE, 0xB9 AX100_BUSY — reserved, not used (informational-only
 * opcodes from the original spec; A3200's own tx_count check makes these
 * unnecessary for now). */
/** Respond with this instead of HO_PROTO_RESP_HEARTBEAT to signal "want to
 *  transmit LoRa" — see HO_PROTO_CMD_LORA_GRANT above for the mechanism. */
#define HO_PROTO_RESP_HEARTBEAT_LORA_PENDING 0xBAU
/** HO_PROTO_CMD_RELAY_POLL response status byte: a queued command follows
 *  in the remaining 4 response bytes. */
#define HO_PROTO_RESP_RELAY_PENDING 0xBBU

/* ── Subsystem flags (bitmask, PREPARE/START arg) ────────────────────────── */
#define HO_PROTO_SUBSYS_NONE        0x00U
#define HO_PROTO_SUBSYS_ADCS        0x01U
#define HO_PROTO_SUBSYS_PAYLOAD     0x02U   /**< PLD1 / MILO camera.        */
#define HO_PROTO_SUBSYS_ADM_MON     0x04U
#define HO_PROTO_SUBSYS_PLD2        0x08U   /**< PLD2 / LoRa.               */
#define HO_PROTO_SUBSYS_ALL         0x0FU

/** Max bytes A3200 ever sends after the command byte, across all commands
 *  actually implemented here (HO_PROTO_CMD_RELAY_RESULT's 4 arg bytes
 *  [subsys][cmd_id][result0][result1] is the largest). */
#define HO_PROTO_MAX_ARG_LEN        4U

/** Max bytes Portenta ever sends back as a response, across all commands
 *  (HO_PROTO_CMD_RELAY_POLL's 5-byte [status][subsys][cmd_id][param0][param1]
 *  response is the largest — every other command responds with exactly 1
 *  byte). */
#define HO_PROTO_MAX_RESP_LEN       5U

#endif /* QTZ_HANDOVER_PROTOCOL_H */
