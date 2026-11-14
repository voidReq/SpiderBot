/**
 * @file   Main.cpp
 * @brief  Tests the Spider robot rc servo movements
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include "MMap.h"
#include "Motor.h"
#include "LEDControl.h"
#include "SpiderLeg.h"

using namespace std;

const int PAUSE = 200000;	// us between poses
const float POSES[][SpiderLeg::JOINT_NUM] = {
	// hip, knee, ankle, as def in SpiderLeg.h
	{-30, 30, 30},	// lift
	{ 30, 30, 30},	// swing left
	{ 30,  0,  0},	// lower
	{-30,  0,  15},	// push right
};
const int NUM_POSES = sizeof(POSES) / sizeof(POSES[0]);	// number of poses in the sequence

int main(int argc, char *argv[]){

	cout << "\nProgram Starting...!" << endl;
	MMap *m_map = new MMap();
	LEDControl *pio = new LEDControl(m_map);

	int legID = -1;	// start invalid so the loop asks at least once
	while (legID < 0 || legID > 5) {	// keep asking until the leg is 0-5
		cout << "Enter leg (0=RF 1=RM 2=RB 3=LF 4=LM 5=LB): ";
		if (!(cin >> legID)) {	// input wasn't a number
			cin.clear();			// reset cin so it can read again
			cin.ignore(256, '\n');	// throw away the bad input
			legID = -1;
		}
	}
	SpiderLeg *leg = new SpiderLeg(m_map, legID * 3, legID * 3 + 1, legID * 3 + 2);	// hip, knee, ankle IDs (Motor.h) 
											// *3 + n accounts for 3 motors / leg

	cout << "Stepping, power off to stop" << endl;
	while (true) {	// repeat the step sequence until the spider is powered off
		for (int p = 0; p < NUM_POSES; p++) {	// go through each pose in order
			while (!leg->IsReady()) usleep(1000); // don't PWM til all joints done
			for (int j = 0; j < SpiderLeg::JOINT_NUM; j++)	// set each joint (hip, knee, ankle) for this pose
				leg->MoveJoint((SpiderLeg::JOINT_ID)j, POSES[p][j]); // iter thru arr def ln 20
			usleep(PAUSE);	// pause before the next pose
		}
	}

	delete leg; // destruct
	delete pio;
	delete m_map;
	cout << "Terminating..." << endl;
	return 0;
}