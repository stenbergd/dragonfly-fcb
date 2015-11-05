/*****************************************************************************
 * @file    pid_control.c
 * @author  Dragonfly
 * @version v. 1.0.0
 * @date    2015-08-31
 * @brief   File contains PID control algorithm implementation
 ******************************************************************************/

// TODO NOTE: Alot of code below is from the old project. It may be used as reference for further development.
// Eventually, it should be (re)moved and/or replaced or reimplemented. See real-time system implementation of PID.

/* Includes ------------------------------------------------------------------*/
#include "pid_control.h"

#include "state_estimation.h"
#include "flight_control.h"
#include "motor_control.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  float32_t K;					// PID gain parameter
  float32_t Ti;					// PID integration time parameter
  float32_t Td;					// PID derivative time parameter
  float32_t Tt;					// Anti-windup tracking parameter
  float32_t Beta;				// Set-point weighting 0-1
  float32_t Gamma;				// Derivative set-point weighting 0-1
  float32_t N;					// Derivative action filter constant
  float32_t P;					// Proportional control part
  float32_t I;					// Integration control part
  float32_t D;					// Derivative control part
  float32_t preState;			// Previous control state value
  float32_t preRef;				// Previous reference signal value
  float32_t upperSatLimit;		// Upper saturation limit of control signal
  float32_t lowerSatLimit;		// Lower saturation limit of control signal
  float32_t ctrlSignalScaling;	// Scaling of PID control signal
  float32_t ctrlSignalOffset;	// Static offset of PID control signal
  bool useIntegralAction;		// Sets if intergral action should be used or not
}PIDController_TypeDef;

/* Private define ------------------------------------------------------------*/
#define CONTROL_PERIOD			(float32_t) FLIGHT_CONTROL_TASK_PERIOD/1000.0

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static PIDController_TypeDef AltCtrl;
static PIDController_TypeDef RollCtrl;
static PIDController_TypeDef PitchCtrl;
static PIDController_TypeDef YawCtrl;

/* Private function prototypes -----------------------------------------------*/
static float32_t UpdatePIDControl(PIDController_TypeDef* ctrlParams, float32_t ctrlState, float32_t refSignal);

/* Exported functions --------------------------------------------------------*/

/*
 * @brief	Initializes the PID controllers, i.e. sets controller parameters.
 * @param	None.
 * @retval	None.
 */
void InitPIDControllers(void) {
	/* Initialize Altitude Controller */
	AltCtrl.K = K_VZ;
	AltCtrl.Ti = TI_VZ;
	AltCtrl.Td = TD_VZ;
	arm_sqrt_f32(AltCtrl.Ti*AltCtrl.Td, &AltCtrl.Tt); // Rule of thumb method to set this
	AltCtrl.Beta = BETA_VZ;
	AltCtrl.Gamma = GAMMA_VZ;
	AltCtrl.N = N_VZ;
	AltCtrl.P = 0.0;
	AltCtrl.I = 0.0;
	AltCtrl.D = 0.0;
	AltCtrl.preState = 0.0;
	AltCtrl.preRef = 0.0;
	AltCtrl.upperSatLimit = 0.0;
	AltCtrl.lowerSatLimit = -MAX_THRUST;	// Negative lower limit since Z points to earth
	AltCtrl.ctrlSignalScaling = MASS;		// Scale with mass to obtain thrust force
	AltCtrl.ctrlSignalOffset = -G_ACC;		// Gravity offset
	AltCtrl.useIntegralAction = false;

	/* Initialize Roll Controller */
	RollCtrl.K = K_RP;
	RollCtrl.Ti = TI_RP;
	RollCtrl.Td = TD_RP;
	arm_sqrt_f32(RollCtrl.Ti*RollCtrl.Td, &RollCtrl.Tt); // Rule of thumb method to set this
	RollCtrl.Beta = BETA_RP;
	RollCtrl.Gamma = GAMMA_RP;
	RollCtrl.N = N_RP;
	RollCtrl.P = 0.0;
	RollCtrl.I = 0.0;
	RollCtrl.D = 0.0;
	RollCtrl.preState = 0.0;
	RollCtrl.preRef = 0.0;
	RollCtrl.upperSatLimit = MAX_ROLLPITCH_MOM;
	RollCtrl.lowerSatLimit = -MAX_ROLLPITCH_MOM;
	RollCtrl.ctrlSignalScaling = IXX;
	RollCtrl.ctrlSignalOffset = 0.0;
	RollCtrl.useIntegralAction = false;

	/* Initialize Pitch Controller */
	PitchCtrl.K = K_RP;
	PitchCtrl.Ti = TI_RP;
	PitchCtrl.Td = TD_RP;
	arm_sqrt_f32(PitchCtrl.Ti*PitchCtrl.Td, &PitchCtrl.Tt); // Rule of thumb method to set this
	PitchCtrl.Beta = BETA_RP;
	PitchCtrl.Gamma = GAMMA_RP;
	PitchCtrl.N = N_RP;
	PitchCtrl.P = 0.0;
	PitchCtrl.I = 0.0;
	PitchCtrl.D = 0.0;
	PitchCtrl.preState = 0.0;
	PitchCtrl.preRef = 0.0;
	PitchCtrl.upperSatLimit = MAX_ROLLPITCH_MOM;
	PitchCtrl.lowerSatLimit = -MAX_ROLLPITCH_MOM;
	PitchCtrl.ctrlSignalScaling = IYY;
	PitchCtrl.ctrlSignalOffset = 0.0;
	PitchCtrl.useIntegralAction = false;

	/* Initialize Yaw Controller */
	YawCtrl.K = K_YR;
	YawCtrl.Ti = TI_YR;
	YawCtrl.Td = TD_YR;
	arm_sqrt_f32(YawCtrl.Ti*YawCtrl.Td, &YawCtrl.Tt); // Rule of thumb method to set this
	YawCtrl.Beta = BETA_YR;
	YawCtrl.Gamma = GAMMA_YR;
	YawCtrl.N = N_YR;
	YawCtrl.P = 0.0;
	YawCtrl.I = 0.0;
	YawCtrl.D = 0.0;
	YawCtrl.preState = 0.0;
	YawCtrl.preRef = 0.0;
	YawCtrl.upperSatLimit = MAX_YAW_MOM;
	YawCtrl.lowerSatLimit = -MAX_YAW_MOM;
	YawCtrl.ctrlSignalScaling = IZZ;
	YawCtrl.ctrlSignalOffset = 0.0;
	YawCtrl.useIntegralAction = false;
}

/*
 * @brief  Update PID control and set control signals
 * @param  None.
 * @retval None.
 */
void UpdatePIDControlSignals(CtrlSignals_TypeDef* ctrlSignals) {
// ctrlSignals->Thrust = UpdatePIDControl(&AltCtrl, GetZVelocity(), GetZVelocityReferenceSignal()); // TODO control altitude later
	ctrlSignals->RollMoment = UpdatePIDControl(&RollCtrl, GetRollAngle(), GetRollAngleReferenceSignal());
	ctrlSignals->PitchMoment = UpdatePIDControl(&PitchCtrl, GetPitchAngle(), GetPitchAngleReferenceSignal());
	ctrlSignals->YawMoment = UpdatePIDControl(&YawCtrl, GetYawAngle(), GetYawAngularRateReferenceSignal()); // TODO should be GetYawRate()
}

/* Private functions ---------------------------------------------------------*/

/*
 * @brief	Updates a PID control value based on current state and reference signal
 * @param	ctrlParams : Reference to PID parameter struct
 * @param	ctrlState : The control state value
 * @param	refSignal : The control reference signal value
 * @retval	PID control signal value
 */
static float32_t UpdatePIDControl(PIDController_TypeDef* ctrlParams, float32_t ctrlState, float32_t refSignal) {
	float32_t controlSignal, tmpControlSignal;

	/* Calculate Proportional control part */
	ctrlParams->P = ctrlParams->K*(ctrlParams->Beta*refSignal - ctrlState);

#if defined(PID_USE_CLASSIC_FORM)
	/* Classic form based on: u(t) = K*e(t) + K/Ti*integr(e(t)) + K*Td*deriv(e(t))
	 * */

	/* Calculate Integral control part */
	if(ctrlParams->Ti > 0.0) {
		ctrlParams->I += ctrlParams->K*CONTROL_PERIOD/ctrlParams->Ti*(refSignal - ctrlState);
	}

	/* Calculate Derivative control part */
	ctrlParams->D = ctrlParams->Td / (ctrlParams->Td + ctrlParams->N * CONTROL_PERIOD)*ctrlParams->D
			+ ctrlParams->K * ctrlParams->Td * ctrlParams->N / (ctrlParams->Td + ctrlParams->N * CONTROL_PERIOD )
			* (ctrlParams->Gamma * (refSignal - ctrlParams->preRef) - (ctrlState - ctrlParams->preState));

#elif defined(PID_USE_PARALLEL_FORM)
	/* Parallel form based on: u(t) = K*e(t) + Ti*integr(e(t)) - Td*deriv(y(t))
	 * */

	/* Calculate Integral control part */
	if(ctrlParams->useIntegralAction) {
		ctrlParams->I += ctrlParams->Ti*CONTROL_PERIOD*(refSignal - ctrlState);
	}

	/* Calculate Derivative control part */
	ctrlParams->D = ctrlParams->Td/(ctrlParams->Td + ctrlParams->N * CONTROL_PERIOD)*ctrlParams->D
			+ ctrlParams->Td*ctrlParams->N/(ctrlParams->Td + ctrlParams->N * CONTROL_PERIOD)
			* (ctrlParams->Gamma*(refSignal - ctrlParams->preRef) - (ctrlState - ctrlParams->preState));

#endif

	/* Calculate sum of P-I-D parts to obtain the control signal and add offset part. Multiply with scaling factor. */
	controlSignal = (ctrlParams->P + ctrlParams->I + ctrlParams->D + ctrlParams->ctrlSignalOffset) * ctrlParams->ctrlSignalScaling;

	/* Saturate controller output */
	tmpControlSignal = controlSignal;
	if (controlSignal < ctrlParams->lowerSatLimit) {
		controlSignal = ctrlParams->lowerSatLimit;
	} else if (controlSignal > ctrlParams->upperSatLimit) {
		controlSignal = ctrlParams->upperSatLimit;
	}

	/* Perform back calculation anti-windup scheme to avoid integral part windup */
	if(ctrlParams->useIntegralAction) {
		ctrlParams->I += CONTROL_PERIOD/ctrlParams->Tt*(tmpControlSignal-controlSignal);
	}

	/* Update previous control state and reference signal variables */
	ctrlParams->preState = ctrlState;
	ctrlParams->preRef = refSignal;

	return controlSignal;
}

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/
