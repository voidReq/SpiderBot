#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

//using namespace std so std:: doesn't need to be written before cout and endl
using namespace std;

// Physical base address of FPGA Devices
const unsigned int LW_BRIDGE_BASE = 0xFF200000;  // Base offset

// Length of memory-mapped IO window
const unsigned int LW_BRIDGE_SPAN = 0x00005000;  // Address map size

// Cyclone V FPGA device addresses
const unsigned int LEDR_BASE = 0x00000000;  // Leds offset
const unsigned int SW_BASE   = 0x00000040;  // Switches offset
const unsigned int KEY_BASE  = 0x00000050;  // Push buttons offset

/** 
 * Initialize general-purpose I/O 
 *  - Opens access to physical memory /dev/mem 
 *  - Maps memory into virtual address space 
 * 
 * @param fd	File descriptor passed by reference, where the result 
 *			of function 'open' will be stored. 
 * @return	Address to virtual memory which is mapped to physical, or MAP_FAILED on error. 
  */ 
char *Initialize(int *fd)
{
    // Open /dev/mem to give access to physical addresses
    *fd = open( "/dev/mem", (O_RDWR | O_SYNC));
    if (*fd == -1)			//  check for errors in openning /dev/mem
    {
        cout << "ERROR: could not open /dev/mem..." << endl;
        exit(1);
    }

    // Get a mapping from physical addresses to virtual addresses
    char *virtual_base = (char *)mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, *fd, LW_BRIDGE_BASE);
    if (virtual_base == MAP_FAILED)		// check for errors
    {
        cout << "ERROR: mmap() failed..." << endl;
        close (*fd);		// close memory before exiting
        exit(1);        // Returns 1 to the operating system;
    }
    return virtual_base;
}

/** 
 * Close general-purpose I/O. 
 * 
 * @param pBase	Virtual address where I/O was mapped. 
 * @param fd	File descriptor previously returned by 'open'. 
 */ 
void Finalize(char *pBase, int fd)
{
    if (munmap (pBase, LW_BRIDGE_SPAN) != 0)
    {
        cout << "ERROR: munmap() failed..." << endl;
        exit(1);
    }
    close (fd); 	// close memory
}

/** 
 * Write a 4-byte value at the specified general-purpose I/O location. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @parem offset	Offset where device is mapped. 
 * @param value	Value to be written. 
 */ 
void RegisterWrite(char *pBase, unsigned int reg_offset, int value)
{
    *(volatile unsigned int *)(pBase + reg_offset) = value;
}

/** 
 * Read a 4-byte value from the specified general-purpose I/O location. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @param offset	Offset where device is mapped. 
 * @return		Value read. 
 */ 
int RegisterRead(char *pBase, unsigned int reg_offset)
{
    return *(volatile unsigned int *)(pBase + reg_offset);
}

/** 
 * Read the state of one switch. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @param switchNum	Switch to read (0-9). 
 * @return		1 if the switch is on, 0 if off, or -1 if switchNum is invalid. 
 */ 
int Read1Switch(char *pBase, int switchNum){
    if (switchNum < 0 || switchNum > 9){	// check for an invalid switch number
        return -1;
    }

    int allSwitches = RegisterRead(pBase, SW_BASE);
    return (allSwitches >> switchNum) & 0x1; //shifts the wanted switch down to bit 0 and ANDs with 1 to only keep that bit
}

/** 
 * Turn one LED on or off without changing the other LEDs. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @param ledNum	LED to change (0-9). 
 * @param state		State to write (0 = off, 1 = on). 
 */ 
void Write1Led(char *pBase, int ledNum, int state){
    int curLeds = RegisterRead(pBase, LEDR_BASE);	// read current LEDs so the others are kept

    if(state == 1){
        curLeds |= (1 << ledNum);	// OR with the mask to turn the bit on
    } else {
        curLeds &= ~(1 << ledNum);	// AND with the inverted mask to turn the bit off
    }
    RegisterWrite(pBase, LEDR_BASE, curLeds);
}

/** 
 * Read the state of all 10 switches as one value. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @return		Value of the switches (0-1023). 
 */ 
int ReadAllSwitches(char *pBase){
    return RegisterRead(pBase, SW_BASE) & 0x3FF; //ANDs with 0x3FF (1111111111) to only keep the 10 switch bits, giving 0-1023
}

/** 
 * Write a value to all 10 LEDs. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @param value		Value to be written (0-1023). 
 */ 
void WriteAllLeds(char *pBase, int value){
    RegisterWrite(pBase, LEDR_BASE, value & 0x3FF);	// keep only the 10 LED bits
}

/** 
 * Read the push buttons. 
 * 
 * @param pBase		Base address returned by 'mmap'. 
 * @return		0-3 for KEY0-KEY3, 4 if more than one is pressed, or -1 if none. 
 */ 
int PushButtonGet(char *pBase) {
    int keys = RegisterRead(pBase, KEY_BASE) & 0x0F; // lower 4 bits
    if (keys == 0) return -1;	// no buttons pressed

    // Count how many buttons are down
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if ((keys >> i) & 1) count++; //shifts button i down to bit 0, and adds 1 to count if it's pressed
    }
    if (count > 1) return 4; // Multiple buttons pressed

    if (keys == 1) return 0; // KEY0
    if (keys == 2) return 1; // KEY1
    if (keys == 4) return 2; // KEY2
    if (keys == 8) return 3; // KEY3

    return -1;
}

/*
* Main Function
*/
int main()
{
    // Initialize
    int fd;		// file descriptor for /dev/mem
    char *pBase = Initialize(&fd);	// map the memory and get the base address

    cout << "\ninit counter" << endl;
    cout << "KEY0: incr | KEY1: decr | KEY2: rshift | KEY3: lshift | Multi: rst to siwtches" << endl;

    // init value from current switch states
    int counter = ReadAllSwitches(pBase);	// start the counter at the switch values
    WriteAllLeds(pBase, counter);
    cout << "Initial counter: " << counter << endl;

    int lastBtn = -1;	// last button state, used so each press only counts once

    while (true) {
        int btn = PushButtonGet(pBase);

        // only trigger once per press
        if (btn != lastBtn) {
            if (btn != -1 && (lastBtn == -1 || btn == 4)) {	// new press, or multiple buttons
                if (btn == 0) {
                    counter = (counter + 1) % 1024;               // increment & rollover
                } else if (btn == 1) {
                    counter = (counter == 0) ? 1023 : counter - 1; // decrement & rollover
                } else if (btn == 2) {
                    counter = (counter >> 1) & 0x3FF;              // shift right, insert 0
                } else if (btn == 3) {
                    counter = (counter << 1) & 0x3FF;              // shift left, insert 0
                } else if (btn == 4) {
                    counter = ReadAllSwitches(pBase);              // reset to switches
                }

                cout << "Counter: " << counter << endl;
                WriteAllLeds(pBase, counter);
            }
            lastBtn = btn;
        }

        usleep(20000); // 20ms debounce
    }

    // Done/kill
    Finalize(pBase, fd);
}
