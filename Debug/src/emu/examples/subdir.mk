################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/emu/examples/emu-ping.cc \
../src/emu/examples/emu-udp-echo.cc 

OBJS += \
./src/emu/examples/emu-ping.o \
./src/emu/examples/emu-udp-echo.o 

CC_DEPS += \
./src/emu/examples/emu-ping.d \
./src/emu/examples/emu-udp-echo.d 


# Each subdirectory must supply rules for building sources it contributes
src/emu/examples/%.o: ../src/emu/examples/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


