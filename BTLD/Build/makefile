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
           $(MCAL_path)/FLASH/src \
		   ../Parse\
           ../Startup\
		   ../Src\
		   

# Include directories
INC_DIRS = ../Inc/ \
		   ../Parse\
           $(wildcard $(MCAL_path)/*/inc/) \
           $(wildcard $(HAL_path)/*/inc/)

INC_FLAGS = $(addprefix -I,$(INC_DIRS))

# Source and object files — C
SRC_FILES     = $(wildcard $(addsuffix /*.c,$(SRC_DIRS)))
OBJ_FILES     = $(patsubst ../%, $(OBJDIR)/%, $(SRC_FILES:.c=.o))
DEP_FILES     = $(OBJ_FILES:.o=.d)
vpath %.c $(SRC_DIRS)

# Source and object files — C++
SRC_FILES_CPP = $(wildcard $(addsuffix /*.cpp,$(SRC_DIRS)))
OBJ_FILES_CPP = $(patsubst ../%, $(OBJDIR)/%, $(SRC_FILES_CPP:.cpp=.o))
DEP_FILES_CPP = $(OBJ_FILES_CPP:.o=.d)
vpath %.cpp $(SRC_DIRS)

# Color codes
COLOR_GREEN = \033[0;32m
COLOR_BLUE = \033[0;34m
COLOR_RESET = \033[0m

.PHONY: all clean flash erase

all: $(Images)/BTLD.bin size

size: $(Images)/BTLD.elf
	@$(SIZE) $<
	@printf  "$(COLOR_GREEN)++ Build complete$(COLOR_RESET)\n"

$(Images)/BTLD.bin: $(Images)/BTLD.elf
	@$(BIN) -O binary $< $@
	@$(BIN) -O ihex $< $(@:.bin=.hex)
	@cp $(@:.bin=.hex) $(@:.bin=.txt)
	@printf  "$(COLOR_BLUE)++ Binary files generated$(COLOR_RESET)\n"

$(Images)/BTLD.elf: $(OBJ_FILES) $(OBJ_FILES_CPP)
	@if [ -z "$(OBJ_FILES)$(OBJ_FILES_CPP)" ]; then \
		printf  "$(COLOR_RED)Error: No object files found - check compilation$(COLOR_RESET)\n"; \
		exit 1; \
	fi
# 	@$(info MCAL_path = $(MCAL_path))
# 	@$(info HAL_path = $(HAL_path))
# 	@$(info SRC_DIRS = $(SRC_DIRS))
# 	@$(info SRC_FILES = $(SRC_FILES))
# 	@$(info OBJ_FILES = $(OBJ_FILES))
	@printf  "$(COLOR_BLUE)Linking with objects:. . .$(COLOR_RESET)\n"
	@$(CXX) $(LDFLAGS) -Wl,-Map,$(Images)/BTLD.map -T $(LinkerDir)/LinkerScript.ld $(OBJ_FILES) $(OBJ_FILES_CPP) -o $@
	@printf  "$(COLOR_BLUE)++ Linking complete$(COLOR_RESET)\n"


$(OBJDIR)/%.o: ../%.c
	@mkdir -p $(dir $@)
	@printf  "$(COLOR_GREEN)Compiling $(notdir $<)$(COLOR_RESET)\n"
	@$(CC) $(CFLAGS) $(INC_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

$(OBJDIR)/%.o: ../%.cpp
	@mkdir -p $(dir $@)
	@printf  "$(COLOR_GREEN)Compiling $(notdir $<)$(COLOR_RESET)\n"
	@$(CXX) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

clean:
	@rm -rf $(OBJDIR)/* $(Images)/*.elf $(Images)/*.map $(Images)/*.bin $(Images)/*.hex $(Images)/*.txt
	@printf  "$(COLOR_GREEN)++ Clean complete$(COLOR_RESET)\n"

flash: $(Images)/BTLD.bin
	@$(STL) -c port=swd -d $< 0x08000000 -v -rst

erase:
	@$(STL) -c port=swd  all

-include $(DEP_FILES) $(DEP_FILES_CPP)