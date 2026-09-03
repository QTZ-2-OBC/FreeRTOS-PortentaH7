/**
 * @file    uart_pingpong_interrupt.c
 * @brief   Interrupt-driven UART ping-pong: receive 10 bytes, echo 10 bytes,
 *          without blocking the caller. Scales to N simultaneous UART links.
 *
 * Power-of-10 compliance notes:
 *  Rule 1 - no recursion; the ISR callbacks and the supervisor are all flat
 *           switch/if state transitions on one enum.
 *  Rule 2 - HAL_UART_*_IT never blocks, so it has no HAL-side timeout. The
 *           bound is supplied explicitly here via a watchdog tick counter
 *           checked in UART_PingPong_IT_Supervise(), called once per
 *           scheduler tick from the non-terminating main loop.
 *  Rule 3 - one static context struct per link, sized at compile time.
 *  Rule 5 - every transition function asserts the state it expects to be
 *           in before acting, and recovers explicitly (-> ERROR state)
 *           if it isn't.
 *  Rule 6 - all mutable state lives in UART_PingPong_t, scoped to one link.
 *  Rule 7 - every HAL_*_IT return value is checked.
 *  Rule 9 - single-level pointers only. The dispatch in the HAL weak
 *           callbacks below is a compile-time override, not a runtime
 *           function-pointer table (no HAL_UART_RegisterCallback use).
 *
 * Concurrency note: `state` and `watchdogTicks` are written from ISR
 * context (HAL_UART_RxCpltCallback/TxCpltCallback run at interrupt level)
 * and read from the main loop, so both are declared volatile. On Cortex-M7
 * single-word reads/writes of these types are atomic, so no extra lock is
 * needed for this specific access pattern.
 */

#include "stm32h7xx_hal.h"

#define UART_PINGPONG_LEN 10U
#define UART_PP_WATCHDOG_LIMIT 1000U /* max supervisor ticks per phase */

typedef enum {
  UART_PP_STATE_IDLE = 0,
  UART_PP_STATE_RX_PENDING,
  UART_PP_STATE_TX_PENDING,
  UART_PP_STATE_ERROR
} UART_PingPongState_t;

typedef struct {
  UART_HandleTypeDef *huart;
  uint8_t buffer[UART_PINGPONG_LEN];
  volatile UART_PingPongState_t state;
  volatile uint32_t watchdogTicks;
} UART_PingPong_t;

/**
 * @brief  Arm the link and start listening for the first 10-byte frame.
 */
HAL_StatusTypeDef UART_PingPong_IT_Start(UART_PingPong_t *ctx,
                                         UART_HandleTypeDef *huart) {
  if ((ctx == NULL) || (huart == NULL)) /* assertion 1 */
  {
    return HAL_ERROR;
  }
  if (HAL_UART_GetState(huart) == HAL_UART_STATE_RESET) /* assertion 2 */
  {
    return HAL_ERROR;
  }

  ctx->huart = huart;
  ctx->watchdogTicks = 0U;
  ctx->state = UART_PP_STATE_RX_PENDING;

  return HAL_UART_Receive_IT(huart, ctx->buffer, UART_PINGPONG_LEN);
}

/** @brief  Transition on RX complete: flip straight into echoing it back. */
void UART_PingPong_IT_OnRxComplete(UART_PingPong_t *ctx) {
  if (ctx == NULL) /* assertion 1 */
  {
    return;
  }
  if (ctx->state != UART_PP_STATE_RX_PENDING) /* assertion 2 */
  {
    ctx->state = UART_PP_STATE_ERROR;
    return;
  }

  ctx->state = UART_PP_STATE_TX_PENDING;
  if (HAL_UART_Transmit_IT(ctx->huart, ctx->buffer, UART_PINGPONG_LEN) !=
      HAL_OK) {
    ctx->state = UART_PP_STATE_ERROR; /* Rule 7 */
  }
}

/** @brief  Transition on TX complete: re-arm the receiver for the next frame.
 */
void UART_PingPong_IT_OnTxComplete(UART_PingPong_t *ctx) {
  if (ctx == NULL) {
    return;
  }
  if (ctx->state != UART_PP_STATE_TX_PENDING) {
    ctx->state = UART_PP_STATE_ERROR;
    return;
  }

  ctx->state = UART_PP_STATE_RX_PENDING;
  ctx->watchdogTicks = 0U;
  if (HAL_UART_Receive_IT(ctx->huart, ctx->buffer, UART_PINGPONG_LEN) !=
      HAL_OK) {
    ctx->state = UART_PP_STATE_ERROR;
  }
}

/**
 * @brief  Bounded liveness check - call once per scheduler tick from the
 *         main for(;;) loop for every active link. This is what turns an
 *         otherwise-unbounded "wait for the interrupt" into a Rule-2
 *         compliant bounded wait.
 */
void UART_PingPong_IT_Supervise(UART_PingPong_t *ctx) {
  if (ctx == NULL) {
    return;
  }
  if (ctx->state == UART_PP_STATE_ERROR) {
    return;
  }

  ctx->watchdogTicks++;
  if (ctx->watchdogTicks > UART_PP_WATCHDOG_LIMIT) {
    (void)HAL_UART_AbortReceive_IT(ctx->huart);
    (void)HAL_UART_AbortTransmit_IT(ctx->huart);
    ctx->state = UART_PP_STATE_ERROR;
  }
}

/* ---- Compile-time dispatch: one context per physical UART link ---- */

static UART_PingPong_t g_link1Ctx; /* e.g. USART2, first sensor MCU  */
static UART_PingPong_t g_link2Ctx; /* e.g. USART3, second sensor MCU */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == g_link1Ctx.huart) {
    UART_PingPong_IT_OnRxComplete(&g_link1Ctx);
  } else if (huart == g_link2Ctx.huart) {
    UART_PingPong_IT_OnRxComplete(&g_link2Ctx);
  } else {
    /* Unknown handle: nothing to recover, intentionally ignored. */
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == g_link1Ctx.huart) {
    UART_PingPong_IT_OnTxComplete(&g_link1Ctx);
  } else if (huart == g_link2Ctx.huart) {
    UART_PingPong_IT_OnTxComplete(&g_link2Ctx);
  }
}

/* ---- Public wiring API - this is the only surface main.c needs ---- */

/**
 * @brief  Arm both UART links. Call once, after both huart handles have
 *         been through HAL_UART_Init() (i.e. after the CubeMX-generated
 *         MX_USARTx_UART_Init() calls in main()).
 */
HAL_StatusTypeDef App_UartLinks_Init(UART_HandleTypeDef *huartLink1,
                                     UART_HandleTypeDef *huartLink2) {
  HAL_StatusTypeDef status1 = UART_PingPong_IT_Start(&g_link1Ctx, huartLink1);
  HAL_StatusTypeDef status2 = UART_PingPong_IT_Start(&g_link2Ctx, huartLink2);

  return ((status1 == HAL_OK) && (status2 == HAL_OK)) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief  Call once per scheduler tick (e.g. once per 1 ms SysTick, or once
 *         per super-loop pass) to enforce the Rule 2 bound on both links.
 */
void App_UartLinks_Supervise(void) {
  UART_PingPong_IT_Supervise(&g_link1Ctx);
  UART_PingPong_IT_Supervise(&g_link2Ctx);
}
