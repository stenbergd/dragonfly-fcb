/******************************************************************************
 * @file    main.c
 * @author  Dragonfly
 * @version v. 1.0.0
 * @date    2015-08-12
 * @brief   Flight Control program for the Dragonfly quadcopter
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "receiver.h"
#include "fcb_sensors.h"
#include "gyroscope.h"
#include "fcb_error.h"

#include "stm32f3_discovery.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t UserButtonPressed;

/* Private function prototypes -----------------------------------------------*/

/* Called every system tick to drive the RTOS */
extern void xPortSysTickHandler(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  EXTI line detection callbacks
 * @param  GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == USER_BUTTON_PIN) {
		UserButtonPressed++;
		if (UserButtonPressed > 0x7) {
			BSP_LED_Toggle(LED7);
			UserButtonPressed = 0x0;
		}
	} else if (GPIO_Pin == GPIO_GYRO_DRDY) {
		FcbSendSensorMessageFromISR(FCB_SENSOR_GYRO_DATA_READY);
	}
}

/**
 * @brief  PWR PVD interrupt callback
 * @param  none
 * @retval none
 */
void HAL_PWR_PVDCallback(void) {
	/* Voltage drop detected - Go to error handler */
	ErrorHandler();
}

/**
 * @brief  SYSTICK callback
 * @param  None
 * @retval None
 */
void HAL_SYSTICK_Callback(void) {
#if 1

	static uint32_t kicks = 0;
	kicks++;

	if ((kicks % 1000) == 0) {
		BSP_LED_Toggle(LED9);
	}
#endif
	HAL_IncTick();

	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
		xPortSysTickHandler();
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == PRIMARY_RECEIVER_TIM) {
		PrimaryReceiverTimerPeriodCountIncrement();
	} else if (htim->Instance == AUX_RECEIVER_TIM) {
		AuxReceiverTimerPeriodCountIncrement();
	}
}

/**
 * @brief  Input Capture callback in non blocking mode
 * @param  htim : TIM IC handle
 * @retval None
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == PRIMARY_RECEIVER_TIM) {
		if (htim->Channel == PRIMARY_RECEIVER_THROTTLE_ACTIVE_CHANNEL)
			UpdateReceiverThrottleChannel();
		else if (htim->Channel == PRIMARY_RECEIVER_AILERON_ACTIVE_CHANNEL)
			UpdateReceiverAileronChannel();
		else if (htim->Channel == PRIMARY_RECEIVER_ELEVATOR_ACTIVE_CHANNEL)
			UpdateReceiverElevatorChannel();
		else if (htim->Channel == PRIMARY_RECEIVER_RUDDER_ACTIVE_CHANNEL)
			UpdateReceiverRudderChannel();
	} else if (htim->Instance == AUX_RECEIVER_TIM) {
		if (htim->Channel == AUX_RECEIVER_GEAR_ACTIVE_CHANNEL)
			UpdateReceiverGearChannel();
		else if (htim->Channel == AUX_RECEIVER_AUX1_ACTIVE_CHANNEL)
			UpdateReceiverAux1Channel();
	}
}

/* Private functions ---------------------------------------------------------*/

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/

