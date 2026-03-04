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
build() {
set -exu

# Build STM32H7xx HAL library for both M4 and M7

MCU=$1
ARCH_NUMBER=$([[ "$MCU" == "M4" ]] && echo "4" || echo "7")
if [ "$MCU" != "M4" ] && [ "$MCU" != "M7" ]; then
	echo "Usage: ./build_lib.sh M4|M7"
	exit 1
fi

if [ "$MCU" == "M4" ]; then
	mkdir -p ../CM4/libs
else
	mkdir -p ../CM7/libs
fi

echo "Building STM32H7xx HAL library for $MCU..."
EXTRA_FLAG=$([[ "$MCU" == "M4" ]] && echo "-mfpu=fpv4-sp-d16" || echo "-mfpu=fpv5-d16")
# Compile all .c files
for file in *.c; do
	arm-none-eabi-gcc -c "$file" -o "${file%.c}.o" \
		-mcpu=cortex-m"$ARCH_NUMBER" \
		-mthumb \
		-mfloat-abi=softfp \
		"$EXTRA_FLAG" \
		-DCORE_CM"$ARCH_NUMBER" \
		-DSTM32H747xx -DUSE_FULL_LL_DRIVER \
		-I./include \
		-I../CM"$ARCH_NUMBER"/include \
		-Os -g
done

# Compile startup file
echo "Compiling startup_stm32h747xx.s..."
arm-none-eabi-gcc -c startup_stm32h747xx.s -o startup_stm32h747xx.o \
	-mcpu=cortex-m"$ARCH_NUMBER" \
	-mthumb

# Create archive
echo "Creating library archive..."
arm-none-eabi-ar rcs ../CM"$ARCH_NUMBER"/libs/stm32h7xx.a ./*.o
echo "Library built: ../CM$ARCH_NUMBER/libs/stm32h7xx.a"

# Clean up object files
echo "Cleaning .o objects..."
rm ./*.o

echo "Done!"
}

build M4
build M7
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

# make MCU=M7 clean
# make MCU=M4 clean
make clean
```

### flash

Inputs: CORE

Flash the to the specified core.

```sh
if [ "$CORE" == "m4" ]; then
    sudo dfu-util --device 0x2341:0x035b -D ./bin/test_M4.elf.bin -a0 --dfuse-address=0x08100000:leave;
elif [ "$CORE" == "m7" ]; then
    sudo dfu-util --device 0x2341:0x035b -D ./bin/test_M7.elf.bin -a0 --dfuse-address=0x08040000:leave;
else
    echo "The CORE variable must be 'm4' or 'm7'!"
fi
```

### unity

Directory: ./deps/unity/

Compile the Munit testing framework.

```sh
gcc -c unity.c -O2 -o unity.o
```

### test

Build and run the test binary. Compile the unity framework first!

```sh
set +x
cd ./Common/tests/
for file in *.c; do
    printf "================================================\n"
    printf "Testing: %s\n" "$file"
    printf "================================================\n"
    set -x
    gcc ../../deps/**/*.o "$file" -lm -o test.bin
    ./test.bin
    set +x
done
```

### bear

Requires: clean

Regenerate compile_commands.json databases for both cores. This enables
autocomplete!

```sh
xc lib
bear --output CM4/compile_commands.json -- xc m4
bear --output CM7/compile_commands.json -- xc m7
```
