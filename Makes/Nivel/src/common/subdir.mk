################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../src/common/Recurso.c \
../../src/common/common_structs.c \
../../src/common/list.c \
../../src/common/mensaje.c \
../../src/common/posicion.c \
../../src/common/sockets.c 

OBJS += \
./src/common/Recurso.o \
./src/common/common_structs.o \
./src/common/list.o \
./src/common/mensaje.o \
./src/common/posicion.o \
./src/common/sockets.o 

C_DEPS += \
./src/common/Recurso.d \
./src/common/common_structs.d \
./src/common/list.d \
./src/common/mensaje.d \
./src/common/posicion.d \
./src/common/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
src/common/%.o: ../../src/common/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/so-nivel-gui-library" -I"/home/utnso/so-commons-library/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


