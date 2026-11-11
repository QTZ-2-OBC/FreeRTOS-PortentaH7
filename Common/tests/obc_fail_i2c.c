#define QTZ_LIB_IMPLEMENTATION
#include "../../deps/unity/unity.h"
#include "../obc.h"

// =======================================
// BEGIN STUB IMPLEMENTATIONS
// =======================================
// Stub implementation of logs. Since we're using the debug interface!
// Comment out the implementation if the logs become to much...
extern uint8_t __DEBUG_INNER_BUFFER[QTZ_DEBUG_CAPACITY];
void QTZ_Debug_Print() {
  // uint16_t size = strlen((char *)__DEBUG_INNER_BUFFER);
  // printf("%.*s", size, (char *)__DEBUG_INNER_BUFFER);
}
void QTZ_OBC_ArmInterrupts() {}
void QTZ_OBC_BeginCritical() {}
void QTZ_OBC_EndCritical() {}
void QTZ_Panic() {}
QTZ_OBC_OperationResult QTZ_OBC_SendI2C_IT(QTZ_ByteArray *msg) {
  return QTZ_OBC_RESULT_ERROR;
};
QTZ_OBC_OperationResult QTZ_OBC_SendRS485_IT(QTZ_ByteArray *msg) {
  return QTZ_OBC_RESULT_OK;
};
// =======================================
// END STUB IMPLEMENTATIONS
// =======================================

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

void test_QTZ_BasicI2CFail() {
  QTZ_INITIALIZE_OBC(ctx);

  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.i2c.rx,
                          (QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_PACKET_LEN, ctx.i2c.rx.length);

  QTZ_OBC_Routine_Tick(&ctx);
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_ERROR, ctx.state);
  CLEAN_OBC_BUFFERS(&ctx);

  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.i2c.rx,
                          (QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_PACKET_LEN, ctx.i2c.rx.length);

  QTZ_OBC_Routine_Tick(&ctx);
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_ERROR, ctx.state);
  CLEAN_OBC_BUFFERS(&ctx);

  TEST_ASSERT_EQUAL(
      QTZ_OBC_RESULT_OK,
      QTZ_OBC_WritePacket(&ctx.i2c.rx,
                          (QTZ_OBC_Packet){
                              .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                              .status = QTZ_OBC_RESULT_OK,
                              .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                              .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_ABORT_HANDOVER,
                              .param0 = 0,
                              .param1 = 0,
                          }));
  TEST_ASSERT_EQUAL(QTZ_OBC_PACKET_LEN, ctx.i2c.rx.length);

  QTZ_OBC_Routine_Tick(&ctx);
  TEST_ASSERT_EQUAL(QTZ_OBC_STATE_ERROR, ctx.state);
  CLEAN_OBC_BUFFERS(&ctx);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_QTZ_BasicI2CFail);
  return UNITY_END();
}
