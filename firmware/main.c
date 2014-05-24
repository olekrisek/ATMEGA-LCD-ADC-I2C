/* Name: main.c
 * Author: <insert your name here>
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#include <avr/io.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>


#define DS1803 0b01010000
#define POT01  0x00
#define POTREAD 0x01
#define CMD 0xA0
#define POTCB_WR0 CMD | 0x09
#define POTCB_WR1 CMD | 0x0A
#define POTCB_WRB CMD | 0x0F


// --------------- READ ADC VALUE ----------------------------
// -----------------------------------------------------------
uint16_t adc_read(uint8_t ch)
{
    // select the corresponding channel 0~7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
    
    // start single convertion
    // write ’1′ to ADSC
    ADCSRA |= (1<<ADSC);
    
    // wait for conversion to complete
    while(ADCSRA & (1<<ADSC));
 
 	// Read 10-bit values, and combine into one 16bit integer. 
    int lo = ADCL;
    int hi = ADCH;
    
    return (hi << 8) | lo;
}

// -------------- TWI TWO WIRE INTERFACE ROUTINES -----------------
// ----------------------------------------------------------------
void TWIInit(void)
{
    //set SCL to 41 KHz
    TWSR = 0x00;
    TWBR = 0x04;
    //enable TWI
    TWCR = (1<<TWEN);
}

void TWIStart(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}
//send stop signal
void TWIStop(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
    while (TWCR & (1<<TWSTO));
}

void TWIWrite(uint8_t u8data)
{
    TWDR = u8data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

uint8_t TWIReadACK(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    while ((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}
//read byte with NACK
uint8_t TWIReadNACK(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}

uint8_t TWIGetStatus(void)
{
    uint8_t status;
    //mask status
    status = TWSR & 0xF8;
    return status;
}

int statusCheck (uint8_t expected, uint8_t stage) {
    uint8_t readstatus = TWIGetStatus();
    if (readstatus != expected ) {
        char str[22];
        lcd_gotoxy(0,2);
        sprintf(str, "E: %i - %i - %i", readstatus, expected, stage);
        lcd_puts (str) ;
        return 1;
    }
    return 0;
}
// ---------- END TWI Routines. ----------------

// Set digital potensiometer value using TWI routines above. 
// -------------------------------------

void SetPotValue(int vpot, uint8_t value) {

    TWIStart();
    if (statusCheck ( 0x08, 1)) return;
    TWIWrite(DS1803| POT01);
    if (statusCheck ( 0x18, 2)) return;
    
    if (vpot == 0 )
        TWIWrite(POTCB_WR0);
    else if (vpot == 1 )
         TWIWrite(POTCB_WR1);
    else TWIWrite(POTCB_WRB);
 
    if (statusCheck ( 0x28, 3)) return;
   
    TWIWrite(value);
    if (statusCheck ( 0x28, 4)) return;

    TWIStop();
    return ;
}

// - MAIN ENTRY ----------------------------------
// -----------------------------------------------
int main(void)
{
    
    /* insert your hardware initialization here */
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();
    lcd_gotoxy(0,0);
    lcd_puts("Velkommen Ole K");
    lcd_gotoxy(0,1);
    lcd_puts("Virker... !!");
    
    // Analog conversion....
    ADMUX |= (1<<REFS0); // VCC as voltage reference.
    ADCSRA |= (1<<ADPS2);// prescaler = 16
    //ADMUX |= (1 << ADLAR); //
    ADCSRA |= (1 << ADEN); // Enable ADC
    
    TWIInit();
    
    SetPotValue(0,120);
    
    int val;
    char str[10];
    for(;;){
        val = adc_read(0);
        itoa (val,str,10);
        lcd_gotoxy(0,3);
        lcd_puts(str);
        lcd_putc(32);
    }
    return 0;   /* never reached */
}

