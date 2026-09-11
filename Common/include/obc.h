#ifndef QTZ_LIB_OBC
#define QTZ_LIB_OBC
#include "common.h"

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

#define QTZ_OBC_I2C_TX_LEN QTZ_OBC_PACKET_LEN * 3
#define QTZ_OBC_I2C_RX_LEN QTZ_OBC_PACKET_LEN

#define QTZ_OBC_UART_TX_LEN QTZ_OBC_PACKET_LEN * 10
#define QTZ_OBC_UART_RX_LEN QTZ_OBC_PACKET_LEN * 10

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

#endif
