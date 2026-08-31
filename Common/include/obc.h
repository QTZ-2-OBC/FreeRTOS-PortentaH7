#ifndef QTZ_LIB_OBC
#define QTZ_LIB_OBC
#include "common.h"

// All req/resp follow the pattern:
// [protocol_id][status][subsys][cmd_id][param0][param1]
//
// Each 1 byte long.
#define QTZ_OBC_CMD_LEN 6

#define QTZ_OBC_I2C_TX_LEN QTZ_OBC_CMD_LEN * 3
#define QTZ_OBC_I2C_RX_LEN QTZ_OBC_CMD_LEN

#define QTZ_OBC_UART_TX_LEN QTZ_OBC_CMD_LEN * 10
#define QTZ_OBC_UART_RX_LEN QTZ_OBC_CMD_LEN * 10

// Max iterations allowed for the OBC before aborting I2C and UART transmittion.
#define QTZ_OBC_WATCHDOG_LIMIT 1000U

typedef struct {
  QTZ_ByteArray rx;
  QTZ_ByteArray tx;
} QTZ_OBC_SerialInterface;

typedef enum {
  QTZ_OBC_STATE_IDLE = 0,
  QTZ_OBC_STATE_ERROR,
  QTZ_OBC_STATE_WAITING_I2C,
  QTZ_OBC_STATE_WAITING_UART,
} QTZ_OBC_State;

typedef struct {
  QTZ_OBC_SerialInterface i2c;
  QTZ_OBC_SerialInterface uart_rs485;

  volatile QTZ_OBC_State state;
  volatile uint32_t watchdog_ticks;
} QTZ_OBC_Ctx;

static QTZ_OBC_Ctx GLOBAL_CTX;

#endif
