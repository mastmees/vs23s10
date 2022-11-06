/*
The MIT License (MIT)

Copyright (c) 2022 Madis Kaal <mast@nomad.ee>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include "font.h"

#include <math.h>
#include <stdlib.h>


#include "vs23s010.hpp"

#ifndef COUNTOF
#define COUNTOF(x) (unsigned int)(sizeof(x)/sizeof(x[0]))
#endif

#define reset_low() PORTB&=~2
#define reset_high() PORTB|=2
#define spics_low() PORTB&=~4
#define spics_high() PORTB|=4
#define parcs_low() PORTB&=~1
#define parcs_high() PORTB|=1
#define rd_low() PORTD&=~4
#define rd_high() PORTD|=4
#define wr_low() PORTD&=~8
#define wr_high() PORTD|=8

#define SS (1<<PB4)
#define MOSI (1<<PB5)
#define MISO (1<<PB6)
#define SCK (1<<PB7)

extern const FONT pal10_font;

#ifndef _NOP
#define _NOP() __asm__ __volatile__("nop")
#endif

class _screen : public VS23S010
{
protected:
    // SPI must be configured to  MSB first, MODE0
    virtual uint8_t spi_byte(uint8_t out)
    {
        SPDR=out;
        while (!(SPSR&(1<<SPIF)))
            ;
        return SPDR;
    }
    
    virtual void spi_select(bool onoff)
    {
        if (onoff) {
            spics_low();
        }
        else {
            spics_high();
        }
    }
    
public:    

    void init(void)
    {
        spics_high();
        DDRB &= ~MISO;
        DDRB |= SS + MOSI + SCK; // SS, MOSI and SCK as outputs
        SPCR=(1<<SPE)|(1<<MSTR); // clock/2
        SPSR=(1<<SPI2X);
        VS23S010::init();
    }
    
    uint8_t operator[](uint32_t i) { return mem_read_byte(i); }

} screen;
  
ISR(WDT_vect)
{
}

/*
I/O configuration
-----------------
I/O pin                               direction    DDR  PORT
PC0 D0                                input        0    1
PC1 D1                                input        0    1
PC2 D2                                input        1    1
PC3 D3                                output       1    1
PC4 keyboard data                     input        0    1
PC5 keyboard clock                    input        0    1

PD0 RxD                               input        0    1
PD1 TxD                               output       1    1
PD2 /RD                               output       1    1
PD3 /WR                               output       1    1
PD4 D4                                input        0    1
PD5 D5                                input        0    1
PD6 D6                                input        0    1
PD7 D7                                input        0    1

PB0 /CSPAR                            output       1    1
PB1 /RESET                            output       1    1
PB2 /CS                               output       1    1
PB3 MOSI                              output       1    1
PB4 MISO                              input        0    1
PB5 CLK                               output       1    1
*/

uint32_t seed=734518;

/* The state must be initialized to non-zero */
uint32_t xrandom()
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	seed = x;
	return x;
}

void serialout(uint8_t b)
{
  while (!(UCSR0A & _BV(UDRE0)));
  UDR0=b;
}    

void hexout(uint8_t b)
{
uint8_t c=(b>>4)+'0';
    if (c>'9')
        c+=7;
    serialout(c);
    c=(b&15)+'0';
    if (c>'9')
        c+=7;
    serialout(c);
}

int16_t slen(const char *s)
{
  int16_t l=0;
  while (s && *s++)
      l++;
  return l; 
}

void mandel(void)
{
    // from http://warp.povusers.org/Mandelbrot/
    float MinRe = -2.0;
    float MaxRe = 1.0;
    float MinIm = -1.2;
    float MaxIm = 1.2;
    float Re_factor = (MaxRe-MinRe)/((float)(screen.width-1));
    float Im_factor = (MaxIm-MinIm)/((float)(screen.height-1));
    uint16_t MaxIterations = 128;
    uint16_t n;
    for(int16_t y=0; y<screen.height; ++y) {
        float c_im = MaxIm - y*Im_factor;
        for(int16_t x=0; x<screen.width; ++x) {
            float c_re = MinRe + x*Re_factor;
            float Z_re = c_re;
            float Z_im = c_im;
            for(n=0; n<MaxIterations; ++n)
            {
                float Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;
                if(Z_re2 + Z_im2 > 4)
                    break;
                Z_im = 2*Z_re*Z_im + c_im;
                Z_re = Z_re2 - Z_im2 + c_re;
            }
            if (n!=MaxIterations)
                screen.set_pixel(x, y, 200 - n);
        }
    }
}

int main(void)
{

  MCUSR=0;
  MCUCR=0;
  // I/O directions
  DDRC=0x0c;
  DDRD=0x0e;
  DDRB=0x2f;
  // initial state
  PORTC=0x3f;
  PORTD=0xff;
  PORTB=0x3f;
  //
  // initialize UART
  UBRR0H=((F_CPU/(16UL*115200))-1)>>8;
  UBRR0L=((F_CPU/(16UL*115200))-1)&0xff;
  UCSR0B=0x18;   // enable rx,tx

  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  //
  reset_low();
  _delay_ms(1);
  reset_high();
  _delay_ms(1);
  sei();
  screen.init();
  // configure watchdog to interrupt&reset, 4 sec timeout
  //WDTCSR|=0x18;
  //WDTCSR=0xe8;

  screen.enable_color(true);
  
  _delay_ms(2000);

  uint8_t state=255;
  int16_t x1;
  int16_t y1;
  int16_t x2,y2,c,i;
  uint8_t beginc=' ';
  const char *title="Welcome to the show";
  while (1) {
    //sleep_cpu(); // timer interrupt wakes us up
    wdt_reset();
    WDTCSR|=0x40;
    screen.set_colors(15,0);
    screen.set_font(&pal10_font);
    screen.filled_rect(0,0,screen.width-1,screen.height-1,0);
    x1=slen(title)*8;
    x1=(screen.width-x1)>>1;
    screen.set_pos(x1,110);
    screen.puts(title);
    _delay_ms(1500);
    screen.filled_rect(0,0,screen.width-1,screen.height-1,0);    
    switch (state) {
        default:
            state=0;
            title="Palette";
            break;
        case 0:
            for (y1=0;y1<16;y1++) {
                for (x1=0;x1<16;x1++) {
                    screen.filled_rect(x1*20+1,y1*15+1,x1*20+17+1,y1*15+13+1,y1*16+x1);
                }
            }
            state++;
            title="White screen";
            break;
        case 1:
            screen.filled_rect(0,0,screen.width-1,screen.height-1,15);
            state++;
            title="Vertical bars";
            break;
        case 2:
            for (x1=0;x1<screen.width;x1+=32) {
                screen.filled_rect(x1,0,x1+31,screen.height-1,(x1&32)?15:0);
            }
            state++;
            title="Line mesh";
            break;
        case 3:
            for (x1=0;x1<screen.width;x1+=7) {
                screen.vline(x1,0,screen.height-1,15);
            }
            for (y1=0;y1<screen.height;y1+=7) {
                screen.hline(0,y1,screen.width-1,15);
            }
            state++;
            title="Random rectangles";
            break;
        case 4:
            for (i=0;i<1000;i++) {
                x1=xrandom()%screen.width;
                y1=xrandom()%screen.height;
                x2=x1+xrandom()%(screen.width-x1);
                y2=y1+xrandom()%(screen.height-y1);
                c=xrandom()&255;
                screen.filled_rect(x1,y1,x2,y2,c);
                wdt_reset();
                WDTCSR|=0x40;
            }
            state++;
            title="Strings and numbers";
            break;
        case 5:
            screen.set_pos(0,40);
            screen.set_font(&pal10_font);
            screen.puts("\r\nScreen width: ");
            screen.printn(screen.width);
            screen.puts("\r\nScreen height: ");
            screen.printn(screen.height);
            screen.puts("\r\nLine size: ");
            screen.printn(screen.linesize);
            screen.puts("\r\nPICLINE_START: ");
            screen.printn(PICLINE_START);
            screen.puts("\r\nPICLINE_TOTAL_BYTES: ");
            screen.printn(PICLINE_TOTAL_BYTES);
            screen.puts("\r\nSTARTPIX: ");
            screen.printn(STARTPIX);
            screen.puts("\r\nENDPIX: ");
            screen.printn(ENDPIX);
            
            state++;
            title="Slow scroll";
            break;
        case 6:
            #define NEXT(c) ((((c+1)-' ')%96)+' ') 
            c=beginc;
            screen.set_pos(0,0);
            for (y1=0;y1<23;y1++) {
                uint8_t cx=c;
                for (x1=0;x1<40;x1++) {
                    screen.putc(cx);
                    cx=NEXT(cx);
                }
                screen.puts("\r\n");
                c=NEXT(c);
            }
            beginc=NEXT(beginc);
            for (i=0;i<100;i++) {
                screen.scroll_down(1);
                wdt_reset();
                WDTCSR|=0x40;
            }
            for (i=0;i<100;i++) {
                screen.scroll_up(1);
                wdt_reset();
                WDTCSR|=0x40;
            }
            state++;
            title="Scrolling text";
            break;
        case 7:
            for (i=0;i<100;i++) {
                screen.set_pos(0,screen.height-11);
                c=beginc;
                for (y1=0;y1<1;y1++) {
                    uint8_t cx=c;
                    for (x1=0;x1<40;x1++) {
                        screen.putc(cx);
                        cx=NEXT(cx);
                    }
                    screen.puts("\r\n");
                    c=NEXT(c);
                }
                beginc=NEXT(beginc);
                screen.scroll_up(10);
            }
            state++;
            title="Vertical lines";
            break;
        case 8:
            for (i=1;i<screen.height;i+=2) {
                screen.vline(i+39,0,i,15);
            }
            state++;
            title="Horizontal lines";
            break;
        case 9:
            for (i=1;i<screen.height;i+=2) {
                screen.hline(0,i,i,15);
                screen.hline(screen.width-i-1,screen.height-i,screen.width-1,15);
            }
            state++;
            title="Widening rectangles";
            break;
        case 10:
            screen.filled_rect(20,10,20,screen.height-20,15);
            screen.filled_rect(60,10,61,screen.height-20,15);
            screen.filled_rect(100,10,102,screen.height-20,15);
            screen.filled_rect(140,10,143,screen.height-20,15);
            screen.filled_rect(180,10,184,screen.height-20,15);
            screen.filled_rect(220,10,225,screen.height-20,15);
            screen.filled_rect(260,10,266,screen.height-20,15);
            title="Mandelbrot";
            state++;
            break;
        case 11:
            mandel();
            state++;
            title="The end";
            break;
    }
    _delay_ms(4000);
  }
}
