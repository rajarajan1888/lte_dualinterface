################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/mpi/model/distributed-simulator-impl.cc \
../src/mpi/model/mpi-interface.cc \
../src/mpi/model/mpi-receiver.cc 

OBJS += \
./src/mpi/model/distributed-simulator-impl.o \
./src/mpi/model/mpi-interface.o \
./src/mpi/model/mpi-receiver.o 

CC_DEPS += \
./src/mpi/model/distributed-simulator-impl.d \
./src/mpi/model/mpi-interface.d \
./src/mpi/model/mpi-receiver.d 


# Each subdirectory must supply rules for building sources it contributes
src/mpi/model/%.o: ../src/mpi/model/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


