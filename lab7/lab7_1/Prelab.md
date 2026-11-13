## Pre-lab 1

i) 50 MHz clock → 1 ms = 50,000 cycles

- −90° → 0.5 ms → 25,000 cycles
- 0° → 1.5 ms → 75,000 cycles
- 90° → 2.5 ms → 125,000 cycles

ii) Linear map, −90°–90° → 25,000–125,000 cycles

$$DC = 25000 + (\theta + 90)\cdot (\frac{100000}{180})$$

DC in cycles, θ in degrees. Clamp θ to [−90, 90], compute as float, cast to int, write to duty cycle register.

## Pre-lab 2

Linear map, speed 0–100 → delay 1700–1000 cycles

$$\text{delay} = 1700 - (\frac{700}{100})\cdot \text{speed} = 1700 - 7\cdot \text{speed}$$

Clamp speed to [0, 100] first.
