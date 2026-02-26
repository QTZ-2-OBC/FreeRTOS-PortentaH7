[![xc compatible](https://xcfile.dev/badge.svg)](https://xcfile.dev)

# FreeRTOS-PortentaH7

GSoC 2020 Arduino project.

[Portenta H7](https://www.arduino.cc/pro/hardware/product/portenta-h7) is a high
performance board released by [Arduino](https://www.arduino.cc).

## Tasks

### lib

Directory: ./STM32H747/

Build the static library of STM32H7 for CM4 and CM7.

```sh
./build.sh M4
./build.sh M7
```

### m4

Build the CM4 flash image, needs the static library to already be built!

```sh
make MCU=M4
```

### m7

Build the CM7 flash image, needs the static library to already be built!

```sh
make MCU=M7
```

### clean

Clean all generated artifacts (including the static library).

```sh
rm -r ./CM4/libs/ || echo "CM4 lib doesn't exist! Not cleaning..."
rm -r ./CM7/libs/ || echo "CM7 lib doesn't exist! Not cleaning..."

make MCU=M7 clean
make MCU=M4 clean
```

### flash

Inputs: CORE

Flash the to the specified core.

```sh
if [ "$CORE" == "m4" ]; then
    sudo dfu-util --device 0x2341:0x035b -D ./test/test_M4.elf.bin -a0 --dfuse-address=0x08100000:leave;
elif [ "$CORE" == "m7" ]; then
    sudo dfu-util --device 0x2341:0x035b -D ./test/test_M7.elf.bin -a0 --dfuse-address=0x08040000:leave;
else
    echo "The CORE variable must be 'm4' or 'm7'!"
fi
```
