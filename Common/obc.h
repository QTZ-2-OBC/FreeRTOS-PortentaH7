#ifndef QTZ_LIB
#define QTZ_LIB

#include "adcs.h"
#include "milo.h"
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define QTZ_ByteArray_Create(buffer_name, inner_memory_name, capacity)         \
  uint8_t inner_memory_name[capacity];                                         \
  QTZ_ByteArray buffer_name;                                                   \
  QTZ_ByteArray_Init(&buffer_name, inner_memory_name, capacity);

#define QTZ_ByteArray_CreateOnlyBuffer(buffer_name, inner_memory_name,         \
                                       capacity)                               \
  QTZ_ByteArray buffer_name;                                                   \
  QTZ_ByteArray_Init(&buffer_name, inner_memory_name, capacity);

// Booleans for QTZ library
typedef enum { QTZ_BOOL_FALSE, QTZ_BOOL_TRUE } QTZ_Bool;

// Holds an array of bytes
typedef struct {
  // The number of bytes this array contains with information.
  size_t length;
  // The number of bytes the data can hold.
  size_t capacity;
  // The pointer to the first element of the array.
  uint8_t *data;
} QTZ_ByteArray;

// Initializes an array with the specified length, the capacity will equal the
// length.
void QTZ_ByteArray_Init(QTZ_ByteArray *self, uint8_t *data, size_t capacity);
// Initializes an array with the specified length and capacity.
void QTZ_ByteArray_InitWithLength(QTZ_ByteArray *self, uint8_t *data,
                                  size_t capacity, size_t length);

// Resets the length of of the array back to 0.
//
// This doesn't free any memory!
void QTZ_ByteArray_Reset(QTZ_ByteArray *self);

typedef enum {
  QTZ_BYTEARRAYGET_OK,
  QTZ_BYTEARRAYGET_INDEX_OUT_OF_BOUNDS,
} QTZ_BYTEARRAYGET_Result;

// Attempt to get a byte from the array.
QTZ_BYTEARRAYGET_Result QTZ_ByteArray_Get(QTZ_ByteArray *self, size_t index,
                                          uint8_t *out);

typedef enum {
  QTZ_BYTEARRAYSET_OK,
  QTZ_BYTEARRAYSET_INDEX_OUT_OF_BOUNDS,
} QTZ_BYTEARRAYSET_Result;
// Attempt to set a byte from the array.
QTZ_BYTEARRAYSET_Result QTZ_ByteArray_Set(QTZ_ByteArray *self, size_t index,
                                          uint8_t value);

typedef enum {
  QTZ_BYTEARRAYAPPEND_OK,
  QTZ_BYTEARRAYAPPEND_NOT_ENOUGH_SPACE,
} QTZ_BYTEARRAYAPPEND_Result;
// Attempts to append the provided value to the byte array.
QTZ_BYTEARRAYAPPEND_Result QTZ_ByteArray_Append(QTZ_ByteArray *self,
                                                uint8_t value);

typedef enum {
  QTZ_BYTEARRAYEXTEND_OK,
  QTZ_BYTEARRAYEXTEND_NOT_ENOUGH_SPACE,
} QTZ_BYTEARRAYEXTEND_Result;
// Attempts to append the provided value to the byte array.
QTZ_BYTEARRAYEXTEND_Result QTZ_ByteArray_Extend(QTZ_ByteArray *self,
                                                QTZ_ByteArray *other);

// Attempts to extend the byte array with a c style string.
//
// The null terminator value is not copied over to the byte array!
QTZ_BYTEARRAYEXTEND_Result QTZ_ByteArray_ExtendCStr(QTZ_ByteArray *self,
                                                    char *other);

// Returns the remaining quantity of free bytes on the array.
size_t QTZ_ByteArray_Remaining(QTZ_ByteArray *self);

// Returns the last free position of data contained in the array.
uint8_t *QTZ_ByteArray_Current(QTZ_ByteArray *self);

// Stop execution of the whole system.
//
// Marks a point in time where it is unsafe to continue the operation of the
// whole system beyond this point. This is known as an irrecoverable error.
//
// For example, a function may expect a pointer to never be null, if it is null
// then that means the bytes of the RAM have been altered and it's no longer
// safe to continue execution.
void QTZ_Panic();

// Panics if the provided expression is false.
#define QTZ_AssertAlways(p)                                                    \
  {                                                                            \
    if (!(p)) {                                                                \
      QTZ_Debug_Fatal("EXPR SHOULD ALWAYS BE TRUE");                           \
      QTZ_Panic();                                                             \
    }                                                                          \
  }

// Panics if the provided expression is true.
#define QTZ_AssertNever(p)                                                     \
  {                                                                            \
    if (p) {                                                                   \
      QTZ_Debug_Fatal("EXPR SHOULD NEVER BE TRUE");                            \
      QTZ_Panic();                                                             \
    }                                                                          \
  }

enum { QTZ_DEBUG_CAPACITY = 1024 };

// Platform specific implementation for initializing the resources for debug
// printing.
//
// Mainly used to setup QTZ_Debug_Print.
void QTZ_Debug_Init();

#ifdef QTZ_DEBUG
#define QTZ_Debug_Log(format, ...)                                             \
  QTZ_Debug_InnerLog("%s:%-4d [LOG] " format "\n", __FILE__, __LINE__,         \
                     ##__VA_ARGS__);
#define QTZ_Debug_Warning(format, ...)                                         \
  QTZ_Debug_InnerLog("%s:%-4d [WAR] " format "\n", __FILE__, __LINE__,         \
                     ##__VA_ARGS__);
#define QTZ_Debug_Error(format, ...)                                           \
  QTZ_Debug_InnerLog("%s:%-4d [ERR] " format "\n", __FILE__, __LINE__,         \
                     ##__VA_ARGS__);
#define QTZ_Debug_Fatal(format, ...)                                           \
  QTZ_Debug_InnerLog("%s:%-4d [FAT] " format "\n", __FILE__, __LINE__,         \
                     ##__VA_ARGS__);

void QTZ_Debug_InnerLog(const char *msg, ...);
#else
#define QTZ_Debug_Log(format, ...)
#define QTZ_Debug_Warning(format, ...)
#define QTZ_Debug_Error(format, ...)
#define QTZ_Debug_Fatal(format, ...)
#endif

// Internal function used to print the output!
//
// Implement according to the platform.
void QTZ_Debug_Print();

// Get the number of digits of n.
size_t QTZ_DigitQuantity(size_t n);

typedef enum {
  QTZ_FMTSIZET_OK,
  QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH,
} QTZ_FMTSIZET_Result;

// Convert n into a string containing n.
QTZ_FMTSIZET_Result QTZ_FmtSizeT(size_t n, QTZ_ByteArray *buffer);

// =======================================
// OBC
// =======================================

typedef enum {
  QTZ_OBC_RESULT_OK,
  QTZ_OBC_RESULT_ERROR,
  QTZ_OBC_RESULT_TIMEOUT,
} QTZ_OBC_OperationResult;

// All req/resp packets follow the pattern:
// [protocol_id] - [status] - [subsys] - [cmd_id] - [param0] - [param1]
//
// Each 1 byte long.
typedef struct {
  uint8_t protocol_id;
  uint8_t status;
  uint8_t subsys;
  uint8_t cmd_id;
  uint8_t param0;
  uint8_t param1;
} QTZ_OBC_Packet;
// Make sure the packet above only contains uint8_t.
// If it doesn't, then this length won't make sense!
#define QTZ_OBC_PACKET_LEN sizeof(QTZ_OBC_Packet)
_Static_assert(sizeof(QTZ_OBC_Packet) == 6,
               "The QTZ_OBC_Packet size has changed! Is this ok?");

#define QTZ_OBC_I2C_TX_LEN QTZ_OBC_PACKET_LEN
#define QTZ_OBC_I2C_RX_LEN QTZ_OBC_PACKET_LEN

#define QTZ_OBC_UART_TX_LEN QTZ_OBC_PACKET_LEN
#define QTZ_OBC_UART_RX_LEN QTZ_OBC_PACKET_LEN

typedef enum {
  QTZ_OBC_PROTOCOL_HANDOVER = 'H',
  QTZ_OBC_PROTOCOL_SUBSYSTEMS = 'S',
} QTZ_OBC_Protocol;

typedef enum {
  QTZ_OBC_SUBSYSTEM_GOMSPACE = 'G',
  QTZ_OBC_SUBSYSTEM_PORTENTA = 'P',
  QTZ_OBC_SUBSYSTEM_MILO = 'M',
  QTZ_OBC_SUBSYSTEM_ADCS = 'A',
} QTZ_OBC_Subsystem;

typedef enum {
  QTZ_OBC_MILO_IMAGE_CLASSIFICATION_LOW = 1,
  QTZ_OBC_MILO_IMAGE_CLASSIFICATION_MEDIUM,
  QTZ_OBC_MILO_IMAGE_CLASSIFICATION_HIGH,
  QTZ_OBC_MILO_IMAGE_CLASSIFICATION_NONE,
} QTZ_OBC_MiloImageClassification;

typedef enum {
  /*
   * ===========================
   * GOMSPACE COMMANDS
   * ===========================
   * */
  QTZ_OBC_COMMAND_GOMSPACE_PING = 0,
  QTZ_OBC_COMMAND_GOMSPACE_PING_ACK,
  QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER,
  QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER_ACK,
  QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT,
  QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT_ACK,
  QTZ_OBC_COMMAND_GOMSPACE_STATUS,
  QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK,

  QTZ_OBC_COMMAND_GOMSPACE_START_TASK,
  QTZ_OBC_COMMAND_GOMSPACE_START_TASK_ACK,
  QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI,
  QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI_ACK,

  QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER,
  QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER_ACK,

  /*
   * ===========================
   * MILO COMMANDS
   * ===========================
   * */
  QTZ_OBC_COMMAND_MILO_PING,
  QTZ_OBC_COMMAND_MILO_PING_ACK,
  QTZ_OBC_COMMAND_MILO_TAKE_PICTURE,
  QTZ_OBC_COMMAND_MILO_TAKE_PICTURE_ACK,
  QTZ_OBC_COMMAND_MILO_PICTURE_CLASI,
  QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK,

  /*
   * ===========================
   * GENERAL COMMANDS
   * ===========================
   */
  QTZ_OBC_COMMAND_TIMEOUT,
} QTZ_OBC_Command;

// Max iterations allowed for the OBC before aborting I2C and UART transmittion.
#define QTZ_OBC_WATCHDOG_LIMIT 1000U

typedef struct {
  QTZ_ByteArray rx;
  QTZ_ByteArray tx;
} QTZ_OBC_SerialInterface;

typedef enum {
  QTZ_OBC_STATE_IDLE = 0,
  QTZ_OBC_STATE_ERROR,
  QTZ_OBC_STATE_HANDOVER_IDLE,
} QTZ_OBC_State;

typedef enum {
  QTZ_OBC_TASK_COMMAND_HANDLED,
  QTZ_OBC_TASK_COMMAND_UNHANDLED,
} QTZ_OBC_TaskCommandHandling;

typedef struct {
  // Defines all the possible states that this task can be in.
  //
  // If it fails, check the error field for more details!
  enum {
    QTZ_OBC_MILO_TASK_STATE_UNSTARTED,
    QTZ_OBC_MILO_TASK_STATE_BEGIN,
    QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE,
    QTZ_OBC_MILO_TASK_STATE_GET_DATA,
    QTZ_OBC_MILO_TASK_STATE_END,
    QTZ_OBC_MILO_TASK_STATE_ERROR,
  } state;
  enum {
    QTZ_OBC_MILO_TASK_NO_ERROR,
    QTZ_OBC_MILO_TASK_ERROR_TAKING_PICTURE,
    QTZ_OBC_MILO_TASK_ERROR_GETTING_DATA,
  } error;
  uint8_t image_classification;
} QTZ_OBC_MILO_Task;

typedef struct {
  QTZ_OBC_SerialInterface i2c;
  QTZ_OBC_SerialInterface uart_rs485;

  QTZ_OBC_MILO_Task milo_task;

  volatile QTZ_OBC_State state;
  volatile uint32_t watchdog_ticks;
} QTZ_OBC_Ctx;

extern QTZ_OBC_Ctx GLOBAL_CTX;

// Tick the main OBC routine to the next step on the simulation.
void QTZ_OBC_Routine_Tick(QTZ_OBC_Ctx *ctx);

// Initialize the OBC with the default global buffers.
void QTZ_OBC_InitWithGlobals(QTZ_OBC_Ctx *ctx);

// Arm the interrupts for receiving commands.
void QTZ_OBC_ArmInterrupts();

// =================
// -- Private API --
// =================
char *QTZ_OBC_StateToStr(QTZ_OBC_State st);
char *QTZ_OBC_MiloTaskToStr(int variant);
char *QTZ_OBC_CommandToStr(QTZ_OBC_Command cmd);
QTZ_OBC_OperationResult QTZ_OBC_ParsePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet *p);
QTZ_OBC_OperationResult QTZ_OBC_WritePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet p);

// Begins a critical section.
// No interrupts can happen, and no other thread can enter.
void QTZ_OBC_BeginCritical(void);

// Ends a critical section.
void QTZ_OBC_EndCritical(void);

// Arms a message to be sent through RS485.
//
// This functions asssumes it works with interrupts.
QTZ_OBC_OperationResult QTZ_OBC_SendRS485_IT(QTZ_ByteArray *msg);

// Arms a message to be sent through I2C.
//
// This functions asssumes it works with interrupts.
QTZ_OBC_OperationResult QTZ_OBC_SendI2C_IT(QTZ_ByteArray *msg);

#ifdef QTZ_LIB_IMPLEMENTATION

// ========================================================
// common.c
// ========================================================

void QTZ_ByteArray_Init(QTZ_ByteArray *self, uint8_t *data, size_t capacity) {
  self->data = data;
  self->length = 0;
  self->capacity = capacity;
}

void QTZ_ByteArray_InitWithLength(QTZ_ByteArray *self, uint8_t *data,
                                  size_t capacity, size_t length) {
  self->data = data;
  self->length = length;
  self->capacity = capacity;
}

QTZ_BYTEARRAYGET_Result QTZ_ByteArray_Get(QTZ_ByteArray *self, size_t index,
                                          uint8_t *out) {
  if (index >= self->length) {
    return QTZ_BYTEARRAYGET_INDEX_OUT_OF_BOUNDS;
  }

  *out = self->data[index];
  return QTZ_BYTEARRAYGET_OK;
}

QTZ_BYTEARRAYSET_Result QTZ_ByteArray_Set(QTZ_ByteArray *self, size_t index,
                                          uint8_t value) {
  if (index >= self->length) {
    return QTZ_BYTEARRAYSET_INDEX_OUT_OF_BOUNDS;
  }

  self->data[index] = value;
  return QTZ_BYTEARRAYSET_OK;
}

QTZ_BYTEARRAYAPPEND_Result QTZ_ByteArray_Append(QTZ_ByteArray *self,
                                                uint8_t value) {
  if (self->length >= self->capacity) {
    return QTZ_BYTEARRAYAPPEND_NOT_ENOUGH_SPACE;
  }

  self->data[self->length] = value;
  self->length++;
  return QTZ_BYTEARRAYAPPEND_OK;
}

QTZ_BYTEARRAYEXTEND_Result QTZ_ByteArray_Extend(QTZ_ByteArray *self,
                                                QTZ_ByteArray *other) {
  if ((self->length + other->length) > self->capacity) {
    return QTZ_BYTEARRAYEXTEND_NOT_ENOUGH_SPACE;
  }

  uint8_t *dest = self->data + self->length;
  memcpy(dest, other->data, other->length);

  self->length += other->length;

  return QTZ_BYTEARRAYEXTEND_OK;
}

QTZ_BYTEARRAYEXTEND_Result QTZ_ByteArray_ExtendCStr(QTZ_ByteArray *self,
                                                    char *other) {
  QTZ_ByteArray tmpBuffer = {};
  QTZ_ByteArray_Init(&tmpBuffer, (uint8_t *)other, strlen(other));
  return QTZ_ByteArray_Extend(self, &tmpBuffer);
}

void QTZ_ByteArray_Reset(QTZ_ByteArray *self) { self->length = 0; }

size_t QTZ_ByteArray_Remaining(QTZ_ByteArray *self) {
  return self->capacity - self->length;
}

uint8_t *QTZ_ByteArray_Current(QTZ_ByteArray *self) {
  return self->data + self->length;
}

void QTZ_AssertNotNull(void *p) {
  if (p == NULL) {
    QTZ_Panic();
  }
}

void QTZ_AssertTrue(QTZ_Bool v) {
  if (!v) {
    QTZ_Panic();
  }
}

void QTZ_AssertFalse(QTZ_Bool v) {
  if (v) {
    QTZ_Panic();
  }
}

// ======================================================================
// debug.c
// ======================================================================
#ifdef QTZ_DEBUG
uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];
void QTZ_Debug_InnerLog(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  int needed_space =
      vsnprintf((char *)(__DEBUG_INNER_BUFFER), QTZ_DEBUG_CAPACITY, msg, args);
  if (needed_space >= QTZ_DEBUG_CAPACITY) {
    __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY - 1] = 0;
  }
  va_end(args);
  QTZ_Debug_Print();
}
#endif

// ======================================================================
// obc.c
// ======================================================================
#define QTZ_OBC_ROUTINE_PREFIX "[OBC-MR]: "
#define QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT                          \
  QTZ_OBC_ROUTINE_PREFIX                                                       \
  "Can't transition from `%s` -> `%s`. State should be: `%s`"

#define QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION2_LOG_FMT                         \
  QTZ_OBC_ROUTINE_PREFIX                                                       \
  "Can't transition from `%s` -> `%s`. State should be: `%s` or `%s`"

#define QTZ_OBC_RECEIVED_COMMAND_FMT                                           \
  QTZ_OBC_ROUTINE_PREFIX                                                       \
  "State: [%-25s][%-25s], received command: [%c][%d][%c][%-20s][%d][%d]"
#define QTZ_OBC_SENDING_COMMAND_FMT                                            \
  QTZ_OBC_ROUTINE_PREFIX                                                       \
  "State: [%-25s][%-25s], sending  command: [%c][%d][%c][%-20s][%d][%d]"

QTZ_OBC_Ctx GLOBAL_CTX;

// -- I2C --
uint8_t i2c_rx_buffer[QTZ_OBC_I2C_RX_LEN];
uint8_t i2c_tx_buffer[QTZ_OBC_I2C_TX_LEN];

// -- UART --
uint8_t uart_rx_buffer[QTZ_OBC_UART_RX_LEN];
uint8_t uart_tx_buffer[QTZ_OBC_UART_TX_LEN];

// ================
// -- Public API --
// ================

void QTZ_OBC_InitWithGlobals(QTZ_OBC_Ctx *ctx) {
  QTZ_ByteArray_CreateOnlyBuffer(rx_buff, i2c_rx_buffer, QTZ_OBC_I2C_RX_LEN);
  QTZ_ByteArray_CreateOnlyBuffer(tx_buff, i2c_tx_buffer, QTZ_OBC_I2C_TX_LEN);

  ctx->i2c.rx = rx_buff;
  ctx->i2c.tx = tx_buff;

  QTZ_ByteArray_CreateOnlyBuffer(rx2_buff, uart_rx_buffer, QTZ_OBC_UART_RX_LEN);
  QTZ_ByteArray_CreateOnlyBuffer(tx2_buff, uart_tx_buffer, QTZ_OBC_UART_TX_LEN);

  ctx->uart_rs485.rx = rx2_buff;
  ctx->uart_rs485.tx = tx2_buff;

  ctx->state = QTZ_OBC_STATE_IDLE;
  ctx->watchdog_ticks = 0;

  // Arm the receiver interrupt
  QTZ_OBC_ArmInterrupts();
}

#define QTZ_OBC_RespondWithPacket(ctx, buffer, p)                              \
  QTZ_Debug_Log(QTZ_OBC_SENDING_COMMAND_FMT, QTZ_OBC_StateToStr(ctx->state),   \
                QTZ_OBC_MiloTaskToStr(ctx->milo_task.state), p.protocol_id,    \
                p.status, p.subsys, QTZ_OBC_CommandToStr(p.cmd_id), p.param0,  \
                p.param1);                                                     \
  QTZ_OBC_WritePacket(buffer, p);

// Handle GOMSPACE OBC Command.
//
// Main OBC state machine for handling communications between the main OBC and
// the secondary OBC.
static void QTZ_OBC_HandleHandoverCommand(QTZ_OBC_Ctx *ctx, QTZ_OBC_Packet *p) {
  if (ctx == NULL || p == NULL) {
    return;
  }

  switch (p->cmd_id) {
  case QTZ_OBC_COMMAND_MILO_PING: { // FIXME: Only for testing, remove after
                                    // testing.
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO,
        .cmd_id = QTZ_OBC_COMMAND_MILO_PING,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_PING: {
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER: {
    if (ctx->state != QTZ_OBC_STATE_IDLE) {
      QTZ_Debug_Warning(QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
                        QTZ_OBC_StateToStr(ctx->state),
                        QTZ_OBC_StateToStr(QTZ_OBC_STATE_HANDOVER_IDLE),
                        QTZ_OBC_StateToStr(QTZ_OBC_STATE_IDLE));
      return;
    }
    ctx->state = QTZ_OBC_STATE_HANDOVER_IDLE;
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT: {
    if (ctx->state != QTZ_OBC_STATE_HANDOVER_IDLE) {
      QTZ_Debug_Warning(
          QTZ_OBC_ROUTINE_PREFIX
          "Can't heartbeat when no handover begin has been called!");
      return;
    }
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_STATUS: {
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK,
        .param0 = ctx->state,
        .param1 = ctx->milo_task.state,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_START_TASK: {
    if (ctx->state != QTZ_OBC_STATE_HANDOVER_IDLE) {
      QTZ_Debug_Warning(QTZ_OBC_ROUTINE_PREFIX
                        "Can't start milo task, OBC is not on handover "
                        "idle mode! (Current: %s)",
                        QTZ_OBC_StateToStr(ctx->state));
      return;
    }
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_UNSTARTED &&
        ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_END) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION2_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_UNSTARTED),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_END));
      return;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_BEGIN;
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_START_TASK_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);

    QTZ_OBC_Packet milo_req = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO, // Send to MILO.
        .cmd_id = QTZ_OBC_COMMAND_MILO_PING,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, milo_req);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI: {
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI_ACK,
        .param0 = ctx->milo_task.image_classification,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  case QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER: {
    if (ctx->state != QTZ_OBC_STATE_HANDOVER_IDLE) {
      QTZ_Debug_Warning(QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
                        QTZ_OBC_StateToStr(ctx->state),
                        QTZ_OBC_StateToStr(QTZ_OBC_STATE_IDLE),
                        QTZ_OBC_StateToStr(QTZ_OBC_STATE_HANDOVER_IDLE));
      return;
    }

    ctx->state = QTZ_OBC_STATE_IDLE;
    QTZ_OBC_Packet resp = {
        .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE, // Send to gomspace.
        .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER_ACK,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->i2c.tx, resp);
  } break;
  }
}

QTZ_OBC_TaskCommandHandling QTZ_OBC_MILO_TaskTick(QTZ_OBC_Ctx *ctx,
                                                  QTZ_OBC_Packet *p) {
  switch (p->cmd_id) {
  case QTZ_OBC_COMMAND_MILO_PING_ACK: {
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_BEGIN &&
        ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_END) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION2_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_BEGIN),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_END));
      return QTZ_OBC_TASK_COMMAND_UNHANDLED;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE;
    QTZ_OBC_Packet req = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO, // Send to MILO.
        .cmd_id = QTZ_OBC_COMMAND_MILO_TAKE_PICTURE,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, req);
  } break;
  case QTZ_OBC_COMMAND_MILO_TAKE_PICTURE_ACK: {
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_GET_DATA),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE));
      return QTZ_OBC_TASK_COMMAND_UNHANDLED;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_GET_DATA;
    QTZ_OBC_Packet req = {
        .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
        .status = QTZ_OBC_RESULT_OK,
        .subsys = QTZ_OBC_SUBSYSTEM_MILO, // Send to MILO.
        .cmd_id = QTZ_OBC_COMMAND_MILO_PICTURE_CLASI,
        .param0 = 0,
        .param1 = 0,
    };
    QTZ_OBC_RespondWithPacket(ctx, &ctx->uart_rs485.tx, req);
  } break;
  case QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK: {
    if (ctx->milo_task.state != QTZ_OBC_MILO_TASK_STATE_GET_DATA) {
      QTZ_Debug_Warning(
          QTZ_OBC_STATE_MACHINE_FAIL_TRANSITION_LOG_FMT,
          QTZ_OBC_MiloTaskToStr(ctx->milo_task.state),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_END),
          QTZ_OBC_MiloTaskToStr(QTZ_OBC_MILO_TASK_STATE_GET_DATA));
      return QTZ_OBC_TASK_COMMAND_UNHANDLED;
    }
    ctx->milo_task.state = QTZ_OBC_MILO_TASK_STATE_END;
    // Task has ended, so now we wait for the main OBC to want to retrieve
    // the result of the operation. We just need to save the operation result.
    ctx->milo_task.image_classification = p->param0;
  } break;
  default:
    return QTZ_OBC_TASK_COMMAND_UNHANDLED;
  }

  // If none of the switch case statements above early returned, then the input
  // has been handled!
  return QTZ_OBC_TASK_COMMAND_HANDLED;
}

static void QTZ_OBC_HandleSubsystemCommand(QTZ_OBC_Ctx *ctx,
                                           QTZ_OBC_Packet *p) {
  if (QTZ_OBC_TASK_COMMAND_HANDLED == QTZ_OBC_MILO_TaskTick(ctx, p)) {
    return; // MILO handled the command, so we don't need to check if the other
            // subsystems should handle it!
  }
  // NOTE: Add other tasks for other subsystems...
}

void QTZ_OBC_Routine_Tick(QTZ_OBC_Ctx *ctx) {
  ctx->watchdog_ticks += 1;

  QTZ_OBC_Packet p;
  if (QTZ_OBC_RESULT_OK != QTZ_OBC_ParsePacket(&ctx->i2c.rx, &p)) {
    QTZ_Debug_Warning(QTZ_OBC_ROUTINE_PREFIX
                      "No packet received from gomspace! Checking other "
                      "submodules...");
    if (QTZ_OBC_RESULT_OK != QTZ_OBC_ParsePacket(&ctx->uart_rs485.rx, &p)) {
      QTZ_Debug_Warning(QTZ_OBC_ROUTINE_PREFIX
                        "No packet received from any submodule either! "
                        "Doing nothing...");
      return;
    }
  }

  QTZ_Debug_Log(QTZ_OBC_RECEIVED_COMMAND_FMT, QTZ_OBC_StateToStr(ctx->state),
                QTZ_OBC_MiloTaskToStr(ctx->milo_task.state), p.protocol_id,
                p.status, p.subsys, QTZ_OBC_CommandToStr(p.cmd_id), p.param0,
                p.param1);

  if (p.subsys != QTZ_OBC_SUBSYSTEM_PORTENTA) {
    QTZ_Debug_Log(QTZ_OBC_ROUTINE_PREFIX
                  "Ignoring command since it doesn't belong to PortentaH7!");
    return;
  }

  switch (p.protocol_id) {
  case QTZ_OBC_PROTOCOL_HANDOVER: {
    QTZ_OBC_HandleHandoverCommand(ctx, &p);
  } break;
  case QTZ_OBC_PROTOCOL_SUBSYSTEMS: {
    QTZ_OBC_HandleSubsystemCommand(ctx, &p);
  } break;
  }

  if (ctx->i2c.tx.length != 0) {
    if (QTZ_OBC_RESULT_OK != QTZ_OBC_SendI2C_IT(&ctx->i2c.tx)) {
      QTZ_Debug_Error(QTZ_OBC_ROUTINE_PREFIX
                      "Failed to arm the I2C msg transmission!");
      ctx->state = QTZ_OBC_STATE_ERROR;
    } else {
      QTZ_Debug_Log(QTZ_OBC_ROUTINE_PREFIX "I2C packet armed!");
    }
  }

  if (ctx->uart_rs485.tx.length != 0) {
    if (QTZ_OBC_RESULT_OK != QTZ_OBC_SendRS485_IT(&ctx->uart_rs485.tx)) {
      QTZ_Debug_Error(QTZ_OBC_ROUTINE_PREFIX
                      "Failed to arm the RS485 msg transmission!");
      ctx->state = QTZ_OBC_STATE_ERROR;
    } else {
      QTZ_Debug_Log(QTZ_OBC_ROUTINE_PREFIX "RS485 packet armed! length: %d",
                    ctx->uart_rs485.tx.length);
    }
  }
}

char *QTZ_OBC_StateToStr(QTZ_OBC_State st) {
  switch (st) {
  case QTZ_OBC_STATE_IDLE:
    return "IDLE";
  case QTZ_OBC_STATE_ERROR:
    return "ERROR";
  case QTZ_OBC_STATE_HANDOVER_IDLE:
    return "HANDOVER_IDLE";
  default:
    return "UNKNOWN";
  }
}

char *QTZ_OBC_MiloTaskToStr(int variant) {
  switch (variant) {
  case QTZ_OBC_MILO_TASK_STATE_UNSTARTED: {
    return "MILO_UNSTARTED";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_BEGIN: {
    return "MILO_BEGIN";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_TAKE_PICTURE: {
    return "MILO_TAKE_PICTURE";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_GET_DATA: {
    return "MILO_GET_DATA";
  } break;
  case QTZ_OBC_MILO_TASK_STATE_END: {
    return "MILO_END";
  } break;
  default:
    return "MILO_UNKNOWN";
  }
}

char *QTZ_OBC_CommandToStr(QTZ_OBC_Command cmd) {
  switch (cmd) {
  case QTZ_OBC_COMMAND_GOMSPACE_PING:
    return "PING";
  case QTZ_OBC_COMMAND_GOMSPACE_PING_ACK:
    return "PING_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER:
    return "BEGIN_HANDOVER";
  case QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER_ACK:
    return "HANDOVER_BEGIN_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT:
    return "HEARTBEAT";
  case QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT_ACK:
    return "HEARTBEAT_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_STATUS:
    return "STATUS";
  case QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK:
    return "STATUS_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_START_TASK:
    return "START_TASK";
  case QTZ_OBC_COMMAND_GOMSPACE_START_TASK_ACK:
    return "START_TASK_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI:
    return "GET_IMAGE_CLASI";
  case QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI_ACK:
    return "GET_IMAGE_CLASI_ACK";
  case QTZ_OBC_COMMAND_MILO_PING:
    return "PING";
  case QTZ_OBC_COMMAND_MILO_PING_ACK:
    return "PING_ACK";
  case QTZ_OBC_COMMAND_MILO_TAKE_PICTURE:
    return "TAKE_PICTURE";
  case QTZ_OBC_COMMAND_MILO_TAKE_PICTURE_ACK:
    return "TAKE_PICTURE_ACK";
  case QTZ_OBC_COMMAND_MILO_PICTURE_CLASI:
    return "PICTURE_CLASI";
  case QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK:
    return "PICTURE_CLASI_ACK";
  case QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER:
    return "ABORT_HANDOVER";
  case QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER_ACK:
    return "ABORT_HANDOVER_ACK";
  case QTZ_OBC_COMMAND_TIMEOUT:
    return "TIMEOUT";
  default:
    return "UNKNOWN";
  }
}

QTZ_OBC_OperationResult QTZ_OBC_ParsePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet *p) {
  if (buffer == NULL || p == NULL) {
    return QTZ_OBC_RESULT_ERROR;
  }

  QTZ_OBC_OperationResult result = QTZ_OBC_RESULT_ERROR;

  QTZ_OBC_BeginCritical();
  if (buffer->length >= QTZ_OBC_PACKET_LEN) {
    memcpy(p, buffer->data, QTZ_OBC_PACKET_LEN);
    QTZ_ByteArray_Reset(buffer);
    result = QTZ_OBC_RESULT_OK;
  }
  QTZ_OBC_EndCritical();

  return result;
}

// Writes the packet to the buffer.
//
// Will overwrite all data available in the buffer with the contents of the
// packet.
QTZ_OBC_OperationResult QTZ_OBC_WritePacket(QTZ_ByteArray *buffer,
                                            QTZ_OBC_Packet p) {
  if (buffer == NULL) {
    return QTZ_OBC_RESULT_ERROR;
  }

  QTZ_OBC_OperationResult result = QTZ_OBC_RESULT_ERROR;

  QTZ_OBC_BeginCritical();
  if (buffer->capacity >= QTZ_OBC_PACKET_LEN) {
    memcpy(buffer->data, &p, QTZ_OBC_PACKET_LEN);
    buffer->length = QTZ_OBC_PACKET_LEN;
    result = QTZ_OBC_RESULT_OK;
  }
  QTZ_OBC_EndCritical();

  return result;
}

size_t QTZ_DigitQuantity(size_t n) {
  if (n < 2) { // Handle edge cases... AKA (0, 1)
    return 1;
  } else {
    // This formula only works if n is 2 or more!
    return ((size_t)log10(n)) + 1;
  }
}

QTZ_FMTSIZET_Result QTZ_FmtSizeT(size_t n, QTZ_ByteArray *buffer) {
  if (n == 0) {
    if (buffer->capacity <= 0) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    if (QTZ_BYTEARRAYAPPEND_OK != QTZ_ByteArray_Append(buffer, '0')) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    return QTZ_FMTSIZET_OK;
  } else {
    size_t buffer_length = QTZ_DigitQuantity(n);
    size_t remaining_space = buffer->capacity - buffer->length;
    if (remaining_space < buffer_length) {
      return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
    }

    for (size_t i = 0; i < buffer_length; i++) {
      size_t zeros = buffer_length - i - 1;
      uint8_t character = '?';
      if (zeros > 0) {
        size_t divider = pow(10, zeros);
        size_t digit = n / divider;
        character = digit + '0';

        n -= digit * divider;
      } else {
        character = n + '0';
      }

      if (character < '0' ||
          character > '9') { // Something went wrong with string conversion!
        character = '?';
      }

      if (QTZ_BYTEARRAYAPPEND_OK != QTZ_ByteArray_Append(buffer, character)) {
        return QTZ_FMTSIZET_BUFFER_NOT_LARGE_ENOUGH;
      }
    }

    return QTZ_FMTSIZET_OK;
  }
}

#include "./std_stubs.c"
#endif

#endif
