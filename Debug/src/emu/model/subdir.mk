################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/emu/model/emu-encode-decode.cc \
../src/emu/model/emu-net-device.cc \
../src/emu/model/emu-sock-creator.cc 

OBJS += \
./src/emu/model/emu-encode-decode.o \
./src/emu/model/emu-net-device.o \
./src/emu/model/emu-sock-creator.o 

CC_DEPS += \
./src/emu/model/emu-encode-decode.d \
./src/emu/model/emu-net-device.d \
./src/emu/model/emu-sock-creator.d 


# Each subdirectory must supply rules for building sources it contributes
src/emu/model/%.o: ../src/emu/model/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


