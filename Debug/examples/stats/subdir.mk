################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../examples/stats/wifi-example-apps.cc \
../examples/stats/wifi-example-sim.cc 

OBJS += \
./examples/stats/wifi-example-apps.o \
./examples/stats/wifi-example-sim.o 

CC_DEPS += \
./examples/stats/wifi-example-apps.d \
./examples/stats/wifi-example-sim.d 


# Each subdirectory must supply rules for building sources it contributes
examples/stats/%.o: ../examples/stats/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


