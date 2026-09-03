#include "../../deps/unity/unity.h"
#include "../common.c"
#include "../debug.c"
#include "../obc.c"
#include <stdio.h>

// Stub implementation of logs. Since we're using the debug interface!
// Comment out the implementation if the logs become to much...
extern uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];
void QTZ_Debug_Print() {
  // uint16_t size = strlen((char *)__DEBUG_INNER_BUFFER);
  // printf("%.*s", size, (char *)__DEBUG_INNER_BUFFER);
}

void setUp() {}
void tearDown() {}

void test_QTZ_BasicPing() {
  // First we create the buffers...
  // This will hold all the data we'll try to transport to other modules.
  QTZ_ByteArray_Create(i2c_tx, i2c_tx_buffer, QTZ_OBC_I2C_TX_LEN);
  QTZ_ByteArray_Create(i2c_rx, i2c_rx_buffer, QTZ_OBC_I2C_RX_LEN);
  QTZ_ByteArray_Create(uart_tx, uart_tx_buffer, QTZ_OBC_UART_TX_LEN);
  QTZ_ByteArray_Create(uart_rx, uart_rx_buffer, QTZ_OBC_UART_RX_LEN);

  // We expect some of the previous tx channels to match this buffer! Depending
  // on the test.
  QTZ_ByteArray_Create(expected, expected_buffer, 100);

  // Next we arm the OBC context...
  QTZ_OBC_Ctx ctx = {
      .state = QTZ_OBC_STATE_IDLE,
      .i2c =
          {
              .tx = i2c_tx,
              .rx = i2c_rx,
          },
      .uart_rs485 =
          {
              .tx = uart_tx,
              .rx = uart_rx,
          },
  };

  // OBC context primed! Let's use it!
  // We first need to emulate we received an interrupt with a message from the
  // primary OBC... For example a simple ping!
  QTZ_OBC_Packet p = {
      .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
      .status = QTZ_OBC_RESULT_OK,
      .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
      .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING,
      .param0 = 0,
      .param1 = 0,
  };
  TEST_ASSERT_EQUAL(QTZ_OBC_RESULT_OK, QTZ_OBC_WritePacket(&ctx.i2c.rx, p));
  TEST_ASSERT_EQUAL(QTZ_OBC_PACKET_LEN, ctx.i2c.rx.length);

  // Then we tick the secondary OBC state machine. In theory, this should ping
  // us back on the i2c_tx buffer.
  QTZ_OBC_Routine_Tick(&ctx);

  // Let's check the buffers!
  TEST_ASSERT_EQUAL(QTZ_OBC_RESULT_OK, QTZ_OBC_ParsePacket(&ctx.i2c.tx, &p));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_IDLE, ctx.state);
  // The packet should be a ping ACK packet, directed towards the primary OBC.
  // Both parameters should be empty.
  TEST_ASSERT_EQUAL(QTZ_OBC_PROTOCOL_HANDOVER, p.protocol_id);
  TEST_ASSERT_EQUAL(QTZ_OBC_RESULT_OK, p.status);
  TEST_ASSERT_EQUAL(QTZ_OBC_SUBSYSTEM_GOMSPACE, p.subsys);
  TEST_ASSERT_EQUAL(QTZ_OBC_COMMAND_GOMSPACE_PING_ACK, p.cmd_id);
  TEST_ASSERT_EQUAL(0, p.param0);
  TEST_ASSERT_EQUAL(0, p.param1);
}

#define QTZ_INITIALIZE_OBC(ctx_name)                                           \
  QTZ_ByteArray_Create(i2c_tx, i2c_tx_buffer, QTZ_OBC_I2C_TX_LEN);             \
  QTZ_ByteArray_Create(i2c_rx, i2c_rx_buffer, QTZ_OBC_I2C_RX_LEN);             \
  QTZ_ByteArray_Create(uart_tx, uart_tx_buffer, QTZ_OBC_UART_TX_LEN);          \
  QTZ_ByteArray_Create(uart_rx, uart_rx_buffer, QTZ_OBC_UART_RX_LEN);          \
                                                                               \
  QTZ_ByteArray_Create(expected, expected_buffer, 100);                        \
                                                                               \
  QTZ_OBC_Ctx ctx_name = {                                                     \
      .state = QTZ_OBC_STATE_IDLE,                                             \
      .i2c =                                                                   \
          {                                                                    \
              .tx = i2c_tx,                                                    \
              .rx = i2c_rx,                                                    \
          },                                                                   \
      .uart_rs485 =                                                            \
          {                                                                    \
              .tx = uart_tx,                                                   \
              .rx = uart_rx,                                                   \
          },                                                                   \
  };

void CLEAN_OBC_BUFFERS(QTZ_OBC_Ctx *ctx) {
  QTZ_ByteArray_Reset(&ctx->i2c.tx);
  QTZ_ByteArray_Reset(&ctx->i2c.rx);

  QTZ_ByteArray_Reset(&ctx->uart_rs485.tx);
  QTZ_ByteArray_Reset(&ctx->uart_rs485.rx);
}

#define TEST_ASSERT_OBC_COMMAND(ctx, expected, req_arr, req, resp_arr,         \
                                exp_packet)                                    \
  TEST_ASSERT_EQUAL(QTZ_OBC_RESULT_OK, QTZ_OBC_WritePacket(&req_arr, req));    \
  TEST_ASSERT_EQUAL(QTZ_OBC_PACKET_LEN, req_arr.length);                       \
                                                                               \
  QTZ_OBC_Routine_Tick(&ctx);                                                  \
  TEST_ASSERT_EQUAL(QTZ_OBC_RESULT_OK,                                         \
                    QTZ_OBC_WritePacket(&expected, exp_packet));               \
  TEST_ASSERT_EQUAL_MEMORY(expected.data, resp_arr.data, expected.length);     \
                                                                               \
  QTZ_ByteArray_Reset(&expected);                                              \
  CLEAN_OBC_BUFFERS(&ctx);

static inline QTZ_OBC_Packet mkPacket(QTZ_OBC_Packet p) { return p; }

void test_QTZ_HandoverInit() {
  QTZ_INITIALIZE_OBC(ctx);

  TEST_ASSERT_OBC_COMMAND(ctx, expected, ctx.i2c.rx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING,
                              .param0 = 0,
                              .param1 = 0,
                          }),
                          ctx.i2c.tx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING_ACK,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_IDLE, ctx.state);

  TEST_ASSERT_OBC_COMMAND(
      ctx, expected, ctx.i2c.rx,
      mkPacket((QTZ_OBC_Packet){
          .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
          .status = QTZ_OBC_RESULT_OK,
          .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
          .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER,
          .param0 = 0,
          .param1 = 0,
      }),
      ctx.i2c.tx,
      mkPacket((QTZ_OBC_Packet){
          .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
          .status = QTZ_OBC_RESULT_OK,
          .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
          .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_BEGIN_HANDOVER_ACK,
          .param0 = 0,
          .param1 = 0,
      }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);

  for (int i = 0; i < 2; i++) {
    TEST_ASSERT_OBC_COMMAND(
        ctx, expected, ctx.i2c.rx,
        mkPacket((QTZ_OBC_Packet){
            .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
            .status = QTZ_OBC_RESULT_OK,
            .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
            .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT,
            .param0 = 0,
            .param1 = 0,
        }),
        ctx.i2c.tx,
        mkPacket((QTZ_OBC_Packet){
            .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
            .status = QTZ_OBC_RESULT_OK,
            .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
            .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_HEARTBEAT_ACK,
            .param0 = 0,
            .param1 = 0,
        }));
    TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);
  }

  TEST_ASSERT_OBC_COMMAND(ctx, expected, ctx.i2c.rx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS,
                              .param0 = 0,
                              .param1 = 0,
                          }),
                          ctx.i2c.tx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK,
                              .param0 = QTZ_OBC_STATE_HANDOVER_IDLE,
                              .param1 = QTZ_OBC_MILO_TASK_STATE_UNSTARTED,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);

  TEST_ASSERT_OBC_COMMAND(ctx, expected, ctx.i2c.rx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_START_TASK,
                              .param0 = 0,
                              .param1 = 0,
                          }),
                          ctx.i2c.tx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_START_TASK_ACK,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);
  TEST_ASSERT_EQUAL(QTZ_OBC_MILO_TASK_STATE_BEGIN, ctx.milo_task.state);

  // -- Check if MILO command was sent
  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&expected,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_MILO,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_PING,
                              .param0 = 0,
                              .param1 = 0,
                          })));
  TEST_ASSERT_EQUAL_MEMORY(expected.data, ctx.uart_rs485.tx.data,
                           expected.length);

  // -- Emulate MILO answer
  TEST_ASSERT_OBC_COMMAND(ctx, expected, ctx.uart_rs485.rx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_PING_ACK,
                              .param0 = 0,
                              .param1 = 0,
                          }),
                          ctx.uart_rs485.tx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_MILO,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_TAKE_PICTURE,
                              .param0 = 0,
                              .param1 = 0,
                          }));

  // -- Emulate MILO answer
  TEST_ASSERT_OBC_COMMAND(ctx, expected, ctx.uart_rs485.rx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_TAKE_PICTURE_ACK,
                              .param0 = 0,
                              .param1 = 0,
                          }),
                          ctx.uart_rs485.tx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_MILO,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_PICTURE_CLASI,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);
  TEST_ASSERT_EQUAL(QTZ_OBC_MILO_TASK_STATE_GET_DATA, ctx.milo_task.state);

  // Delete all data from the uart tx buffer.
  // This command doesn't need a response from portenta.
  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.uart_rs485.tx, mkPacket((QTZ_OBC_Packet){})));
  TEST_ASSERT_OBC_COMMAND(
      ctx, expected, ctx.uart_rs485.rx,
      mkPacket((QTZ_OBC_Packet){
          .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
          .status = QTZ_OBC_RESULT_OK,
          .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
          .cmd_id = QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK,
          .param0 = QTZ_OBC_MILO_IMAGE_CLASSIFICATION_HIGH,
          .param1 = 0,
      }),
      ctx.uart_rs485.tx,
      mkPacket((QTZ_OBC_Packet){
          // This packet doesn't matter, no answer is given from portenta
      }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);
  TEST_ASSERT_EQUAL(QTZ_OBC_MILO_TASK_STATE_END, ctx.milo_task.state);

  TEST_ASSERT_OBC_COMMAND(ctx, expected, ctx.i2c.rx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS,
                              .param0 = 0,
                              .param1 = 0,
                          }),
                          ctx.i2c.tx,
                          mkPacket((QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS_ACK,
                              .param0 = ctx.state,
                              .param1 = ctx.milo_task.state,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);
  TEST_ASSERT_EQUAL(QTZ_OBC_MILO_TASK_STATE_END, ctx.milo_task.state);

  TEST_ASSERT_OBC_COMMAND(
      ctx, expected, ctx.i2c.rx,
      mkPacket((QTZ_OBC_Packet){
          .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
          .status = QTZ_OBC_RESULT_OK,
          .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
          .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI,
          .param0 = 0,
          .param1 = 0,
      }),
      ctx.i2c.tx,
      mkPacket((QTZ_OBC_Packet){
          .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
          .status = QTZ_OBC_RESULT_OK,
          .subsys = QTZ_OBC_SUBSYSTEM_GOMSPACE,
          .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_GET_IMAGE_CLASI_ACK,
          .param0 = ctx.milo_task.image_classification,
          .param1 = 0,
      }));
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_HANDOVER_IDLE, ctx.state);
  TEST_ASSERT_EQUAL(QTZ_OBC_MILO_TASK_STATE_END, ctx.milo_task.state);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_QTZ_BasicPing);
  RUN_TEST(test_QTZ_HandoverInit);
  return UNITY_END();
}
