################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/common/subdir.mk
-include src/Memoria/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: libMemoria

dependents:
	-cd /home/utnso/tp-20131c-ahrelocoooo/Makes/Commons-library && $(MAKE) clean all

# Tool invocations
libMemoria: $(OBJS) $(USER_OBJS) /home/utnso/tp-20131c-ahrelocoooo/Makes/Commons-library/libcommons.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/tp-20131c-ahrelocoooo/Makes/Commons-library" -shared -o "libMemoria.so" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(LIBRARIES) libMemoria
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/tp-20131c-ahrelocoooo/Makes/Commons-library/libcommons.so:

-include ../makefile.targets
