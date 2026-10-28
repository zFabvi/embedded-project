# NUCLEO-C031C6 Project for E-Agle recruiting

This repository contains the firmware for acquiring, filtering, and transmitting serial data from analog sensors (POT and NTC). The project is built specifically for the STM32 NUCLEO-C031C6 board.

The development process and this documentation are strictly subdivided into three mandatory steps.

## Setup & HowTOs

The project is configured for the PlatformIO development environment.
1. Make sure to have PlatformIO installed.
2. Clone the repo and open the root folder.
3. Connect the NUCLEO-C031C6 board via USB (you'll need a Micro USB cable!)
4. To compile and upload the firmware to the board run `pio run -t upload`
5. Now open the serial monitor to view the data and interact. To do so run ` pio run -t monitor` and make sure to set the baud rate to 115200.

PS. You should update the editor's database with `pio run -t compiledb`, otherwise you'll see a bunch of errors with the HAL functions (that will obviously work anyway).

## Step N. 1

### CubeMX configs

First of all we need to capture the data that is coming from POT and NTC sensors.

To do so I had to handle the data stream through USART2 (in Asynchronous Mode, at 115200 baud rate). To prevent CPU overhead during data acquisition DMA is set to Circular Mode. The most obvious thing is to set two separate channels for the Potentiometer (PA0) and the NTC (PA1) and use the ADC Scan Mode and Continuous Conversion Mode for better performance. I also had to enable the PC13 pin to use the User Button.

PS. Remember that our ADC has a 12-bit physical limit ($2^{12} - 1 = 4095$).

### CLI and STREAMING modes 

You'll find the core logic in `main.c`.

* A buffer stores POT and NTC values and streams them through HAL functions.
* Dedicated functions in `conversions-api.c` are used to convert the RAW sensor values in physical units (Volts and Celsius).
* Flags are used to switch between CLI and Streaming mode (the switch is done by pressing the Blue Button on the board).

An array of characters is used to progressively store the letters. At the end we compare the buffer with the name of the commands (through `strcmp()`) and implement the logic behind them all. Then I added three commands to apply and remove filters (use them while in CLI mode):

* `noflt` removes applied filters.
* `mvavg` applies a moving average filter (150 elements cap for now). Here I used a Rolling Sum algorithm. Basically it takes a new element that enters the window and subtracts the one that goes out.
* `noise` applies spike noises to the values (I chose those noises 'cause they are sharp and it is difficult not to see them). Currently with `Xorshift32`.

The logic behind those filters is in `filters-api.c` so that `main.c` stays clean.

---

## Step N. 2

### Hardware Timers & PWM Design Choices

For the second step I redesigned the firmware around a table-driven Finite State Machine using the provided `libfsm-sw` library.

To handle timing and LED control on the STM32C031C6 (running at $48\text{ MHz}$), I configured two separate hardware timers:

* `TIM14`(1ms timebase): Set with `Prescaler = 48 - 1` and `Period = 1000 - 1` in interrupt mode (`HAL_TIM_Base_Start_IT`). Every millisecond, `HAL_TIM_PeriodElapsedCallback` increments a `tick_ms` counter. All periodic tasks 100ms UART streaming, 200ms warning messages, 2s button hold detection, and 5s / 10s timeouts) are computed by comparing elapsed ticks (`ctx->tick_ms - start_tick`).
* `TIM1` (Dual-Channel PWM): Drives both the Board LED (`PA5` on `TIM1_CH1`) and the External LED (`PA9` on `TIM1_CH2`). Because both channels belong to the same hardware timer instance (`TIM1`), they share a single Prescaler (`PSC`) and Auto-Reload Register (`ARR`).

I locked `TIM1` at a fixed 1kHz PWM frequency (`Prescaler = 48 - 1`, `Period = 1000 - 1`, giving compare values from `0` to `1000`). The External LED (`CCR2`) gets true analog-like brightness control by updating its duty cycle every millisecond, while the Board LED (`CCR1`) achieves exact blinking periods and duty cycles (like 500ms at 15% or 200ms at 80%) by toggling `CCR1` between `1000` and `0` against `tick_ms` modular arithmetic.

PS. Since `#include "eagletrt-fsm-api.h"` is included inside `fsm-api.h` rather than directly in `main.c`, PlatformIO's default dependency finder missed the static library during linking. Setting `lib_ldf_mode = deep+` and adding `libfsm-sw` to `lib_deps` in `platformio.ini` solved the issue.

### Software Architecture & `SystemContext`

`main.c` now acts purely as a hardware initializer. It calibrates the ADC, starts the circular DMA transfer, starts `TIM1` and `TIM14`, verifies that every HAL call returns `HAL_OK`, and then runs a single instruction inside `while(1)`:

```c
system_fsm_run(&sys_ctx);
```

I avoided scattering global variables across files by packing the entire application state into `struct SystemContext` (defined in `fsm-api.h` and passed via the `void *data` pointer):

* `adc_buffer`: pointer to the 2-element DMA array (`[0]` for POT, `[1]` for NTC).
* `active_filter`: tracks the currently selected filter (`0` = raw, `1` = moving average, `2` = spike noise).
* `tick_ms` and `btn_pressed`: `volatile` flags updated by `TIM14` and `EXTI13` ISRs.
* `cli_buffer` and `cli_index`: state variables for continuous UART character reception.
* `error_msg`: stores a string describing the exact fault that triggered `STATE_FATAL`.

#### State Machine Logic (`fsm-api.c`)

The state table maps all 6 states (`STATE_POST` to `STATE_FATAL`) and explicitly restricts valid transitions through dedicated `struct Transition` arrays. Whenever a state change occurs, transition callbacks (`transition_to_cli_state`, `transition_to_listening`, `transition_to_warning`, `transition_to_fatal`) automatically reset internal timestamps, clear pending button flags, and print the mode banners (`CLI MODE` or `LISTENING MODE`) once.

Here is how each state operates:

* **`POST` (Power-On Self Test):** Executed right after startup (with a 10ms initial settle time so the DMA populates `buffer_adc`). It checks if either `raw_pot` or `raw_ntc` is `0` (disconnected/open circuit) or `>= 4095` (shorted). If a fault is found, it writes the specific sensor name into `error_msg` and jumps to `Fatal`; otherwise it transitions to `Waiting`.
* **`Waiting` & `Paused`:** Both states share `process_cli()`, which polls `HAL_UART_Receive` with a 0ms timeout to read incoming bytes without blocking the LED animations. Characters are echoed back and buffered until `\r` or `\n` is received. Supported commands are the same as the ones in Step 1, but I added:
  * `run`: transitions the FSM into `Listening` mode (the same transition happens if the User Button on `PC13` is pressed).
  While in these states, `calc_breathing_duty()` creates a 2-second wave (`0 -> 1000 -> 0`) on the External LED, while the Board LED blinks at 500ms (15% duty cycle) in `Waiting` and 2000ms (50% duty cycle) in `Paused`.
* **`Listening`:** Continuously verifies that neither sensor hits `0` or `4095` at runtime. It applies the active filter and computes the raw difference `diff = raw_ntc - raw_pot`. The External LED PWM duty cycle scales between `0` and `500` raw ADC units (`FSM_DIFF_THRESHOLD`). If `diff > 500` continuously for `5000ms`, the board transitions to `Warning`. Every 100ms, raw values are converted to Volts and Celsius via `conversions-api.c` and streamed over UART2. Pressing the User Button pauses acquisition and moves to `Paused`.
* **`Warning`:** Shuts off both LEDs (`update_leds_pwm(0, 0)`), keeps checking the ADC buffer for physical sensor disconnections, and prints `WARNING` over serial every 200ms. To recover, the user must hold down the User Button (`PC13`, active-low) for at least 2000ms continuously (checked via `HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET`), which resets the state back to `Waiting`. If the board remains in `Warning` for 10 seconds (10000ms), it transitions to `Fatal`.
* **`Fatal`:**  Any unexpected HAL return code (`!= HAL_OK` on UART, ADC, DMA, or Timers), POST failure, runtime sensor disconnect, or unacknowledged `Warning` lands here. The External LED is turned off, the Board LED flashes with a 40ms period (20ms ON, 20ms OFF), and the exact fault stored in `ctx->error_msg` is printed every 100ms (`ERROR: <reason>`). Pressing the User Button calls `NVIC_SystemReset()` to reboot the microcontroller.












