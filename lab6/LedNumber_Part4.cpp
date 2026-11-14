// Name: Georgy Rached and James Hanstedt
// Date: November 13, 2026
// EECE 2160 Lab 6 - Assignment 4: PushButtonClass.cpp
// Compile: g++ PushButtonClass.cpp -o PushButtonClass
// Run: ./PushButtonClass

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

//using namespace std so std:: doesn't need to be written before cout and endl
using namespace std;

// Physical base address of FPGA Devices 
const unsigned int LW_BRIDGE_BASE = 0xFF200000;	// Base offset
const unsigned int LW_BRIDGE_SPAN = 0x00005000;	// Address map size

// Cyclone V FPGA device addresses
const unsigned int LEDR_BASE      = 0x00000000;	// Leds offset
const unsigned int SW_BASE        = 0x00000040;	// Switches offset
const unsigned int KEY_BASE       = 0x00000050;	// Push buttons offset

/** 
 * Class that handles access to the DE1-SoC FPGA memory. 
 * Maps the physical I/O addresses into virtual memory when created, 
 * and unmaps them when destroyed. 
 */ 
class DE1SoCfpga
{
    char *pBase;	// Virtual base address returned by 'mmap'
    int fd;		// File descriptor for /dev/mem

public:
    /** 
     * Constructor: initialize general-purpose I/O 
     *  - Opens access to physical memory /dev/mem 
     *  - Maps memory into virtual address space 
     */ 
    DE1SoCfpga()
    {
        // Open /dev/mem to give access to physical addresses
        fd = open("/dev/mem", (O_RDWR | O_SYNC));
        if (fd == -1)		// check for errors in opening /dev/mem
        {
            cout << "ERROR: could not open /dev/mem..." << endl;
            exit(1);
        }

        // Get a mapping from physical addresses to virtual addresses
        pBase = (char *)mmap(NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
        if (pBase == MAP_FAILED)	// check for errors
        {
            cout << "ERROR: mmap() failed..." << endl;
            close(fd);		// close memory before exiting
            exit(1);
        }
    }

    /** 
     * Destructor: close general-purpose I/O. 
     * Unmaps the memory and closes /dev/mem. 
     */ 
    ~DE1SoCfpga()
    {
        if (munmap(pBase, LW_BRIDGE_SPAN) != 0)
        {
            cout << "ERROR: munmap() failed..." << endl;
            exit(1);
        }
        close(fd);	// close memory
    }

    /** 
     * Write a 4-byte value at the specified general-purpose I/O location. 
     * 
     * @param offset	Offset where device is mapped. 
     * @param value	Value to be written. 
     */ 
    void RegisterWrite(unsigned int offset, int value)
    {
        *(volatile unsigned int *)(pBase + offset) = value;
    }

    /** 
     * Read a 4-byte value from the specified general-purpose I/O location. 
     * 
     * @param offset	Offset where device is mapped. 
     * @return		Value read. 
     */ 
    int RegisterRead(unsigned int offset)
    {
        return *(volatile unsigned int *)(pBase + offset);
    }
};

/** 
 * Class that controls the LEDs, switches, and push buttons. 
 * Each function takes a pointer to a DE1SoCfpga object to read and write the registers. 
 */ 
class LEDControl
{
public:
    LEDControl() {}		// Constructor (nothing to set up)
    ~LEDControl() {}	// Destructor (nothing to clean up)

    /** 
     * Read the state of one switch. 
     * 
     * @param fpga		Pointer to the DE1SoCfpga object. 
     * @param switchNum	Switch to read (0-9). 
     * @return		1 if the switch is on, 0 if off, or -1 if switchNum is invalid. 
     */ 
    int Read1Switch(DE1SoCfpga *fpga, int switchNum)
    {
        if (switchNum < 0 || switchNum > 9)	// check for an invalid switch number
        {
            return -1;
        }

        int allSwitches = fpga->RegisterRead(SW_BASE);
        return (allSwitches >> switchNum) & 0x1; //shifts the wanted switch down to bit 0 and ANDs with 1 to only keep that bit
    }

    /** 
     * Turn one LED on or off without changing the other LEDs. 
     * 
     * @param fpga		Pointer to the DE1SoCfpga object. 
     * @param ledNum	LED to change (0-9). 
     * @param state		State to write (0 = off, 1 = on). 
     */ 
    void Write1Led(DE1SoCfpga *fpga, int ledNum, int state)
    {
        int curLeds = fpga->RegisterRead(LEDR_BASE);	// read current LEDs so the others are kept

        if (state == 1)
        {
            //set: shift 1 left by ledNum, OR with curLeds
            curLeds |= (1 << ledNum);
        }
        else
        {
            //clear: shift 1 left by ledNum, invert and & with curLeds
            curLeds &= ~(1 << ledNum);
        }
        fpga->RegisterWrite(LEDR_BASE, curLeds); // writes state
    }

    /** 
     * Read the state of all 10 switches as one value. 
     * 
     * @param fpga		Pointer to the DE1SoCfpga object. 
     * @return		Value of the switches (0-1023). 
     */ 
    int ReadAllSwitches(DE1SoCfpga *fpga)
    {
        return fpga->RegisterRead(SW_BASE) & 0x3FF; //ANDs with 0x3FF (1111111111) to only keep the 10 switch bits, giving 0-1023
    }

    /** 
     * Write a value to all 10 LEDs. 
     * 
     * @param fpga		Pointer to the DE1SoCfpga object. 
     * @param value		Value to be written (0-1023). 
     */ 
    void WriteAllLeds(DE1SoCfpga *fpga, int value) // val is to be written
    {
        fpga->RegisterWrite(LEDR_BASE, value & 0x3FF);	// keep only the 10 LED bits
    }

    /** 
     * Read the push buttons. 
     * 
     * @param fpga		Pointer to the DE1SoCfpga object. 
     * @return		0-3 for KEY0-KEY3, 4 if more than one is pressed, or -1 if none. 
     */ 
    int PushButtonGet(DE1SoCfpga *fpga)
    {
        int keys = fpga->RegisterRead(KEY_BASE) & 0x0F;	// keep only the 4 button bits
        if (keys == 0) return -1;	// no buttons pressed

        // count how many buttons are pressed
        int count = 0;
        for (int i = 0; i < 4; i++)
        {
            if ((keys >> i) & 1) count++; //shifts button i down to bit 0, and adds 1 to count if it's pressed
        }
        if (count > 1) return 4;	// more than one button pressed

        if (keys == 1) return 0;	// KEY0
        if (keys == 2) return 1;	// KEY1
        if (keys == 4) return 2;	// KEY2
        if (keys == 8) return 3;	// KEY3

        return -1;
    }
};

/*
* Main Function
*/
int main() 
{ 
    // Initialize (the DE1SoCfpga constructor maps the memory)
    DE1SoCfpga fpga;
    LEDControl ledCtrl;

    // Counter test
    int counter = ledCtrl.ReadAllSwitches(&fpga);	// start the counter at the switch values
    ledCtrl.WriteAllLeds(&fpga, counter);
    cout << "Initial counter: " << counter << endl;

    int lastBtn = -1;	// last button state, used so each press only counts once

    while (true)
    {
        int btn = ledCtrl.PushButtonGet(&fpga);

        if (btn != lastBtn)	// only act when the button state changes
        {
            if (btn != -1 && (lastBtn == -1 || btn == 4))	// new press, or multiple buttons
            {
                if (btn == 0)
                {
                    counter = (counter + 1) % 1024;	// increment, rolls over to 0 after 1023
                }
                else if (btn == 1)
                {
                    counter = (counter == 0) ? 1023 : counter - 1;	// decrement, rolls over to 1023 after 0
                }
                else if (btn == 2)
                {
                    counter = (counter >> 1) & 0x3FF;	// shift right, insert 0
                }
                else if (btn == 3)
                {
                    counter = (counter << 1) & 0x3FF;	// shift left, insert 0
                }
                else if (btn == 4)
                {
                    counter = ledCtrl.ReadAllSwitches(&fpga);	// reset to switches
                }

                cout << "Counter: " << counter << endl;
                ledCtrl.WriteAllLeds(&fpga, counter);
            }
            lastBtn = btn;
        }

        usleep(20000);	// 20ms delay for debouncing
    }

    // Done (the DE1SoCfpga destructor unmaps the memory)
    return 0;
}