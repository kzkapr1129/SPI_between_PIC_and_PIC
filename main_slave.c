/*
 * File:   main.c
 * Author: Nakayama
 *
 * Created on September 28, 2018, 8:43 PM
 */

#include <xc.h>

#define _XTAL_FREQ 8000000
// #define USE_INNER_PULLUP

#pragma config FOSC = INTOSC, WDTE = OFF, LVP = OFF

static const unsigned char LCD_ADD_7BIT = 0x3e;
static const unsigned char LCD_ADD_8BIT = LCD_ADD_7BIT << 1;

void ST7032_LCD_init(unsigned char contrast);
void ST7032_LCD_write(unsigned char cont, char data);
void ST7032_LCD_writeString(unsigned char cont, const char* str);

void main(void) {
    /* set a clock frequency */
    OSCCON = 0b01110010; // 8MHz
    
    /* set port status */
    ANSELA = 0b00000000; // all of pins are digital
    ANSELB = 0b00000000; // all of pins are digital
    TRISA  = 0b00000000; // RA0(SDO2) is output
    TRISB  = 0b00110110; // RB1(SDA), RB2(SDI2), RB4(SCL) and RB5(SCK2) are input
    PORTA  = 0b00000000;
    PORTB  = 0b00000000;
    
    /* set SPI settings */
    SSP2STAT = 0b00000000; // SMP(slave), CKE(when idle to active: 0)
    SSP2CON1 = 0b00100101; // SSPEN, CKP(when low is idle: 0), disable SS-pin
    SSP2CON3 = 0b00000000; // BOEN(0: ignore to update SSPxBUF when BF flag set)
    
    /* set interruption settings */
    PIR4bits.SSP2IF = 0; // clear interruption flag
    PIE4bits.SSP2IE = 1; // allow SPI interruption
    INTCONbits.PEIE = 1; // allow interrupting peripheral
    INTCONbits.GIE  = 1; // allow global
    
    ST7032_LCD_init(36);
    ST7032_LCD_writeString(0x40, "initializing");
    
    __delay_ms(100); // wait until volt become stable    
    
    ST7032_LCD_write(0x00, 0x01); // Clear display

    while (1) {
        while (!SSP2STATbits.BF);
        unsigned char ret = SSP2BUF;
        SSP2BUF = '*';
        if ((ret >= 'a' && ret <= 'z') ||
                (ret >= 'A' && ret <= 'Z') ||
                (ret >= '0' && ret <= '9')) {
            ST7032_LCD_write(0x00, 0x01); // Clear display
            __delay_ms(10);
            ST7032_LCD_write(0x40, ret);
        } else {
            ST7032_LCD_write(0x00, 0x01); // Clear display
            __delay_ms(500);
            ST7032_LCD_writeString(0x40, "invalid char");
        }
    }
}

void I2C_Master_init() {
#ifdef USE_INNER_PULLUP
    /* set pull up */
    WPUB   = 0b00010010;
    WPUBbits.WPUB1 = 1;
    WPUBbits.WPUB4 = 1;
#else
    WPUB   = 0b00000000;
#endif
    
    SSP1STAT = 0b10000000; // standard mode (100kHz)
    SSP1CON1 = 0b00101000; // allow clock from slave, I2C master mode
    SSP1CON2 = 0;
    SSP1ADD = 0x13;
}

void I2C_Master_wait() {
    // wait for founding START or ACK sequence
    while ((SSP1STAT & 0x04) || (SSP1CON2 & 0x1F));
}

void I2C_Master_start() {
    SSP1CON2bits.SEN = 1;
    I2C_Master_wait();
}

void I2C_Master_stop() {
    SSP1CON2bits.PEN = 1;
    I2C_Master_wait();
}

void I2C_Master_write(unsigned char d) {
    SSP1BUF = d;
    I2C_Master_wait();
}

void ST7032_LCD_init(unsigned char contrast) {
    __delay_ms(40);
    
    I2C_Master_init();
    __delay_ms(100);
    
    ST7032_LCD_write(0x00, 0x38); // Function set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x39); // Function set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x14); // Internal OSC frequency
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x70|(contrast & 0xf)); // Contrast set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x54|((contrast & 0x30) >> 4)); // Power/ICON/Contrast control
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x6C); // Follower control
    __delay_ms(200);
    ST7032_LCD_write(0x00, 0x38); // Function set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x0d); // Display ON/OFF control
    __delay_ms(2);
    ST7032_LCD_write(0x00, 0x01); // Clear display    

    __delay_ms(100);
}

void ST7032_LCD_write(unsigned char cont, char data) {
    I2C_Master_start();
    I2C_Master_write(LCD_ADD_8BIT);
    I2C_Master_write(cont);
    I2C_Master_write(data);
    I2C_Master_stop();
}

void ST7032_LCD_writeString(unsigned char cont, const char* str) {
    const char* d;
    for (d = str; *d != '\0'; d++) {
        char c = *d;
        ST7032_LCD_write(cont, c);
        __delay_us(30);
    }
}