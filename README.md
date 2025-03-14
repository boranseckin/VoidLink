# VoidLink

An affordable and compact wireless communication device.
VoidLink forms a decentralized network to relay relative position and critical information over long
ranges with minimal power.

Uses:

- [sx126x driver](https://github.com/Lora-net/sx126x_driver/) from Semtech (ported for raspberry pi pico)
- [Pico_ePaper_Code](https://github.com/waveshareteam/Pico_ePaper_Code) from Waveshare

## Usage

1. Clone the repo including the driver library.

```bash
git clone --recurse-submodules https://github.com/boranseckin/voidlink.git
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
picotool load voidlink.uf2
```

## Pinout

The default pinout diagram between sx126x, eink display, buttons and pico.

| T/X  | uC            | uC                  | T/X    |
| ---- | ------------- | ------------------- | ------ |
| GND  | GND (38)      |                     | ANT    |
| GND  | GND (38)      | GND (38)            | GND    |
| RXEN |               | GP17 (22)           | CS/NSS |
| TXEN |               | GP18(SPI0 SCK) (24) | CLK    |
| DIO2 |               | GP19(SPI0 TX) (25)  | MOSI   |
| DIO1 | GP22 (29)     | GP16(SPI0 RX) (21)  | MISO   |
| GND  | GND (38)      | GP20 (26)           | RESET  |
| 3V3  | 3V3(OUT) (36) | GP21 (27)           | BUSY   |

| eInk | uC                 |
| ---- | ------------------ |
| VCC  | 3V3(OUT) (36)      |
| GND  | GND                |
| DIN  | GP11(SPI1 TX) (15) |
| CLK  | GP10(SPI SCK) (14) |
| CS   | GP9(SPI CS) (12)   |
| DC   | GP8 (11)           |
| RST  | GP12 (16)          |
| BUSY | GP13 (17)          |

| Button | uC      |
| ------ | ------- |
|Previous| GP6 (8) |
| Next   | GP5 (7) |
| Ok     | GP4 (6) |
| Back   | GP3 (5) |
