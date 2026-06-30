# Bluetooth-Controlled Smart RC Car (STM32F446RE)

A Bluetooth-controlled robotic car built on the **STM32 Nucleo-F446RE** board. The car is driven over **HC-05 Bluetooth** from a custom **MIT App Inventor** Android app (joystick control, horn, and headlight buttons), and is protected by an **HC-SR04 ultrasonic sensor** that automatically stops the car and sounds a buzzer if it gets too close to an obstacle while moving forward.

---

## Table of Contents

- [Components Used](#components-used)
- [Schematic / Wiring Diagram](#schematic--wiring-diagram)
- [Demo Video](#demo-video)
- [Project Gallery](#project-gallery)
- [MIT App Inventor Application](#mit-app-inventor-application)
- [Firmware Overview](#firmware-overview)
- [Pin Mapping](#pin-mapping)
- [Function Documentation](#function-documentation)
- [Main Loop Walkthrough](#main-loop-walkthrough)
- [Communication Protocol (HC-05 Packets)](#communication-protocol-hc-05-packets)

---

## Components Used

| Component | Quantity | Purpose |
|---|---|---|
| STM32 Nucleo-F446RE | 1 | Main microcontroller board |
| HC-05 Bluetooth Module | 1 | Wireless serial link to the Android app |
| HC-SR04 Ultrasonic Sensor | 1 | Obstacle detection / auto-stop |
| L298N Motor Driver | 2 | Dual H-bridge driver for the 4 BO motors |
| BO Motor (TT Gear Motor) | 4 | Drive motors (2 left side, 2 right side) |
| White LED | 1 | Headlight |
| Red LED | 1 | Obstacle / brake warning indicator |
| Buzzer (PWM driven) | 1 | Horn + obstacle warning tone |
| 2S2P 18650 Li-ion Battery Pack | 1 | Main power source for motors |
| Power Bank | 1 | Power source for the STM32 board |

---

## Schematic / Wiring Diagram

<img width="1479" height="850" alt="Schematic diagram" src="https://github.com/user-attachments/assets/345013d2-77e6-487b-96d1-9152dbae88a5" />

---

## Demo Video

[![Watch the demo](https://img.youtube.com/vi/KXQ_XRK3Z9U/0.jpg)](https://www.youtube.com/watch?v=KXQ_XRK3Z9U)

---

## Project Gallery

| Top View | Side View | Rear View 1 |
|---|---|---|
| <img width="320" alt="Top view" src="https://github.com/user-attachments/assets/5bffcb82-fdd2-432a-9a55-b467ee897b69" /> | <img width="320" alt="Side view" src="https://github.com/user-attachments/assets/c5e051e7-48de-4e3a-80d8-31ef59878367" /> | <img width="320" alt="Rear view" src="https://github.com/user-attachments/assets/e4a2f975-5b33-4007-a7ac-0b0a965eff8c" /> |

| Riew View 2 | Front View |
|---|---|
|  |  |

---

## MIT App Inventor Application

The car is controlled by a custom Android app built with **MIT App Inventor**, featuring a joystick for movement, a horn button, and a headlight toggle, all communicating with the car over Bluetooth (HC-05).

_Add screenshots of the app interface and a link/QR code to the `.aia` project or compiled `.apk` below._

| App Home Screen | Control Screen |
|---|---|
| ![App Home](./assets/app_home.png) | ![App Controls](./assets/app_controls.png) |

- 📱 [Download the compiled `.apk`](./Bluetooth_Joystick.apk)

---

## Firmware Overview

The firmware is generated from **STM32CubeMX** (HAL-based) and extended with custom application logic. At a high level it:

1. Receives 3-byte command packets from the HC-05 over **USART1** via interrupt-driven reception.
2. Periodically triggers the **HC-SR04** ultrasonic sensor and measures echo pulse width using **TIM1** input capture to compute distance.
3. Converts joystick input into PWM duty cycles (via **TIM2** channels 1 & 2) and direction GPIOs for the two **L298N** drivers.
4. Smoothly ramps motor speed/direction (instead of snapping instantly) to protect the motors and gearbox, including a "deadtime" pause when reversing direction.
5. Drives a **PWM buzzer** (via **TIM3** channel 1) for the horn and for obstacle warnings, and toggles the white headlight / red warning LEDs.
6. Automatically halts forward motion and sounds an alarm if an obstacle is detected within **15 cm**.

---

## Pin Mapping

| Function | Port/Pin | Peripheral |
|---|---|---|
| HC-SR04 Trig | PB10 | GPIO Output |
| HC-SR04 Echo | PA8 | TIM1 CH1 (Input Capture) |
| L298N #1 IN1 | PC9 | GPIO Output |
| L298N #1 IN2 | PC8 | GPIO Output |
| L298N #2 IN3 | PB8 | GPIO Output |
| L298N #2 IN4 | PC6 | GPIO Output |
| Left Motor PWM | TIM2 CH1 | PWM Output |
| Right Motor PWM | TIM2 CH2 | PWM Output |
| White LED (Headlight) | PB4 | GPIO Output |
| Red LED (Warning) | PB5 | GPIO Output |
| Buzzer | TIM3 CH1 | PWM Output |
| HC-05 TX/RX | USART1 | UART, 9600 baud |
| Debug/Aux UART | USART2 | UART, 9600 baud |

---

## Function Documentation

### `int _write(int file, char *ptr, int len)`
Retargets the C standard library's `printf` to send characters over **ITM (SWO)** for debugging, by calling `ITM_SendChar()` on each byte.

### `void set_frequency(uint32_t frequency)`
Computes the auto-reload register (ARR) value needed to produce the given `frequency` on **TIM3** (assuming a 1 MHz timer clock) and updates the ARR and 50% duty-cycle compare value (`CCR`), effectively setting the buzzer's pitch.

### `void buzzer_ring(void)`
Starts PWM output on **TIM3 CH1** and sets the buzzer tone to 2500 Hz, turning the buzzer on.

### `void buzzer_stop(void)`
Stops PWM output on **TIM3 CH1**, silencing the buzzer.

### `void delay(uint16_t time)`
A blocking microsecond-ish delay implemented by resetting **TIM1**'s counter and busy-waiting until it reaches `time`. Used for the HC-SR04 10 µs trigger pulse.

### `void HCSR04_Read(void)`
Issues a 10 µs HIGH pulse on the HC-SR04 trigger pin to start a distance measurement, then enables **TIM1 CH1** capture-compare interrupt so the echo pulse width can be measured asynchronously in `HAL_TIM_IC_CaptureCallback`.

### `void ramp_left_motors(Movement *leftSide, uint16_t target, uint8_t backwardsFlag)`
Smoothly drives the **left** side motors (via L298N IN3/IN4 and TIM2 CH1 PWM) toward a `target` CCR (speed) value:
- Detects a direction reversal request. If the motor is already stopped, the direction flips immediately; if it's still spinning, the target is forced to 0 and a `REVERSE_DEADTIME` countdown begins so the motor fully stops before reversing (protects the gearbox/driver).
- Counts down `deadtime` each call while forcing `target = 0`, then commits the new direction once the deadtime expires or the motor has actually stopped.
- Ramps `CCR` toward `target` in steps of at most `MAX_STEP` per call (rather than jumping instantly), preventing abrupt speed changes.
- Writes the resulting `CCR` to `TIM2->CCR1` and sets the L298N IN3/IN4 GPIO pins according to stop / forward / backward state.

### `void ramp_right_motors(Movement *rightSide, uint16_t target, uint8_t backwardsFlag)`
Identical logic to `ramp_left_motors`, but operates on the **right** side motors using `TIM2->CCR2` and the L298N IN1/IN2 pins.

### `void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)`
Interrupt callback fired by **TIM1 CH1** input-capture edges, used to time the HC-SR04 echo pulse:
- On the first (rising-edge) capture, records `timeValue1` and switches the input capture polarity to falling edge.
- On the second (falling-edge) capture, records `timeValue2`, computes the elapsed tick `difference` (handling timer counter wraparound), converts it to `distance` in cm using the speed of sound (`difference * 0.034 / 2`), resets the polarity back to rising edge, and disables the capture interrupt until the next `HCSR04_Read()` call.

### `void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)`
Interrupt callback fired when a byte is received on **USART1** (HC-05 link). Implements a simple variable-length packet receiver keyed off `rxBuffer[0]` (the command ID):
- `1` = Joystick packet, 3 bytes total (`[1, xValue, yValue]`)
- `2` = Horn packet, 2 bytes total (`[2, onOff]`)
- `3` = Light packet, 2 bytes total (`[3, onOff]`)

Once the expected byte count for the current command is received, `rxReadyFlag` is set so the main loop can process the packet, and the next byte's interrupt reception is re-armed via `HAL_UART_Receive_IT`.

### `void Error_Handler(void)`
Disables interrupts and halts in an infinite loop. Called whenever a HAL initialization step fails, used as a catch-all fault state.

### Peripheral Init Functions (CubeMX-generated)
- `SystemClock_Config()` — configures the HSI-driven PLL to clock the MCU.
- `MX_GPIO_Init()` — configures all GPIO pins (LEDs, L298N control pins, trigger pin, user button).
- `MX_TIM1_Init()` — configures TIM1 as an input-capture timer for echo pulse timing.
- `MX_TIM2_Init()` — configures TIM2 as a dual-channel PWM generator for the left/right motor speed.
- `MX_TIM3_Init()` — configures TIM3 as a single-channel PWM generator for the buzzer.
- `MX_USART1_UART_Init()` — configures USART1 (9600 baud) for the HC-05 Bluetooth link.
- `MX_USART2_UART_Init()` — configures USART2 (9600 baud), used for auxiliary/debug serial.

---

## Main Loop Walkthrough

The `while (1)` loop in `main()` runs continuously and performs four jobs every iteration:

1. **Process incoming Bluetooth commands** (only when `rxReadyFlag == 1`, set by the UART RX callback):
   - **Joystick (`rxBuffer[0] == 1`):** Maps the raw joystick byte values (0–251 range) to a PWM target (`targetLeftCCR` / `targetRightCCR`), determines forward/backward direction from whether the byte is above or below 125, and applies a minimum CCR floor (`MIN_CCR`) so the motors don't stall at very low duty cycles.
   - **Horn (`rxBuffer[0] == 2`):** Calls `buzzer_ring()` or `buzzer_stop()` depending on the payload byte.
   - **Light (`rxBuffer[0] == 3`):** Turns the white headlight LED on or off depending on the payload byte.

2. **Poll the ultrasonic sensor every 60 ms** — calls `HCSR04_Read()` on a non-blocking timer (`HAL_GetTick()`), kicking off a new distance measurement without stalling the loop.

3. **Update motor ramping every 20 ms:**
   - If the car is commanded to move **forward** and the last measured `distance` is below `MAXIMUM_DISTANCE` (15 cm), the target speed is forced to 0, the buzzer sounds, and the red warning LED turns on.
   - Otherwise (distance is safe), the buzzer stops and the red LED turns off.
   - `ramp_left_motors()` and `ramp_right_motors()` are called once per 20 ms tick to gradually step the motors toward their target speed/direction.

4. Loop repeats indefinitely — all timing is handled via `HAL_GetTick()` comparisons, so the loop never blocks except for the brief 10 µs `delay()` used inside `HCSR04_Read()`.

---

## Communication Protocol (HC-05 Packets)

| Byte 0 (Command ID) | Byte 1 | Byte 2 | Meaning |
|---|---|---|---|
| `1` | X value (0–251) | Y value (0–251) | Joystick: left motor speed/direction (X), right motor speed/direction (Y). 0–125 = forward, 126–251 = backward |
| `2` | `1` / `0` | — | Horn ON / OFF |
| `3` | `1` / `0` | — | Headlight ON / OFF |

---

## License

This project's firmware is built on top of STMicroelectronics' HAL drivers (see header in `main.c` for the original copyright/license terms). Add your own project license here (e.g. MIT) if you intend to share the rest of the code/app freely.
