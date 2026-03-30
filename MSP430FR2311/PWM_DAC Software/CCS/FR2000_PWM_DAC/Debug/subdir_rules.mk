################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
main.obj: C:/Users/a0224668/Documents/NPD/Hummingbird/PWM_DAC/Source/main.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.4.LTS/bin/cl430" -vmspx --code_model=small -O4 --opt_for_speed=0 --use_hw_mpy=none --include_path="C:/ti/ccsv7/ccs_base/msp430/include" --include_path="C:/Users/a0224668/Documents/NPD/Hummingbird/PWM_DAC/CCS/FR2000_PWM_DAC" --include_path="C:/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.4.LTS/include" --advice:power=all --advice:hw_config=all --define=__MSP430FR2000__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="main.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


