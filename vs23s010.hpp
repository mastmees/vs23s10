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
#pragma once
#include <stdint.h>
#include "font.h"

#define noNTSC_VIDEO   // 320x200 noninterclaced NTSC
#define PAL_VIDEO    // 320x240 noninterlaced PAL

#include "vs23defines.hpp"

class VS23S010
{

private:

    void setlindex(uint16_t line, uint16_t addr);
    void setplindex(uint16_t line, uint32_t byteaddr, uint16_t protoaddr);
    void protoline(uint16_t line, uint16_t offset, uint16_t limit, uint16_t data);

protected:

    // address of character bitmaps of current font loaded into vram with appropriate line
    // spacing and address of character info (offset from vmemchars and char width)
    uint32_t vmemchars;
    uint32_t vcharinfo;
    
    // implement these platform specific methods in derived class
    // SPI must be configured to  MSB first, MODE0
    virtual uint8_t spi_byte(uint8_t out) = 0;
    virtual void spi_select(bool onoff) = 0;
    
    // these all rely on two virtuals above
    uint16_t spi_word(uint16_t out);
    void spi_write_program(uint32_t data);
    uint16_t mem_write_word(uint32_t addr,uint16_t data);
    uint8_t mem_write_byte(uint32_t addr,uint8_t data);
    uint8_t mem_read_byte(uint32_t addr);
    uint8_t reg_byte(uint8_t regop,uint8_t data);
    uint8_t reg_word(uint8_t regop,uint16_t data);
    void blitter_op(uint32_t src, uint8_t w, uint8_t h, uint32_t dst, uint8_t backwards);

public:

    // screen size parameters
    const int16_t width = XPIXELS;
    const int16_t height = YPIXELS;
    // number of bytes in memory used by each visible scanline
    const int16_t linesize = (PICLINE_BYTE_ADDRESS(1)-PICLINE_BYTE_ADDRESS(0));
    // current foreground and background colors
    uint8_t fgcolor,bgcolor;
    // text output position (upper left corner)
    int16_t cursorx,cursory;
    // currently used font
    const FONT* current_font;

    // when the object is constructed, the subclass constructor has not
    // yet run, so no SPI operations in base class constructor allowed    
    VS23S010();

    inline void enable_color(bool yesno)
    {
        protoline(0,BURST,BURSTDUR,yesno?BURST_LEVEL:BLANK_LEVEL);
    }

    inline bool block_move_active()
    {
        return (bool)(reg_word(CURLINE,0x0000)&CURLINE_MVBS);
    }
    
    inline void set_colors(uint8_t fg,uint8_t bg) { fgcolor=fg; bgcolor=bg; }
    inline void set_pos(int16_t x,int16_t y) { cursorx=x; cursory=y; }

    void init();
    // graphics primitives
    #define hline(x1,y,x2,color) filled_rect(x1,y,x2,y,color)
    void set_pixel(int16_t x, int16_t y, uint8_t color);
    void vline(int16_t x,int16_t y1,int16_t y2,uint8_t color);
    void line(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint8_t color);
    void rect(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint8_t color);
    void filled_rect(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint8_t color);
    // text rendering
    uint8_t char_width(uint8_t c,const FONT* font);
    int16_t blitchar(uint8_t c,int16_t x,int16_t y,const FONT* font);
    int16_t vblitchar(uint8_t c,int16_t x,int16_t y);
    void set_font(const FONT* font);    
    int16_t putc(uint8_t c);
    int16_t puts(char *s);
    int16_t puts(const char *s);
    int16_t printn(int32_t n);
    void scroll_up(int16_t lines);
    void scroll_down(int16_t lines);    
};
