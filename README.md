# VoidLink

A decentralized mesh communication system for off-grid environments.
It uses LoRa to enable low-cost and portable nodes that relay messages and location data without any external infrastructure.
VoidLink offers a self-sustaining alternative to satellite phones for adventurers, geologists, and emergency responders.

Read more about it in the [blog post](https://www.boranseckin.com/projects/voidlink).

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

## Uses
- [sx126x driver](https://github.com/Lora-net/sx126x_driver/) from Semtech (ported for raspberry pi pico)
- [Pico_ePaper_Code](https://github.com/waveshareteam/Pico_ePaper_Code) from Waveshare
