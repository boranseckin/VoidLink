# sx126x-pico

[sx126x driver](https://github.com/Lora-net/sx126x_driver/) port for raspberry pi pico

## Usage

1. Clone the repo including the driver library.

```bash
git clone --recurse-submodules https://github.com/boranseckin/sx126x-pico.git
```

2. Install the [Pico C/C++ SDK](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html).

3. Set the `PICO_SDK_PATH`.

4. Build the project. Use `pico` or `pico2` depending on the target.

```bash
mkdir build
cd build
cmake .. -DPICO_BOARD=pico
make
```

5. Load the firmware onto the target.

```bash
# for tx
picotool load sx126x_pico_tx.uf2
# for rx
picotool load sx126x_pico_rx.uf2
```

## Pinout

The default pinout diagram between sx126x and pico.
| T/X | uC | uC | T/X |
| ---- | ------------- | ------------------- | ------ |
| GND | GND (38) | | ANT |
| GND | GND (38) | GND (38) | GND |
| RXEN | | GP8 (11) | CS/NSS |
| TXEN | | GP18(SPI0 SCK) (24) | CLK |
| DIO2 | | GP19(SPI0 TX) (25) | MOSI |
| DIO1 | GP10 (14) | GP16(SPI0 RX) (21) | MISO |
| GND | GND (38) | GP9 (12) | RESET |
| 3V3 | 3V3(OUT) (36) | GP11 (15) | BUSY |
