################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/tree/climbers/Climber.cpp \
../src/treenet/tree/climbers/Robin.cpp \
../src/treenet/tree/climbers/Crow.cpp \
../src/treenet/tree/climbers/Cat.cpp \
../src/treenet/tree/climbers/Ant.cpp

OBJS += \
./src/treenet/tree/climbers/Climber.o \
./src/treenet/tree/climbers/Robin.o \
./src/treenet/tree/climbers/Crow.o \
./src/treenet/tree/climbers/Cat.o \
./src/treenet/tree/climbers/Ant.o

CPP_DEPS += \
./src/treenet/tree/climbers/Climber.d \
./src/treenet/tree/climbers/Robin.d \
./src/treenet/tree/climbers/Crow.d \
./src/treenet/tree/climbers/Cat.d \
./src/treenet/tree/climbers/Ant.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/tree/climbers/%.o: ../src/treenet/tree/climbers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


