# EECE2160 – Terasic Spider Robot

C++ code for controlling the Terasic Spider hexapod (DE10-Nano SoC) for EECE2160, Fall 2026.

| Folder     | Contents                             | Status         |
|------------|--------------------------------------|----------------|
| `lab7/`    | Servo control and single-leg motion  | ✅ Done        |
| `lab8/`    | TBD                                  | 🚧 Coming soon |
| `project/` | Sensor project                       | 🚧 Coming soon |

---

## Lab 7 – Controlling Spider Servos in C++

The spider has 6 legs with 3 servos each (hip, knee, ankle), for 18 RC servos total. The PWM generators already live in the FPGA, so this lab drives them from C++ on the ARM side through memory-mapped registers.

### Part 1 – `Motor` class (`lab7/part1-motor/`)

Completes the `Motor` class and tests one servo at a time. The servo starts at 0° and the push buttons step it:

- **KEY0** → −15°
- **KEY1** → +15°

### Part 2 – `SpiderLeg` class (`lab7/part2-spiderleg/`)

Adds a `SpiderLeg` class that holds the three `Motor` objects for one leg and runs a coordinated sequence of movements.

### How the servos are driven

Each servo has four 32-bit registers: period, duty cycle, adjust delay (read back as the ready bit), and abort. The clock is 50 MHz, so the 20 ms period is 1,000,000 cycles.

Angle maps linearly to the duty cycle:

| Angle | Pulse width | Clock cycles |
|-------|-------------|--------------|
| −90°  | 0.5 ms      | 25,000       |
| 0°    | 1.5 ms      | 75,000       |
| 90°   | 2.5 ms      | 125,000      |

$$\text{DC} = 75{,}000 + \frac{50{,}000}{90}\,\theta, \qquad -90^\circ \le \theta \le 90^\circ$$

Speed is set on a 0–100 scale and converted to the servo adjust delay (1700 cycles at speed 0, 1000 cycles at speed 100):

$$\text{delay} = 1700 - 7 \cdot \text{speed}$$

A new angle is only written once the servo's ready bit is 1, so a servo never gets a new command mid-motion. The right-side servos (IDs 0–8) are mounted mirrored, so their angles are negated.

### Motor IDs

| Leg          | IDs (hip, knee, ankle) |
|--------------|------------------------|
| Right front  | 0, 1, 2                |
| Right middle | 3, 4, 5                |
| Right back   | 6, 7, 8                |
| Left front   | 9, 10, 11              |
| Left middle  | 12, 13, 14             |
| Left back    | 15, 16, 17             |

### Build and run

```bash
# ssh into the DE10-Nano first
cd lab7/part1-motor      # or lab7/part2-spiderleg
make
./spider                 # executable name is set in the Makefile
```

> ⚠️ Hold the spider by its chassis before running so it can't walk off the table.
>
> To stop, **power off the robot**. Restarting the program does not reset the PWM circuits.

### Files

| File                | Purpose                                              |
|---------------------|------------------------------------------------------|
| `RegisterMap.h`     | Register addresses for PWM, LEDs, switches, and keys |
| `MMap.cpp/.h`       | Maps device memory; register read/write              |
| `LEDControl.cpp/.h` | LEDs, switches, and push buttons                     |
| `Motor.cpp/.h`      | Single servo: angle, speed, ready check, reset       |
| `SpiderLeg.cpp/.h`  | Three motors grouped as one leg (Part 2)             |
| `Main.cpp`          | Test program                                         |
| `Makefile`          | Build                                                |

---

## Lab 8

_Coming soon._

---

## Sensor Project

_Coming soon._
