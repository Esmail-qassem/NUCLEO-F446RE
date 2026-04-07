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
           $(MCAL_path)/GPIO/src\
           $(MCAL_path)/EXTI/src\
		   $(MCAL_path)/ADC/src\
           $(MCAL_path)/AFIO/src \
           $(MCAL_path)/UART/src \
           $(MCAL_path)/FPEC/src \
           $(MCAL_path)/PwrMD/src \
           $(MCAL_path)/DMA/src \
           $(MCAL_path)/IWDG/src \
           $(MCAL_path)/BASIC_TIMER/src\
           $(MCAL_path)/I2C/src\
           $(MCAL_path)/FLASH/src\
           $(MCAL_path)/GP_TIMER/src\
		   $(MCAL_path)/PendSV/src\
           $(HAL_path)/ESP\
           $(HAL_path)/OLED\
           $(P1_path)/FLAPPY_Bird\
		   ../RTOS\
           ../Startup\
		   ../Src\
		   

# Include directories
INC_DIRS = ../Inc/ \
		   ../RTOS\
           $(wildcard $(MCAL_path)/*/inc/) \
           $(wildcard $(HAL_path)/*/)\
           $(wildcard $(P1_path)/*/)

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

all: $(Images)/application.bin size

size: $(Images)/application.elf
	@$(SIZE) $<
	@printf  "$(COLOR_GREEN)++ Build complete$(COLOR_RESET)\n"

$(Images)/application.bin: $(Images)/application.elf
	@$(BIN) -O binary $< $@
	@$(BIN) -O ihex $< $(@:.bin=.hex)
	@cp $(@:.bin=.hex) $(@:.bin=.txt)
	@printf  "$(COLOR_BLUE)++ Binary files generated$(COLOR_RESET)\n"

$(Images)/application.elf: $(OBJ_FILES)
	@if [ -z "$(OBJ_FILES)" ]; then \
		printf  "$(COLOR_RED)Error: No object files found - check compilation$(COLOR_RESET)"; \
		exit 1; \
	fi
# 	@$(info MCAL_path = $(MCAL_path))
# 	@$(info HAL_path = $(HAL_path))
# 	@$(info SRC_DIRS = $(SRC_DIRS))
# 	@$(info SRC_FILES = $(SRC_FILES))
# 	@$(info OBJ_FILES = $(OBJ_FILES))
	@printf  "$(COLOR_BLUE)Linking with objects:. . .$(COLOR_RESET)\n"
	@$(LD) -Map $(Images)/application.map -T $(LinkerDir)/LinkerScript.ld $(OBJ_FILES) -o $@
	@printf  "$(COLOR_BLUE)++ Linking complete$(COLOR_RESET)\n"


$(OBJDIR)/%.o: ../%.c
	@mkdir -p $(dir $@)
	@printf  "$(COLOR_GREEN)Compiling $(notdir $<)$(COLOR_RESET)\n"
	@$(CC) $(CFLAGS) $(INC_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

clean:
	@rm -rf $(OBJDIR)/* $(Images)/*.elf $(Images)/*.map $(Images)/*.bin $(Images)/*.hex $(Images)/*.txt
	@printf  "$(COLOR_GREEN)++ Clean complete$(COLOR_RESET)\n"

flash: $(Images)/application.bin
	@$(STL) --reset write $< 0x08000000


erase:
	@$(STL) erase

-include $(DEP_FILES)