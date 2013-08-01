################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../src/Plataforma/Planificador/Planificador.c 

OBJS += \
./src/Plataforma/Planificador/Planificador.o 

C_DEPS += \
./src/Plataforma/Planificador/Planificador.d 


# Each subdirectory must supply rules for building sources it contributes
src/Plataforma/Planificador/%.o: ../../src/Plataforma/Planificador/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/so-commons-library/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


