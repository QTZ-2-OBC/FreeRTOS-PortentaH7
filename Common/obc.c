#include <include/common.h>
#include <include/obc.h>

// -- I2C --
uint8_t i2c_rx_buffer[QTZ_OBC_I2C_RX_LEN];
uint8_t i2c_tx_buffer[QTZ_OBC_I2C_TX_LEN];

// -- UART --
uint8_t uart_rx_buffer[QTZ_OBC_UART_RX_LEN];
uint8_t uart_tx_buffer[QTZ_OBC_UART_TX_LEN];

// ================
// -- Public API --
// ================

void OBC_InitWithGlobals(QTZ_OBC_Ctx *ctx) {
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
}

void OBC_Tick(QTZ_OBC_Ctx *ctx) {}
