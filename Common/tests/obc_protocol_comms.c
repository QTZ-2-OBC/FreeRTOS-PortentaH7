#define QTZ_LIB_IMPLEMENTATION
#include "../../deps/unity/unity.h"
#include "../obc.h"
#include "./helpers/obc_stubs.c"

void QTZ_Debug_Print() {
  // uint16_t size = strlen((char *)__DEBUG_INNER_BUFFER);
  // printf("%.*s", size, (char *)__DEBUG_INNER_BUFFER);
}

void setUp() {}
void tearDown() {}

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

void test_QTZ_MILO_TaskStateMachine() {
  QTZ_INITIALIZE_OBC(ctx);

  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.uart_rs485.rx,
                          (QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_PING_ACK,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  QTZ_OBC_Routine_Tick(&ctx);
  QTZ_ByteArray_Reset(&expected);

  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.uart_rs485.rx,
                          (QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_MILO_PING_ACK,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  QTZ_OBC_Routine_Tick(&ctx);
  QTZ_ByteArray_Reset(&expected);
}

void test_QTZ_MILO_CommandCorruption() {
  QTZ_INITIALIZE_OBC(ctx);

  QTZ_Debug_Log("------------------------------------------------------------");
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

  QTZ_Debug_Log("------------------------------------------------------------");
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
    QTZ_Debug_Log(
        "------------------------------------------------------------");
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

  QTZ_Debug_Log("------------------------------------------------------------");
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

  QTZ_Debug_Log("------------------------------------------------------------");
  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.i2c.rx,
                          (QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_START_TASK,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_PACKET_LEN, ctx.i2c.rx.length);
  QTZ_OBC_Routine_Tick(&ctx);

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

  // NOTE: In an ideal world, MILO would answer with a PING_ACK, but here we're
  // emulating an error during transmission so the bytes are jumbled up.
  QTZ_Debug_Log("------------------------------------------------------------");
  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.uart_rs485.rx, (QTZ_OBC_Packet){
                                                  .protocol_id = 12,
                                                  .status = 13,
                                                  .subsys = 56,
                                                  .cmd_id = 246,
                                                  .param0 = 76,
                                                  .param1 = 90,
                                              }));
  QTZ_OBC_Routine_Tick(&ctx);

  // NOTE: Some time later, we receive the correct ping!
  QTZ_Debug_Log("------------------------------------------------------------");
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
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_QTZ_MILO_TaskStateMachine);
  RUN_TEST(test_QTZ_MILO_CommandCorruption);
  return UNITY_END();
}
