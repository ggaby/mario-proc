################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../../so-commons-library/src/commons/collections/dictionary.c \
../../../so-commons-library/src/commons/collections/list.c \
../../../so-commons-library/src/commons/queue.c 

OBJS += \
./commons/collections/dictionary.o \
./commons/collections/list.o \
./commons/collections/queue.o 

C_DEPS += \
./commons/collections/dictionary.d \
./commons/collections/list.d \
./commons/collections/queue.d 


# Each subdirectory must supply rules for building sources it contributes
commons/collections/%.o: ../../../so-commons-library/src/commons/collections/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


