################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/myether.c \
../src/rec.c \
../src/send.c \
../src/tx_file.c 

OBJS += \
./src/myether.o \
./src/rec.o \
./src/send.o \
./src/tx_file.o 

C_DEPS += \
./src/myether.d \
./src/rec.d \
./src/send.d \
./src/tx_file.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


