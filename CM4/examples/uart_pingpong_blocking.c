/**
 * @file    uart_pingpong_blocking.c
 * @brief   Blocking (polled) UART ping-pong: receive 10 bytes, echo 10 bytes.
 *
 * Power-of-10 compliance notes:
 *  Rule 1 - single straight-line control path, no goto/recursion.
 *  Rule 2 - HAL_UART_Receive/Transmit take an explicit Timeout, so the
 *           internal wait loop has a provable upper bound (see HAL source:
 *           UART_WaitOnFlagUntilTimeout). The outer for(;;) is the one
 *           deliberately non-terminating loop a scheduler task is allowed
 *           to have.
 *  Rule 3 - buffer is a fixed-size stack array, no malloc/free anywhere.
 *  Rule 5 - two assertions per function (parameter + state), each with an
 *           explicit recovery action (return an error code).
 *  Rule 6 - buffer and status are declared at the narrowest scope in which
 *           they are used.
 *  Rule 7 - every HAL return value is checked and propagated.
 *  Rule 9 - pointers are single-level (uint8_t *), no function pointers.
 */

#include "stm32h7xx_hal.h"

#define UART_PINGPONG_LEN     10U
#define UART_RX_TIMEOUT_MS    100U
#define UART_TX_TIMEOUT_MS    50U

/**
 * @brief  Perform one bounded receive-then-echo round trip on a UART link.
 * @param  huart  Initialized UART handle for this link.
 * @retval HAL_OK on a completed round trip, otherwise the HAL error that
 *         stopped it (HAL_ERROR, HAL_BUSY or HAL_TIMEOUT).
 *
 * Worst-case blocking time is bounded: UART_RX_TIMEOUT_MS + UART_TX_TIMEOUT_MS.
 * While this call is blocked, no other peripheral on this core can be
 * serviced - see the discussion on when this is/isn't acceptable.
 */
HAL_StatusTypeDef UART_PingPong_Blocking(UART_HandleTypeDef *huart)
{
    uint8_t buffer[UART_PINGPONG_LEN];
    HAL_StatusTypeDef status;

    if (huart == NULL)                                   /* assertion 1 */
    {
        return HAL_ERROR;
    }
    if (HAL_UART_GetState(huart) == HAL_UART_STATE_RESET) /* assertion 2 */
    {
        return HAL_ERROR;
    }

    status = HAL_UART_Receive(huart, buffer, UART_PINGPONG_LEN, UART_RX_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return status;                                    /* Rule 7 */
    }

    return HAL_UART_Transmit(huart, buffer, UART_PINGPONG_LEN, UART_TX_TIMEOUT_MS);
}

/**
 * @brief  Scheduler task wrapper. Deliberately non-terminating (Rule 2
 *         exception for scheduler loops) - the loop condition is a
 *         compile-time constant, so a tool can prove it never exits by
 *         itself, which is the required proof obligation for this class
 *         of loop.
 */
void UART_PingPong_Task(UART_HandleTypeDef *huart)
{
    for (;;)
    {
        if (UART_PingPong_Blocking(huart) != HAL_OK)
        {
            /* Explicit recovery action (Rule 5/7): release the peripheral
               so the next iteration starts from a clean state instead of
               retrying into a wedged transfer. */
            (void)HAL_UART_AbortReceive(huart);
            (void)HAL_UART_AbortTransmit(huart);
        }
    }
}
