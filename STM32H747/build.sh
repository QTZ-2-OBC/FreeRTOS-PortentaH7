#!/usr/bin/env bash

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
