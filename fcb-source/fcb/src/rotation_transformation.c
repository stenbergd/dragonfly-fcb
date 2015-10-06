/******************************************************************************
 * @file    rotation_transformation.c
 * @brief   Functions to keep track of coordinate system representations and
 * 			transformations between world and body frames. The transformations
 * 			are based on Euler angle rotations (Z-Y-X / roll-pitch-yaw) and
 * 			rotation matrices.
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "rotation_transformation.h"

#include <math.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
arm_matrix_instance_f32 DCM;
arm_matrix_instance_f32 DCMInv;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/*
 * @brief  Initializes the Direction Cosine Matrix
 * @param  None
 * @retval None
 */
void InitRotationMatrix(void) {
	float32_t DCMInvInit[9] =
	{
			1,     0,     1,
			0,     1,     0,
			0,     0,     1,
	};

	/* Initialize the inverse DCM to the unit matrix (3x3) */
	arm_mat_init_f32(&DCMInv, 3, 3, DCMInvInit);

	/* Initializes the DCM to the unit matrix (3x3) */
	UpdateRotationMatrix(0.0, 0.0, 0.0);
}

/*
 * @brief  Updates the Direction Cosine Matrix
 * @param  roll : roll angle in radians
 * @param  pitch : pitch angle in radians
 * @param  yaw : yaw angle in radians
 * @retval None
 */
void UpdateRotationMatrix(const float32_t roll, const float32_t pitch, const float32_t yaw) {
	/* Calculate the DCM based on roll, pitch and yaw angles */
	float32_t DCMUpdate[9] =
	{
			arm_cos_f32(pitch)*arm_cos_f32(yaw),     arm_cos_f32(pitch)*arm_sin_f32(yaw),     -arm_sin_f32(pitch),
			-arm_cos_f32(roll)*arm_sin_f32(yaw)+arm_sin_f32(roll)*arm_sin_f32(pitch)*arm_cos_f32(yaw),     arm_cos_f32(roll)*arm_cos_f32(yaw)+arm_sin_f32(roll)*arm_sin_f32(pitch)*arm_sin_f32(yaw),     arm_sin_f32(roll)*arm_cos_f32(pitch),
			arm_sin_f32(roll)*arm_sin_f32(yaw)+arm_cos_f32(roll)*arm_sin_f32(pitch)*arm_cos_f32(yaw),     -arm_sin_f32(roll)*arm_cos_f32(yaw)+arm_cos_f32(roll)*arm_sin_f32(pitch)*arm_sin_f32(yaw),     arm_cos_f32(roll)*arm_cos_f32(pitch),
	};

	/* Init the DCM that transforms FROM the inertial frame TO the body frame */
	arm_mat_init_f32(&DCM, 3, 3, DCMUpdate);

	/* Calculate the DCM inverse, which is the same as matrix transpose since DCM is an orthonormal matrix. The inverse
	 * transforms FROM the body frame TO the inertial frame*/
	arm_mat_trans_f32(&DCM, &DCMInv);
}

/*
 * @brief  Calculates the attitude (roll, pitch, yaw angles) based on magnetometer input
 * @param  dstAttitude : Destination vector in which to store the calculated attitude (roll, pitch, yaw)
 * @param  bodyMagneticReadings : The magnetometer sensor readings in the UAV body-frame
 * @param  inertialMagneticVector : The magnetic flux vector in the inertial frame
 * @retval None
 */
void GetAttitudeFromMagnetometer(float32_t* dstAttitude, float32_t* bodyMagneticReadings,
		float32_t* inertialMagneticVector) {

	/* Calculate the axis/angle representing the rotation from inertial-frame magnetic field to the body-frame sensor
	 * readings. NOTE: Inertial magnetic field vector depends on where on earth UAV is operating - Malmö, SE assumed.
	 * */

	float32_t bodyMagneticNormalized[3];
	float32_t inertialMagneticNormalized[3];
	float32_t rotationAxisVector[3];
	float32_t rotationAngle;
	float32_t dotProd;

	/* Get unit length normalized versions of the vectors */
	Vector3DNormalize(bodyMagneticNormalized, bodyMagneticReadings);
	Vector3DNormalize(inertialMagneticNormalized, inertialMagneticVector);

	/* Get the rotation axis vector between the two magnetic vectors (inertial and body) */
	Vector3DCrossProduct(rotationAxisVector, bodyMagneticNormalized, inertialMagneticNormalized);

	/* Get the angle for the body to the inertial frame vectors rotated around rotation vector axis */
	arm_dot_prod_f32(bodyMagneticNormalized, inertialMagneticNormalized, 3, &dotProd);
	rotationAngle = acos(dotProd);

	/* Calculate rotation matrix elements formed by axis/angle representation */
	float32_t cosAngle = arm_cos_f32(rotationAngle);
	float32_t sinAngle = arm_sin_f32(rotationAngle);
	float32_t r11 = cosAngle + rotationAxisVector[0]*rotationAxisVector[0]*(1.0-cosAngle);
	float32_t r12 = rotationAxisVector[0]*rotationAxisVector[1]*(1.0-cosAngle) - rotationAxisVector[2]*sinAngle;
	float32_t r13 = rotationAxisVector[0]*rotationAxisVector[2]*(1.0-cosAngle) + rotationAxisVector[1]*sinAngle;
	float32_t r23 = rotationAxisVector[1]*rotationAxisVector[2]*(1.0-cosAngle) - rotationAxisVector[0]*sinAngle;
	float32_t r33 = cosAngle + rotationAxisVector[2]*rotationAxisVector[2]*(1.0-cosAngle);

	/* Extract Euler angles from rotation matrix elements */
	dstAttitude[0] = atan2(r23, r33);
	dstAttitude[1] = asin(-r13);
	dstAttitude[2] = atan2(r12, r11);

	// TODO Init inertialMagneticVector somewhere at startup
	// TODO adjust for difference between magnetic and geographic north pole? Should this done here?
}

/*
 * @brief  Calculates the cross product of two 3D vectors and stores the result in a third
 * @param  dstVector : Destination vector
 * @param  srcVector1 : Source vector 1
 * @param  srcVector2 : Source vector 2
 * @retval None
 */
void Vector3DCrossProduct(float32_t* dstVector, const float32_t* srcVector1, const float32_t* srcVector2) {
	dstVector[0] = srcVector1[1]*srcVector2[2] - srcVector1[2]*srcVector2[1];
	dstVector[1] = srcVector1[2]*srcVector2[0] - srcVector1[0]*srcVector2[2];
	dstVector[2] = srcVector1[0]*srcVector2[1] - srcVector1[1]*srcVector2[0];;
}

/*
 * @brief  Normalizes the input 3D vector to unit length
 * @param  dstVector : normalized vector result
 * @param  srcVector : vector to be normalized
 * @retval None
 */
void Vector3DNormalize(float32_t* dstVector, float32_t* srcVector) {
	float32_t norm;

	/* Calculate norm of source vector */
	arm_dot_prod_f32(srcVector, srcVector, 3, &norm);
	norm = sqrt(norm);

	/* Normalize the vector to unit length and store it in destination vector */
	arm_scale_f32(srcVector, 1.0/norm, dstVector, 3);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/
