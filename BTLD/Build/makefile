# Ensure directories exist before building
include makeconfig
$(shell mkdir -p $(OBJDIR) $(Images))

ifdef APPLICATION_ONLY
CFLAGS+= -DAPPLICATION_ONLY
endif

# Source directories
SRC_DIRS +=$(MCAL_path)/RCC/src \
           $(MCAL_path)/NVIC/src \
           $(MCAL_path)/SysTick/src \
           $(MCAL_path)/GPIO/src \
           $(MCAL_path)/EXTI/src \
           $(MCAL_path)/AFIO/src \
           $(MCAL_path)/SPI/src \
           $(MCAL_path)/UART/src \
           $(MCAL_path)/FPEC/src \
           $(MCAL_path)/PwrMD/src \
           $(MCAL_path)/DMA/src \
           $(MCAL_path)/IWDG/src \
           $(MCAL_path)/TIMERS/src\
           $(MCAL_path)/I2C/src\
           $(HAL_path)/LCD/src\
           $(HAL_path)/EEPROM/src\
           $(HAL_path)/OLED_SH1106/src\
		   ../RTOS\
           ../Startup\
           ../P1/flappy_bird\
		   ../Src\
		   

# Include directories
INC_DIRS = ../Inc/ \
		   ../RTOS\
		   ../P1/flappy_bird\
           $(wildcard $(MCAL_path)/*/inc/) \
           $(wildcard $(HAL_path)/*/inc/)

INC_FLAGS = $(addprefix -I,$(INC_DIRS))

# Source and object files
SRC_FILES = $(wildcard $(addsuffix /*.c,$(SRC_DIRS)))
OBJ_FILES = $(patsubst ../%, $(OBJDIR)/%, $(SRC_FILES:.c=.o))
DEP_FILES = $(OBJ_FILES:.o=.d)
vpath %.c $(SRC_DIRS)

# Color codes
COLOR_GREEN = \033[0;32m
COLOR_BLUE = \033[0;34m
COLOR_RESET = \033[0m

.PHONY: all clean flash erase

all: $(Images)/BTLD.bin size

size: $(Images)/BTLD.elf
	@$(SIZE) $<
	@echo -e "$(COLOR_GREEN)++ Build complete$(COLOR_RESET)"

$(Images)/BTLD.bin: $(Images)/BTLD.elf
	@$(BIN) -O binary $< $@
	@$(BIN) -O ihex $< $(@:.bin=.hex)
	@cp $(@:.bin=.hex) $(@:.bin=.txt)
	@echo -e "$(COLOR_BLUE)++ Binary files generated$(COLOR_RESET)"

$(Images)/BTLD.elf: $(OBJ_FILES)
	@if [ -z "$(OBJ_FILES)" ]; then \
		echo -e "$(COLOR_RED)Error: No object files found - check compilation$(COLOR_RESET)"; \
		exit 1; \
	fi
# 	@$(info MCAL_path = $(MCAL_path))
# 	@$(info HAL_path = $(HAL_path))
# 	@$(info SRC_DIRS = $(SRC_DIRS))
# 	@$(info SRC_FILES = $(SRC_FILES))
# 	@$(info OBJ_FILES = $(OBJ_FILES))
	@echo -e "$(COLOR_BLUE)Linking with objects:. . .$(COLOR_RESET)"
	@$(LD) -Map $(Images)/BTLD.map -T $(LinkerDir)/LinkerScript.ld $(OBJ_FILES) -o $@
	@echo -e "$(COLOR_BLUE)++ Linking complete$(COLOR_RESET)"


$(OBJDIR)/%.o: ../%.c
	@mkdir -p $(dir $@)
	@echo -e "$(COLOR_GREEN)Compiling $(notdir $<)$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

clean:
	@rm -rf $(OBJDIR)/* $(Images)/*.elf $(Images)/*.map $(Images)/*.bin $(Images)/*.hex $(Images)/*.txt
	@echo -e "$(COLOR_GREEN)++ Clean complete$(COLOR_RESET)"

flash: $(Images)/BTLD.bin
	@$(STL) -c port=swd -d $< 0x08000000 -v -rst

erase:
	@$(STL) -c port=swd -e all

-include $(DEP_FILES)