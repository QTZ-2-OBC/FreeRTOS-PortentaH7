/**
 * @file    i2c_pingpong_interrupt.c
 * @brief   Interrupt-driven I2C ping-pong with the central MCU as I2C
 *          SLAVE: receive 10 bytes, echo 10 bytes, without blocking.
 *
 * Same structure and rule mapping as uart_pingpong_interrupt.c: a small
 * state machine driven from HAL_I2C_SlaveRxCpltCallback /
 * HAL_I2C_SlaveTxCpltCallback, plus an explicit watchdog to give the
 * otherwise-unbounded "wait for the master" an upper bound (Rule 2).
 */

#include "stm32h7xx_hal.h"

#define I2C_PINGPONG_LEN 10U
#define I2C_PP_WATCHDOG_LIMIT 1000U

typedef enum {
  I2C_PP_STATE_IDLE = 0,
  I2C_PP_STATE_RX_PENDING,
  I2C_PP_STATE_TX_PENDING,
  I2C_PP_STATE_ERROR
} I2C_PingPongState_t;

typedef struct {
  I2C_HandleTypeDef *hi2c;
  uint8_t buffer[I2C_PINGPONG_LEN];
  volatile I2C_PingPongState_t state;
  volatile uint32_t watchdogTicks;
} I2C_PingPong_t;

/** @brief  Arm the slave link and start listening for the first frame. */
HAL_StatusTypeDef I2C_PingPong_IT_Start(I2C_PingPong_t *ctx,
                                        I2C_HandleTypeDef *hi2c) {
  if ((ctx == NULL) || (hi2c == NULL)) /* assertion 1 */
  {
    return HAL_ERROR;
  }
  if (HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_RESET) /* assertion 2 */
  {
    return HAL_ERROR;
  }

  ctx->hi2c = hi2c;
  ctx->watchdogTicks = 0U;
  ctx->state = I2C_PP_STATE_RX_PENDING;

  return HAL_I2C_Slave_Receive_IT(hi2c, ctx->buffer, I2C_PINGPONG_LEN);
}

/** @brief  Transition on slave RX complete: echo the frame back. */
void I2C_PingPong_IT_OnSlaveRxComplete(I2C_PingPong_t *ctx) {
  if (ctx == NULL) {
    return;
  }
  if (ctx->state != I2C_PP_STATE_RX_PENDING) /* assertion */
  {
    ctx->state = I2C_PP_STATE_ERROR;
    return;
  }

  ctx->state = I2C_PP_STATE_TX_PENDING;
  if (HAL_I2C_Slave_Transmit_IT(ctx->hi2c, ctx->buffer, I2C_PINGPONG_LEN) !=
      HAL_OK) {
    ctx->state = I2C_PP_STATE_ERROR; /* Rule 7 */
  }
}

/** @brief  Transition on slave TX complete: re-arm for the next frame. */
void I2C_PingPong_IT_OnSlaveTxComplete(I2C_PingPong_t *ctx) {
  if (ctx == NULL) {
    return;
  }
  if (ctx->state != I2C_PP_STATE_TX_PENDING) {
    ctx->state = I2C_PP_STATE_ERROR;
    return;
  }

  ctx->state = I2C_PP_STATE_RX_PENDING;
  ctx->watchdogTicks = 0U;
  if (HAL_I2C_Slave_Receive_IT(ctx->hi2c, ctx->buffer, I2C_PINGPONG_LEN) !=
      HAL_OK) {
    ctx->state = I2C_PP_STATE_ERROR;
  }
}

/**
 * @brief  Bounded liveness check, call once per scheduler tick. Because a
 *         wedged I2C slave transfer usually needs a bus-level recovery
 *         (not just an abort), the timeout path here flags ERROR for the
 *         application to re-init the peripheral - see the blocking
 *         example's recovery comment for the same caveat.
 */
void I2C_PingPong_IT_Supervise(I2C_PingPong_t *ctx) {
  if (ctx == NULL) {
    return;
  }
  if (ctx->state == I2C_PP_STATE_ERROR) {
    return;
  }

  ctx->watchdogTicks++;
  if (ctx->watchdogTicks > I2C_PP_WATCHDOG_LIMIT) {
    ctx->state = I2C_PP_STATE_ERROR;
  }
}

/* ---- Compile-time dispatch (single I2C master line in this example) ---- */

static I2C_PingPong_t g_masterLinkCtx;

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c == g_masterLinkCtx.hi2c) {
    I2C_PingPong_IT_OnSlaveRxComplete(&g_masterLinkCtx);
  }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c == g_masterLinkCtx.hi2c) {
    I2C_PingPong_IT_OnSlaveTxComplete(&g_masterLinkCtx);
  }
}

/* ---- Public wiring API - this is the only surface main.c needs ---- */

/**
 * @brief  Arm the I2C slave link. Call once, after HAL_I2C_Init() (i.e.
 *         after MX_I2Cx_Init() in the CubeMX-generated main()).
 */
HAL_StatusTypeDef App_I2CLink_Init(I2C_HandleTypeDef *hi2cLink) {
  return I2C_PingPong_IT_Start(&g_masterLinkCtx, hi2cLink);
}

/**
 * @brief  Call once per scheduler tick to enforce the Rule 2 bound.
 */
void App_I2CLink_Supervise(void) {
  I2C_PingPong_IT_Supervise(&g_masterLinkCtx);
}
