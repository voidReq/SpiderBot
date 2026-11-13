 /**
 * @file   Motor.cpp
 * @Author Richard, Modified by: John Kimani (j.kimani@northeastern.edu)
 * @date   Modified on: 11/01/2025
 * @brief  Process an RC Serco motor in the Terasic Spider
 *
 * Implements the Motor class that provides functions to operate
 * on the various rc servos on the Terasic Spider robot
 */

#include "Motor.h"
#include "RegisterMap.h"
#include <math.h>
#include <iostream>
using namespace std;

/*
*	Constructor initializes the specified servo
*	with defualt values
*/
Motor::Motor(MMap* mio, int MonotrID)
{
	m_nMotorID = MonotrID;		// Motor ID (0-17) specifies rc servo to test
	motor_angle = 0.0;			// Angle [-90 to 90] degrees. 0.0 is the defualt
	motor_speed = DELAY_MAX; 	// Actual delay in cycles. Higher delay ==> slow speed
	mmio = mio;					// Memory map object for register access

	uint32_t duty_cycle = 75000;	// Neutral 0.0 degrees
	// 	Setup the PWM period, default (duty cycle=75000), default speed=1700 cycles
	mmio->RegisterWrite((szPWM_Base[m_nMotorID] + REG_TOTAL_DUR), PWM_PERIOD);
	mmio->RegisterWrite((szPWM_Base[m_nMotorID] + REG_HIGH_DUR), duty_cycle);
	mmio->RegisterWrite((szPWM_Base[m_nMotorID] + REG_ADJ_SPEED), motor_speed);
	mmio->RegisterWrite((szPWM_Base[m_nMotorID] + REG_ABORT), 0);	// wake up mode
}

/**
*	Destructor
*/
Motor::~Motor() {
	// Stop sending PWM
	mmio->RegisterWrite((szPWM_Base[m_nMotorID] + REG_ABORT), 1);	// Disable PWM signal
}

/**
 * Puts the motor into the neutral, angle 0 position by
 * calling the specific function Move(...).
 */
void Motor::Reset(void){
	Move(0.0);
}

/**
 * @return true when this motor is prepared to receive
 * a new SetSpeed or Move method call; otherwise false.
*/
bool Motor::IsReady(void)
{
	return mmio->RegisterRead(szPWM_Base[m_nMotorID] + REG_ADJ_STATUS) & 0x1;
}

/**
 *	Check the current motor angle
 */
float Motor::GetfAngle(void)
{
	return motor_angle;
}

/**
 * Compute an appropriate delay value based on the given speed.
 * @param speed - a number between SPEED_MIN and SPEED_MAX,
 * Limit the angle to SPEED_MAX or SPEED_MIN if out of bounds
 * Compute motor_speed with your algorithm before writing to register
 */
void Motor::SetSpeed(int speed)
{
	if (speed > SPEED_MAX) speed = SPEED_MAX;
	if (speed < SPEED_MIN) speed = SPEED_MIN;

	motor_speed = DELAY_MAX - (DELAY_MAX - DELAY_MIN) * speed / SPEED_MAX; // prelab 2: 1700-(700)*speed/100
	mmio->RegisterWrite(szPWM_Base[m_nMotorID] + REG_ADJ_SPEED, motor_speed);
}

/**
 *	Return the current motor speed
 */
uint32_t Motor::GetSpeed(void)
{
	return motor_speed;
}

/**
 *	Move motor to the specified angle. First...
 * @param fAngle - a number between DEGREE_MIN and DEGREE_MAX,
 *  Limit the angle to DEGREE_MAX or DEGREE_MIN if out of bounds
 *  If angle = -0.0 then set it to 0.0
 *  If controlling right side motors, set fAngle = -fAngle
 *  Compute PWM duty cycle with your algorithm before writing to register
 */
void Motor::Move(float fAngle) {
	if (fAngle > DEGREE_MAX) fAngle = DEGREE_MAX;
	if (fAngle < DEGREE_MIN) fAngle = DEGREE_MIN;
	if (fAngle == 0.0f) fAngle = 0.0f;	// -0.0 -> 0.0
	motor_angle = fAngle;

	if (m_nMotorID < 9) fAngle = -fAngle;	// right legs are mirrored

	uint32_t duty_cycle = PWM_MIN + (fAngle - DEGREE_MIN) * (PWM_MAX - PWM_MIN) / (DEGREE_MAX - DEGREE_MIN); // prelab eq 1)ii
		// 25k+(ang+90)*(125k-25k)/(90--90)
	mmio->RegisterWrite(szPWM_Base[m_nMotorID] + REG_HIGH_DUR, duty_cycle); // write cycle to regs
}
