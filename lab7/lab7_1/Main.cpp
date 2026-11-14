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

using namespace std;

const float STEP = 15.0;	// degrees per button press

int main(int argc, char *argv[]){

	cout << "\nProgram Starting...!" << endl;
	MMap *m_map = new MMap();
	LEDControl *pio = new LEDControl(m_map);

	int motorID = -1;	// start invalid so the loop asks at least once
	while (motorID < 0 || motorID > 17) {	// keep asking until the ID is 0-17
		cout << "Enter motor ID (0-17): ";
		if (!(cin >> motorID)) {	// input wasn't a number
			cin.clear();			// reset cin so it can read again
			cin.ignore(256, '\n');	// throw away the bad input
			motorID = -1;
		}
	}
	Motor *motor = new Motor(m_map, motorID);	// dynamic Motor object for the chosen servo, starts at 0 degrees

	cout << "KEY0 -15 deg, KEY1 +15 deg" << endl;
	while (true) {	// keep checking the buttons until the spider is powered off
		for (int key = 0; key < 2; key++) { // constantly check the two buttons "simultaneously" -- not
			if (!pio->ReadButton(key)) continue; // no press, recheck

			float step = (key == 0) ? -STEP : STEP; // if key0, decr, if key1, incr
			while (!motor->IsReady()) usleep(1000); // give it a sec, don't PWM til done
			motor->Move(motor->GetfAngle() + step);  // send cmd
			cout << "Angle: " << motor->GetfAngle() << endl;

			while (pio->ReadButton(key)) usleep(10000);	// wait for release of button
		}
		usleep(10000);	// short delay between button checks
	}

	delete motor; // destruct
	delete pio;
	delete m_map;
	cout << "Terminating..." << endl;
	return 0;
}