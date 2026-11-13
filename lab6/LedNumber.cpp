#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

using namespace std;

// Physical base address of FPGA Devices 
const unsigned int LW_BRIDGE_BASE = 0xFF200000;
const unsigned int LW_BRIDGE_SPAN = 0x00005000;

// Cyclone V FPGA device addresses
const unsigned int LEDR_BASE      = 0x00000000;
const unsigned int SW_BASE        = 0x00000040;
const unsigned int KEY_BASE       = 0x00000050;

class DE1SoCfpga
{
    char *pBase;
    int fd;

public:
    DE1SoCfpga()
    {
        fd = open("/dev/mem", (O_RDWR | O_SYNC));
        if (fd == -1)
        {
            cout << "ERROR: could not open /dev/mem..." << endl;
            exit(1);
        }
        pBase = (char *)mmap(NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
        if (pBase == MAP_FAILED)
        {
            cout << "ERROR: mmap() failed..." << endl;
            close(fd);
            exit(1);
        }
    }

    ~DE1SoCfpga()
    {
        if (munmap(pBase, LW_BRIDGE_SPAN) != 0)
        {
            cout << "ERROR: munmap() failed..." << endl;
            exit(1);
        }
        close(fd);
    }

    void RegisterWrite(unsigned int offset, int value)
    {
        *(volatile unsigned int *)(pBase + offset) = value;
    }

    int RegisterRead(unsigned int offset)
    {
        return *(volatile unsigned int *)(pBase + offset);
    }
};

class LEDControl
{
public:
    LEDControl() {}
    ~LEDControl() {}

    int Read1Switch(DE1SoCfpga *fpga, int switchNum)
    {
        if (switchNum < 0 || switchNum > 9)
        {
            return -1;
        }

        int allSwitches = fpga->RegisterRead(SW_BASE);
        // does all switches, shifted right by the specified one 
        // (isolating specified) ANDed with 1 
        return (allSwitches >> switchNum) & 0x1;	
    }

    void Write1Led(DE1SoCfpga *fpga, int ledNum, int state)
    {
        int curLeds = fpga->RegisterRead(LEDR_BASE);

        if (state == 1)
        {
            //clear: shift 1 left by ledNum, OR with curLeds
            curLeds |= (1 << ledNum);
        }
        else
        {
            //clear: shift 1 left by ledNum, invert and & with curLeds
            curLeds &= ~(1 << ledNum);
        }
        fpga->RegisterWrite(LEDR_BASE, curLeds); // writes state
    }

    int ReadAllSwitches(DE1SoCfpga *fpga)
    {
        return fpga->RegisterRead(SW_BASE) & 0x3FF; // 1111111111 = 10
    }

    void WriteAllLeds(DE1SoCfpga *fpga, int value) // val is to be written
    {
        fpga->RegisterWrite(LEDR_BASE, value & 0x3FF);
    }

    int PushButtonGet(DE1SoCfpga *fpga)
    {
        int keys = fpga->RegisterRead(KEY_BASE) & 0x0F;
        if (keys == 0) return -1;

        int count = 0;
        for (int i = 0; i < 4; i++)
        {
            if ((keys >> i) & 1) count++;
        }
        if (count > 1) return 4;

        if (keys == 1) return 0;
        if (keys == 2) return 1;
        if (keys == 4) return 2;
        if (keys == 8) return 3;

        return -1;
    }
};

int main() 
{ 
    DE1SoCfpga fpga;
    LEDControl ledCtrl;
    /*
    // Sample test program
    int value = 0; 
    int state = 0;
    //cout << "Enter an int value between 0 to 1023: " << endl; 
    //cin >> value; 
    //cout << "value to be written to LEDs = " << value << endl; 
    //WriteAllLeds(pBase, value);
    //int readLEDs = RegisterRead(pBase, LEDR_BASE);
    //cout << "value of LEDS read = " << readLEDs << endl;
    cout << "Enter switch to read (0..9): " << endl; 
    cin >> value; 
    int res = ledCtrl.Read1Switch(&fpga, value);
    cout << "Val of switch: " << res << endl;

    cout << "Enter LED to write (0..9): " << endl; 
    cin >> value; 
    cout << "Enter a state to write (0 or 1): " << endl; 
    cin >> state;
    ledCtrl.Write1Led(&fpga, value, state);
    cout << "Written" << endl;

    int allSwitches = ledCtrl.ReadAllSwitches(&fpga);
    cout << "All switches: " << allSwitches << endl;
    ledCtrl.WriteAllLeds(&fpga, 500);

    // Counter test
    int counter = ledCtrl.ReadAllSwitches(&fpga);
    ledCtrl.WriteAllLeds(&fpga, counter);
    cout << "Initial counter: " << counter << endl;
    */
    int lastBtn = -1;

    while (true)
    {
        int btn = ledCtrl.PushButtonGet(&fpga);

        if (btn != lastBtn)
        {
            if (btn != -1)
            {
                if (btn == 0)
                {
                    counter = (counter + 1) % 1024; // incr  w/ loopback logic
                }
                else if (btn == 1)
                {
                    counter = (counter == 0) ? 1023 : counter - 1; // decr w/ loopback logic
                }
                else if (btn == 2)
                {
                    counter = (counter >> 1) & 0x3FF; // rshift
                }
                else if (btn == 3)
                {
                    counter = (counter << 1) & 0x3FF; // lshift
                }
                else if (btn == 4)
                {
                    counter = ledCtrl.ReadAllSwitches(&fpga); // rst to switch states
                }

                cout << "Counter: " << counter << endl;
                ledCtrl.WriteAllLeds(&fpga, counter);
            }
            lastBtn = btn;
        }

        usleep(20000); // slight dbounce, could be raised
    }

    return 0;
}
