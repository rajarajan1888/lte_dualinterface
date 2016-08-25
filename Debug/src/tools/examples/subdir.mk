################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/tools/examples/gnuplot-example.cc 

OBJS += \
./src/tools/examples/gnuplot-example.o 

CC_DEPS += \
./src/tools/examples/gnuplot-example.d 


# Each subdirectory must supply rules for building sources it contributes
src/tools/examples/%.o: ../src/tools/examples/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


