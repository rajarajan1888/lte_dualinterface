################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/lte/examples/lena-cqi-threshold.cc \
../src/lte/examples/lena-dual-stripe.cc \
../src/lte/examples/lena-fading.cc \
../src/lte/examples/lena-intercell-interference.cc \
../src/lte/examples/lena-pathloss-traces.cc \
../src/lte/examples/lena-profiling.cc \
../src/lte/examples/lena-rem-sector-antenna.cc \
../src/lte/examples/lena-rem.cc \
../src/lte/examples/lena-rlc-traces.cc \
../src/lte/examples/lena-simple-epc.cc \
../src/lte/examples/lena-simple.cc \
../src/lte/examples/lena-x2-handover.cc 

OBJS += \
./src/lte/examples/lena-cqi-threshold.o \
./src/lte/examples/lena-dual-stripe.o \
./src/lte/examples/lena-fading.o \
./src/lte/examples/lena-intercell-interference.o \
./src/lte/examples/lena-pathloss-traces.o \
./src/lte/examples/lena-profiling.o \
./src/lte/examples/lena-rem-sector-antenna.o \
./src/lte/examples/lena-rem.o \
./src/lte/examples/lena-rlc-traces.o \
./src/lte/examples/lena-simple-epc.o \
./src/lte/examples/lena-simple.o \
./src/lte/examples/lena-x2-handover.o 

CC_DEPS += \
./src/lte/examples/lena-cqi-threshold.d \
./src/lte/examples/lena-dual-stripe.d \
./src/lte/examples/lena-fading.d \
./src/lte/examples/lena-intercell-interference.d \
./src/lte/examples/lena-pathloss-traces.d \
./src/lte/examples/lena-profiling.d \
./src/lte/examples/lena-rem-sector-antenna.d \
./src/lte/examples/lena-rem.d \
./src/lte/examples/lena-rlc-traces.d \
./src/lte/examples/lena-simple-epc.d \
./src/lte/examples/lena-simple.d \
./src/lte/examples/lena-x2-handover.d 


# Each subdirectory must supply rules for building sources it contributes
src/lte/examples/%.o: ../src/lte/examples/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


