
# This file is a part of MRNIU/FreeRTOS-PortentaH7 (https://github.com/MRNIU/FreeRTOS-PortentaH7).
#
# Makefile for MRNIU/FreeRTOS-PortentaH7.

CURR_DIR := $(shell pwd)
ROOT_DIR := $(CURR_DIR)

include $(ROOT_DIR)/Makefile.env

.PHONY: default
default:
	@$(MAKE) all

.PHONY: all
all: $(SUB_DIR)
	@if [ "$(MCU)" = "" ]; then\
        echo "Please specify M4 or M7";\
        exit 1;\
    fi
	@echo Entry $(CURR_DIR)
	@for subdir in $(SUB_DIR); \
    	do $(MAKE) -C $$subdir all || exit 1; \
    done
	$(MAKE) info
	$(MAKE) test_$(MCU).elf
	@echo Leave $(CURR_DIR)

.PHONY: remake
remake:
	$(MAKE) clean
	@$(MAKE) all

.PHONY: upload
upload:
	@echo uploading...
	$(DFU) $(DFU_FLAG)
	@echo Done.

.PHONY: clean
clean:
	@echo Entry $(CURR_DIR)
	@echo Deleting... $<
	@find . -name "*.o"  | xargs rm -f
	@find . -name "*.d"  | xargs rm -f
	@for subdir in $(SUB_DIR); \
		do $(MAKE) -C $$subdir clean || exit 1; \
	done
	@echo Deleting Completed
	@echo Leave $(CURR_DIR)

# OBJ_M4 := $(shell find  . -path ./CM7 -prune -o -name "*.o" -print -o -name "*4.ino" -print)
# OBJ_M7 := $(shell find  . -path ./CM4 -prune -o -name "*.o" -print -o -name "*7.ino" -print)

OBJ_M4 := $(shell find . \( -path ./CM7 -o -path ./deps -o -path ./Common/tests \) -prune -o -name "*.o" -print)
OBJ_M7 := $(shell find . \( -path ./CM4 -o -path ./deps -o -path ./Common/tests \) -prune -o -name "*.o" -print)

# This should work (?
.PHONY: test_M4.elf
test_M4.elf: $(OBJ_M4)
	$(CXX) $(LDFLAGS_ALL_$(MCU)) $^ -o $(INO_DIR)/$@
	$(OBJCPY) -O binary $(INO_DIR)/$@ $(INO_DIR)/$@.bin

.PHONY: test_M7.elf
test_M7.elf: $(OBJ_M7)
	$(CXX) $(LDFLAGS_ALL_$(MCU)) $^ -o $(INO_DIR)/$@
	$(OBJCPY) -O binary $(INO_DIR)/$@ $(INO_DIR)/$@.bin

.PHONY: code_line_count
code_line_count:
	find . -type f -name "*.[c|cpp|s|S|h|hpp]" -exec cat {} \; | wc -l

.PHONY: generate_map
generate_map:
	$(READELF) -s $(IMAGE_KERNEL) >> $(RES_KERNEL_MAP)

.PHONY: generate_nm
generate_nm:
	$(NM) $(IMAGE_KERNEL) >> $(RES_KERNEL_NM)

.PHONY: info
info:
	@echo Current dir: $(CURR_DIR)
	@echo MCU: $(MCU)
	@echo CC: $(CC)
	@echo CFLAGS_@MCU: $(CFLAGS_$(MCU))
	@echo CFLAGS_ARDUINOCORE_@MCU: $(CFLAGS_ARDUINOCORE_$(MCU))
	@echo CFLAGS_FREERTOS_@MCU: $(CFLAGS_FREERTOS_$(MCU))
	@echo CXX: $(CXX)
	@echo CXXFLAGS_@MCU: $(CXXFLAGS_$(MCU))
	@echo CXXFLAGS_ARDUINOCORE_@MCU: $(CXXFLAGS_ARDUINOCORE_$(MCU))
	@echo CXXFLAGS_INO_@MCU: $(CXXFLAGS_INO_$(MCU))
	@echo LDFLAGS_ALL_@MCU: $(LDFLAGS_ALL_$(MCU))
	@echo LINKERSCRIPT_@MCU: $(LINKERSCRIPT_$(MCU))
	@echo AS: $(AS)
	@echo ASFLAGS: $(ASFLAGS)
	@echo exclude_dirs: $(exclude_dirs)
	@echo SUBDIRS: $(SUB_DIR)
	@echo CORE_C@MCU_DIR: $(CORE_C$(MCU)_DIR)
	@echo CORE_C@MCU_INCLUDE_DIR: $(CORE_C$(MCU)_INCLUDE_DIR)
	@echo VARIANTS_DIR: $(VARIANTS_DIR)
	@echo MCU_@MCU: $(MCU_$(MCU))
	@echo DEFINES_@MCU: $(DEFINES_$(MCU))
	@echo INCLUDES_@MCU: $(INCLUDES_$(MCU))
	@echo FREERTOSH7_DIR: $(FREERTOSH7_DIR)
	@echo ARDUINOAPI_DIR: $(ARDUINOAPI_DIR)
	@echo DEPRECATED_DIR: $(DEPRECATED_DIR)
	@echo FREERTOS_DIR: $(FREERTOS_DIR)
	@echo FREERTOS_INCLUDE_DIR: $(FREERTOS_INCLUDE_DIR)
	@echo FREERTOS_IAR_DIR: $(FREERTOS_IAR_DIR)
	@echo STM32H7XXHAL_INCLUDE_DIR: $(STM32H7XXHAL_INCLUDE_DIR)
	@echo CMSIS_INCLUDE_DIR: $(CMSIS_INCLUDE_DIR)
	@echo OBJ_M4: $(OBJ_M4)
	@echo OBJ_M7: $(OBJ_M7)
