#include <string.h>
#include "driverlib.h"
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"

#include "hal.h"

// Funciones
void initRTC(void);

// Variables globales
volatile uint8_t sendStringToHost = FALSE;                    // Bandera para decidir si enviar datos por USB
uint8_t usbString[26] = "Hello world from MSP430!\n\r";       // String a mandar


void main(void) {
    WDT_A_hold(WDT_A_BASE);                 // Detiene el watchdog timer

    PMM_setVCore(PMM_CORE_LEVEL_2);         // El Vcore mínimo para la USB API es PMM_CORE_LEVEL_2
    USBHAL_initPorts();                     // Configura puertos en LOW
    USBHAL_initClocks(8000000);             // Configura relojes MCLK=SMCLK=FLL=8MHz; ACLK=REFO=32kHz
    USB_setup(TRUE,TRUE);                   // Inicia USB & eventos. Conéctate si hay un host presente

    initRTC();                              // Inicializa el reloj de tiempo real

    __enable_interrupt();                   // Activa interrupciones globalmente

    while (1) {
        __bis_SR_register(LPM0_bits + GIE); // Entra a LPM0, que mantiene DCO/FLL activo. Para USB, no se puede menos de LPM0

        if (sendStringToHost) {
            sendStringToHost = FALSE;

            if (USBCDC_sendDataInBackground(usbString, 26, CDC0_INTFNUM, 1000)) {
              _NOP();  // Si falla la comunicación, llega aquí. Tal vez por desconectar el cable o pasar el timeout de 1000 intentos
            }
        }
    }  //while(1)
}  //main()


/*
 * @Función         initRTC
 *
 * @Descripción     Inicializa el TA0 como timer de 1s
 * @Params          N/A
 */
void initRTC (void) {
    TA0CCR0 = 32768;
    TA0CTL = TASSEL_1 | MC_1 | TACLR;
    TA0CCTL0 = CCIE;
}


// Timer0 A0 ISR. Se ejecuta cada que TAR pasa de 32768 a 0
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
#else
#error Compiler not found!
#endif
{
    sendStringToHost = TRUE;                // Permite enviar string al host
    __bic_SR_register_on_exit(LPM3_bits);   // Salir de Low Power Mode
}


// USB ISR
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(UNMI_VECTOR))) UNMI_ISR (void)
#else
#error Compiler not found!
#endif
{
        switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG )) {
        case SYSUNIV_NONE:
                __no_operation();
                break;
        case SYSUNIV_NMIIFG:
                __no_operation();
                break;
        case SYSUNIV_OFIFG:
                UCS_clearFaultFlag(UCS_XT2OFFG);
                UCS_clearFaultFlag(UCS_DCOFFG);
                SFR_clearInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);
                break;
        case SYSUNIV_ACCVIFG:
                __no_operation();
                break;
        case SYSUNIV_BUSIFG:
                // If the CPU accesses USB memory while the USB module is
                // suspended, a "bus error" can occur.  This generates an NMI.  If
                // USB is automatically disconnecting in your software, set a
                // breakpoint here and see if execution hits it.  See the
                // Programmer's Guide for more information.
                SYSBERRIV = 0;  // Clear bus error flag
                USB_disable();  // Disable
        }
}
