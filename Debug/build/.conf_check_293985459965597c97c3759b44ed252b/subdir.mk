################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../build/.conf_check_293985459965597c97c3759b44ed252b/test.cpp 

OBJS += \
./build/.conf_check_293985459965597c97c3759b44ed252b/test.o 

CPP_DEPS += \
./build/.conf_check_293985459965597c97c3759b44ed252b/test.d 


# Each subdirectory must supply rules for building sources it contributes
build/.conf_check_293985459965597c97c3759b44ed252b/%.o: ../build/.conf_check_293985459965597c97c3759b44ed252b/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


