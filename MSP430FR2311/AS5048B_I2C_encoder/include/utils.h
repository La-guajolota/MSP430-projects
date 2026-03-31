/**
 * @file utils.h
 * @brief System clock initialization and hardware utilities.
 * @author Adrián Silva Palafox
 * @date may 31 2026
 */
#ifndef UTILS_H
#define UTILS_H

#include <msp430.h>

/**
 * @brief Configures the internal DCO to 16 MHz and sets FRAM wait states.
 * * Modifies the CSCTL registers to set the Digitally Controlled Oscillator (DCO) 
 * to 16 MHz, sourcing MCLK and SMCLK. Configures NWAITS_1 in the FRAM control 
 * register to allow memory access at clock speeds exceeding 8 MHz.
 * * @warning Disables the FLL during configuration and polls until hardware lock is achieved.
 */
void initClockTo16MHz(void);

#endif /* UTILS_H */