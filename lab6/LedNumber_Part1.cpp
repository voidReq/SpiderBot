/**
 * reads the state of one switch and returns 0 or 1 depending on if it's on or off
 *
 * @param   pBase      base address returned by mmap
 * @param   switchNum  which switch to read (0-9)
 * @return  int        1 if the switch is on, 0 if it's off, -1 if switchNum is invalid
 */
int Read1Switch(char *pBase, int switchNum){ 
    if (switchNum < 0 || switchNum > 9){ //checks if the switch number is outside the valid range 0-9
        return -1; //returns -1 to show the switch number was invalid
    }

    int allSwitches = RegisterRead(pBase, SW_BASE); //reads the state of all 10 switches at once
    return (allSwitches >> switchNum) & 0x1; //shifts the wanted switch down to bit 0 and ANDs with 1 to only keep that bit
}

/**
 * turns one LED on or off without changing the rest of the LEDs
 *
 * @param   pBase   base address returned by mmap
 * @param   ledNum  which LED to change (0-9)
 * @param   state   what to set the LED to (0 for off, 1 for on)
 * @return  none    does not return anything
 */
void Write1Led(char *pBase, int ledNum, int state){
    int curLeds = RegisterRead(pBase, LEDR_BASE); //reads the current state of all the LEDs so the others don't get changed

    if(state == 1){ //if state is 1, turns the LED on
        curLeds |= (1 << ledNum); //ORs with a mask at ledNum to turn only that bit on
    } else { //if state is 0, turns the LED off
        curLeds &= ~(1 << ledNum); //ANDs with the inverted mask to turn only that bit off
    }
    RegisterWrite(pBase, LEDR_BASE, curLeds); //writes the updated value back to the LEDs
}

int main(){
	// Initialize
	int fd;
	char *pBase = Initialize(&fd); 

	int value = 0;

	cout << "Enter LED to read (0..9): " << endl;
	cin >> value;
	int res = Read1Switch(pBase, value);
	cout << "Val of switch: " << res << endl;
	cout << "Enter LED to write (0..9): " << endl;
	cin >> value;
	cout << "Enter LED state (0 or 1): " << endl; cin >> res;
	Write1Led(pBase, value, res);
	cout << "Written" << endl;
	Finalize(pBase, fd);
}

