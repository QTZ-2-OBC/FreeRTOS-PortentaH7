/**
 * @file    i2c_pingpong_blocking.c
 * @brief   Blocking I2C ping-pong with the central MCU acting as I2C SLAVE
 *          (a separate MCU is the I2C master): receive 10 bytes, echo
 *          10 bytes back on the next read.
 *
 * Power-of-10 notes mirror uart_pingpong_blocking.c:
 *  Rule 2 - bounded by explicit Timeout arguments.
 *  Rule 3 - fixed-size stack buffer, no heap.
 *  Rule 5/7 - two checked assertions + every HAL return value checked.
 *  Rule 9 - single-level pointers only.
 *
 * Important I2C-specific caveat: as a slave, this MCU cannot decide when
 * the transaction happens - the external master drives the clock and
 * initiates both the write (host -> us) and the following read (us ->
 * host). HAL_I2C_Slave_Receive/Transmit block until the master actually
 * performs its side or the timeout elapses, so a slow/silent master will
 * hold this call for up to the full timeout on every cycle.
 */

#include "stm32h7xx_hal.h"

#define I2C_PINGPONG_LEN     10U
#define I2C_RX_TIMEOUT_MS    100U
#define I2C_TX_TIMEOUT_MS    50U

/**
 * @brief  One bounded slave receive-then-echo round trip.
 * @param  hi2c  Initialized I2C handle configured in slave mode.
 * @retval HAL_OK on success, otherwise the HAL error/timeout code.
 */
HAL_StatusTypeDef I2C_PingPong_Blocking(I2C_HandleTypeDef *hi2c)
{
    uint8_t buffer[I2C_PINGPONG_LEN];
    HAL_StatusTypeDef status;

    if (hi2c == NULL)                                      /* assertion 1 */
    {
        return HAL_ERROR;
    }
    if (HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_RESET)      /* assertion 2 */
    {
        return HAL_ERROR;
    }

    status = HAL_I2C_Slave_Receive(hi2c, buffer, I2C_PINGPONG_LEN, I2C_RX_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return status;                                      /* Rule 7 */
    }

    return HAL_I2C_Slave_Transmit(hi2c, buffer, I2C_PINGPONG_LEN, I2C_TX_TIMEOUT_MS);
}

/**
 * @brief  Scheduler task wrapper - deliberately non-terminating.
 */
void I2C_PingPong_Task(I2C_HandleTypeDef *hi2c)
{
    for (;;)
    {
        if (I2C_PingPong_Blocking(hi2c) != HAL_OK)
        {
            /* A stuck I2C slave transaction is a bus-level condition, not
               just a peripheral flag - re-arming alone is often not
               enough if SDA/SCL are wedged. Re-init here as the explicit
               recovery action; add a GPIO-level bus-recovery sequence
               (9 clocks + STOP) before HAL_I2C_Init if this fires often. */
            (void)HAL_I2C_DeInit(hi2c);
            (void)HAL_I2C_Init(hi2c);
        }
    }
}
