################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Nivel/VerificadorDeadlock/verificador_deadlock.c 

OBJS += \
./src/Nivel/VerificadorDeadlock/verificador_deadlock.o 

C_DEPS += \
./src/Nivel/VerificadorDeadlock/verificador_deadlock.d 


# Each subdirectory must supply rules for building sources it contributes
src/Nivel/VerificadorDeadlock/%.o: ../src/Nivel/VerificadorDeadlock/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/so-nivel-gui-library" -I"/home/utnso/so-commons-library/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


