#include <string.h>
#include "driverlib.h"
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"
#include "hal.h"

// Banderas
volatile uint8_t bCDCDataReceived_event = FALSE;  // Bandera para saber cuando se reciben datos en el buffer

// Variables globales
#define BUFFER_SIZE 256
char dataBuffer[BUFFER_SIZE] = "";
char nl[2] = "\n";
uint16_t count;

void main (void) {
    WDT_A_hold(WDT_A_BASE);                 // Detiene el watchdog timer

    PMM_setVCore(PMM_CORE_LEVEL_2);         // El Vcore mínimo para la USB API es PMM_CORE_LEVEL_2
    USBHAL_initPorts();                     // Configura puertos en LOW
    USBHAL_initClocks(8000000);             // Configura relojes MCLK=SMCLK=FLL=8MHz; ACLK=REFO=32kHz
    USB_setup(TRUE,TRUE);                   // Inicia USB & eventos. Conéctate si hay un host presente

    __enable_interrupt();                   // Activa interrupciones globalmente

    while (1) {
        uint8_t receiveError = 0;
        uint8_t sendError = 0;
        uint16_t count;

        switch (USB_getConnectionState()) {
            case ST_ENUM_ACTIVE:                        // Este caso se ejecuta cuando el micro se enumera en el host
                __disable_interrupt();

                // Si no hay bytes en el buffer...
                if (!USBCDC_getBytesInUSBBuffer(CDC0_INTFNUM))
                    __bis_SR_register(LPM0_bits + GIE); // ... entra en LPM0

                __enable_interrupt();

                // Si se recibieron bytes...
                if (bCDCDataReceived_event) {
                    bCDCDataReceived_event = FALSE;

                    // ... obtén la cuenta de bytes del dataBuffer ...
                    count = USBCDC_receiveDataInBuffer((uint8_t*)dataBuffer,BUFFER_SIZE,CDC0_INTFNUM);

                    // ... y haz un eco al host de los mismos datos
                    if (USBCDC_sendDataInBackground((uint8_t*)dataBuffer, count, CDC0_INTFNUM, 1)){
                        sendError = 0x01;               // Marca error de envío
                        break;                          // Termina la ejecución
                    }
                }
                break;

            case ST_PHYS_DISCONNECTED:                  // Este caso se ejecuta al desconectar el micro del host
            case ST_ENUM_SUSPENDED:                     // Este caso se ejecuta si el micro está enumerado pero suspendido por el host
            case ST_PHYS_CONNECTED_NOENUM_SUSP:         // Este caso se ejecuta si se conecta a un hub sin host (solo energía)
                __bis_SR_register(LPM3_bits + GIE);
                _NOP();
                break;

            case ST_ENUM_IN_PROGRESS:                   // Este caso se ejecuta momentáneamente mientras se enumera el micro en el host
            default:;
        }

        if (receiveError || sendError) {
            // Si hay algún error, manejarlo aquí
        }
    }  //while(1)
} // main()

/*
 * ======== UNMI_ISR ========
 *
 * Non Maskable Interrupts
 */
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(UNMI_VECTOR))) UNMI_ISR (void)
#else
#error Compiler not found!
#endif
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG ))
    {
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
            // Si el CPU accede la memoria del USB mientras el módulo del USB está suspendido,
            // esto puede causar un error de bus, lo cual genera una NMI.
            SYSBERRIV = 0;          // Limpia bandera de error
            USB_disable();          // Deshabilita el USB
    }
}
