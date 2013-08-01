################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../../so-commons-library/src/commons/bitarray.c \
../../../so-commons-library/src/commons/config.c \
../../../so-commons-library/src/commons/error.c \
../../../so-commons-library/src/commons/log.c \
../../../so-commons-library/src/commons/process.c \
../../../so-commons-library/src/commons/string.c \
../../../so-commons-library/src/commons/temporal.c \
../../../so-commons-library/src/commons/commons/txt.c 

OBJS += \
./commons/bitarray.o \
./commons/config.o \
./commons/error.o \
./commons/log.o \
./commons/process.o \
./commons/string.o \
./commons/temporal.o \
./commons/txt.o 

C_DEPS += \
./commons/bitarray.d \
./commons/config.d \
./commons/error.d \
./commons/log.d \
./commons/process.d \
./commons/string.d \
./commons/temporal.d \
./commons/txt.d 


# Each subdirectory must supply rules for building sources it contributes
commons/%.o: ../../../so-commons-library/src/commons/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


