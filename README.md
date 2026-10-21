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

### Step N. 1

#### Signal Processing

First of all we need to capture the data that is coming from POT and NTC sensors. 

To do so I had to handle the data stream through USART2 (in Asynchronous Mode, at 115200 baud rate). 
To prevent CPU overhead during data acquisition DMA is set to Circular Mode.
The most obvious thing is to set two separate channels for the Potentiometer (PA0) and the NTC (PA1) and use the ADC Scan Mode and Continuous Conversion Mode for better performance.
I also had to enable the PC13 pin to use the User Button. 

PS. Remember that our ADC has a 12-bit physical limit. Check [here](https://www.st.com/en/microcontrollers-microprocessors/stm32c031c6.html).

You'll find the core login in `main.c`.
- A buffer stores POT and NTC values and streams them through HAL functions.
- Dedicated functions in `conversions.c` are used to convert the RAW sensor values in physical units (Volts and Celsius).
- Flags are used to switch between CLI and Streaming mode (the switch is done by pressing the Blue Button on the board).

An array of characters is used to progressively store the letters. At the end we compare the buffer with the name of the commands (through `strcmp()`) and implement the logic behind them all.
Then I added three commands to apply and remove filters (use them while in CLI mode):
- `noflt` removes applied filters.
- `mvavg` applies a moving average filters (150 elements cap for now). Here I used a Rolling Sum algorithm. Basically it takes a new element that enters the window and subtracts the one that goes out.
- `noise` applies spike noises to the values (i chose those noises 'cause them are sharp and is difficult not to see them). Currently with `rand()` (i'll implement Xorshift32 soon, is much more efficient and lightweight).

The logic behind those filters is in `filters.c` so that `main.c` stays clean.











