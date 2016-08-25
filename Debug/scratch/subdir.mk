################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../scratch/att_ex1.cc \
../scratch/checkworking.cc \
../scratch/lena-att.cc \
../scratch/lena-att_multiple.cc \
../scratch/lena-simple-epc.cc \
../scratch/lena-simple.cc \
../scratch/ltea_sample.cc \
../scratch/scratch-simulator.cc 

OBJS += \
./scratch/att_ex1.o \
./scratch/checkworking.o \
./scratch/lena-att.o \
./scratch/lena-att_multiple.o \
./scratch/lena-simple-epc.o \
./scratch/lena-simple.o \
./scratch/ltea_sample.o \
./scratch/scratch-simulator.o 

CC_DEPS += \
./scratch/att_ex1.d \
./scratch/checkworking.d \
./scratch/lena-att.d \
./scratch/lena-att_multiple.d \
./scratch/lena-simple-epc.d \
./scratch/lena-simple.d \
./scratch/ltea_sample.d \
./scratch/scratch-simulator.d 


# Each subdirectory must supply rules for building sources it contributes
scratch/%.o: ../scratch/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


