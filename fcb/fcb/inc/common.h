/******************************************************************************
 * @file    fcb/common.c
 * @author  �F Dragonfly
 * @version v. 1.0.0
 * @date    2015-06-09
 * @brief   Header file for common functions
 ******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMON_H
#define __COMMON_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

/* Exported function prototypes --------------------------------------------- */
uint32_t Calculate_CRC(const uint8_t* dataBuffer, const uint32_t dataBufferSize);

#endif /* __COMMON_H */

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/