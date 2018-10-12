// INCLUDE
#include <xc.h>

// CONFIG
#pragma config FOSC = INTOSC, WDTE = OFF, LVP = OFF

// DEFILE
#define _XTAL_FREQ 8000000

void putc(unsigned char c) {
    while (!TXSTAbits.TRMT); // wait for TX is ready
    TXREG = c;
}

void puts(const char* s) {
    while (*s != '\0') {
        putc((unsigned char)*s);
        s++;
    }
}

unsigned char getc() {
    unsigned char c = 0;
    
    while (PIR1bits.RCIF == 0); // wait for USART is received

    // check whether error occur
    if ((RCSTAbits.OERR) || (RCSTAbits.FERR)) {
        // the overrun error or the framing error occurred
        
        // restart USART
        RCSTA = 0;    // disable USART
        RCSTA = 0x90; // enable USART

    } else {
        c = RCREG; // get byte received by USART
    }
    
    return c;
}

void main(void) {
    /* set a clock frequency */
    OSCCON = 0b01110010; // 8MHz
    
    /* set port status */
    ANSELA  = 0b00000000; // all of pins are digital
    ANSELB  = 0b00000000; // all of pins are digital
    TRISA   = 0b00000000; // RA6(SDO1) is output
    TRISB   = 0b00100110; // RB1(SDI1), RB2(RX) and RB5(TX) are input, RB4(SCK) is output
    PORTA   = 0b00000000; // initialize portA
    PORTB   = 0b00000000; // initialize portB

    /* set USART settings */
    TXSTA   = 0x24; // TXEN(TX enable), BRGH(high speed)
    RCSTA   = 0x90; // SPEN(serial port enable), CREN(continually receive USART)
    BAUDCON = 0x08; // BRG16 (have 115.2 BAUD RATE is available)
    SPBRG   = 16; // actual rate: 115200 (data seat p303)
    RXDTSEL = 1; // RB2 to RX (data seat p.118)
    TXCKSEL = 1; // RB5 to TX (data seat p.118)
    
    /* set SPI settings */
    SSP1STAT = 0b00000000; // SMP(center), CKE(when idle to active: 0)
    SSP1CON1 = 0b00100001; // SSPEN, CKP(when low is idle: 0), Clock=FOSC/16
    SDO1SEL  = 1; // RA6 to SDO1 (data seat p.118)
    
    /* set interruption settings */
    PIR1bits.RCIF   = 0; // clear interruption flag
    PIE1bits.RCIE   = 1; // allow USART interruption
    PIR1bits.SSP1IF = 0; // clear interruption flag
    PIE1bits.SSP1IE = 1; // allow SPI interruption
    INTCONbits.PEIE = 1; // allow interrupting peripheral
    INTCONbits.GIE  = 1; // allow global
    
    while (1) {
        puts("ready ...\n");
        unsigned char c = getc();
        
        SSP1BUF = c;
        while (!SSP1STATbits.BF);
        unsigned char recv = SSP1BUF;
        puts("RECV: ");putc(recv);putc('\n');
    }
}
