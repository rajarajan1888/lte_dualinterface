################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../examples/tcp/star.cc \
../examples/tcp/tcp-bulk-send.cc \
../examples/tcp/tcp-large-transfer.cc \
../examples/tcp/tcp-nsc-lfn.cc \
../examples/tcp/tcp-nsc-zoo.cc \
../examples/tcp/tcp-star-server.cc 

OBJS += \
./examples/tcp/star.o \
./examples/tcp/tcp-bulk-send.o \
./examples/tcp/tcp-large-transfer.o \
./examples/tcp/tcp-nsc-lfn.o \
./examples/tcp/tcp-nsc-zoo.o \
./examples/tcp/tcp-star-server.o 

CC_DEPS += \
./examples/tcp/star.d \
./examples/tcp/tcp-bulk-send.d \
./examples/tcp/tcp-large-transfer.d \
./examples/tcp/tcp-nsc-lfn.d \
./examples/tcp/tcp-nsc-zoo.d \
./examples/tcp/tcp-star-server.d 


# Each subdirectory must supply rules for building sources it contributes
examples/tcp/%.o: ../examples/tcp/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


