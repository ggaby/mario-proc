################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../src/Plataforma/Plataforma.c 

OBJS += \
./src/Plataforma/Plataforma.o 

C_DEPS += \
./src/Plataforma/Plataforma.d 


# Each subdirectory must supply rules for building sources it contributes
src/Plataforma/%.o: ../../src/Plataforma/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/so-commons-library/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


