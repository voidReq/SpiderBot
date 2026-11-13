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

	int motorID = -1;
	while (motorID < 0 || motorID > 17) {
		cout << "Enter motor ID (0-17): ";
		if (!(cin >> motorID)) {
			cin.clear();
			cin.ignore(256, '\n');
			motorID = -1;
		}
	}
	Motor *motor = new Motor(m_map, motorID);

	cout << "KEY0 -15 deg, KEY1 +15 deg" << endl;
	while (true) {
		for (int key = 0; key < 2; key++) { // constantly check the two buttons "simultaneously" -- not
			if (!pio->ReadButton(key)) continue; // no press, recheck

			float step = (key == 0) ? -STEP : STEP; // if key0, decr, if key1, incr
			while (!motor->IsReady()) usleep(1000); // give it a sec, don't PWM til done
			motor->Move(motor->GetfAngle() + step);  // send cmd
			cout << "Angle: " << motor->GetfAngle() << endl;

			while (pio->ReadButton(key)) usleep(10000);	// wait for release of button
		}
		usleep(10000);
	}

	delete motor; // destruct
	delete pio;
	delete m_map;
	cout << "Terminating..." << endl;
	return 0;
}
