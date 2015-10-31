/****************************************************************************
* @file    state_estimation.h
* @brief   Header file for state_estimation.c
*****************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSORS_H
#define __SENSORS_H

/* Includes ------------------------------------------------------------------*/
#include "arm_math.h"
#include "usbd_cdc_if.h"
#include "flight_control.h"
#include "fcb_retval.h"
#include "common.h"

/* Exported types ------------------------------------------------------------*/
typedef struct KalmanFilter
{
	float32_t q1;	// Process noise covariance matrix components
	float32_t q2;
	float32_t r1;	// Measurement noise covariance matrix component
	float32_t p11;	// Error covariance matrix component
	float32_t p12;
	float32_t p21;
	float32_t p22;
	float32_t k1;	// Kalman gain
	float32_t k2;   // note: k1 & k2 these don't have to be part of the struct as they
	                // need not be carried over from one iteration to the next
	                // but it's useful for displaying variables.
} KalmanFilterType;

/**
 * This is used for roll, pitch & yaw attitude.
 */
typedef struct AttitudeStateVector
{
  float32_t angle; /* for yaw, think "heading" */
  float32_t angleRate; /* not used */
  float32_t angleRateBias;
} AttitudeStateVectorType;

/* Exported constants --------------------------------------------------------*/

// TODO we need separate values for roll pitch and yaw as well as separate init values of P matrix
#define	STATE_ESTIMATION_SAMPLE_PERIOD	(float32_t) 	FLIGHT_CONTROL_TASK_PERIOD / 1000.0
#define Q1_CAL (float32_t)								0.05
#define	Q2_CAL (float32_t)								0.005
#define	R1_CAL (float32_t)								0.000185 /* 480 measured from USB console and
                                                          * calculated with SensorVariance.sce
                                                          */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
float32_t GetRollAngle(void);
float32_t GetPitchAngle(void);
float32_t GetYawAngle(void);
float32_t GetHeading(void);

void InitStatesXYZ(void);
void PredictStatesXYZ(const float32_t sensorRateRoll, const float32_t sensorRatePitch, const float32_t sensorRateYaw);
void CorrectStatesXYZ(const float32_t sensorAngleRoll, const float32_t sensorAnglePitch, const float32_t sensorAngleYaw);
FcbRetValType StartStateSamplingTask(const uint16_t sampleTime, const uint32_t sampleDuration);
FcbRetValType StopStateSamplingTask(void);
void SetStatePrintSamplingSerialization(const SerializationType serializationType);
void PrintStateValues(const SerializationType serializationType);

#endif /* __SENSORS_H */

/* **** END OF FILE ****/
