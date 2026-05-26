/*
 * File:   main.c
 * Author: viktor
 *
 * Created on April 14, 2026, 7:31 PM
 */

#define F_CPU 16000000UL
#include "chip1.h"
#include <avr/cpufunc.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>



int main(void) 
{
    oschf_init();  //inits clock at 16MH and sets it as main clock
    usart_init();
    PORTA.DIRSET = PIN2_bm;
    PORTF.DIRCLR = PIN3_bm;

    
    sei();                              //Enables global interrupts
    
                                        
    
    uint8_t power = 10;
    
    tca_split_init(power);
    
    

    adc_init();
   
    RTC_init();

    
    while (1) 
    {
        
        if (message_ready)
        {
            message_ready = 0;
            parse_message((char*)rx_buffer);
            
            
        }

        
        
        
        
    }
    return 0;
}
