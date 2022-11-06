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
#include "vs23s010.hpp"
#include <util/delay.h>


// enable low-pass filter on luma for less color fringing
// this introduces ugly spikes at start and end of pixel data
// and as the spike in the start is in positive direction there
// is a visible light line at the edge of visible pixel area.
// this is super annoying. even more so as the filter does not
// remove the color fringing completely.
// define to 0 to disable the filtering

#define LUMAFILTER  (BLOCKMVC1_PYF)

// this is empty placeholder font that gets used until
// real font is loaded. it just just one character defined.

static uint8_t const emptychar[] PROGMEM = {
  0xfe,
  0x82,
  0x82,
  0x82,
  0x82,
  0x82,
  0x82,
  0xfe,
  0x00,
  0x00
};

static const FONT emptyfont = {
    .firstchar = 32,
    .lastchar = 32,
    .height = 10,
    .width = 8,
    .widths_P = NULL,
    .bitmaps_P = {emptychar}
};

VS23S010::VS23S010() : vmemchars(0), vcharinfo(0), fgcolor(15), 
                        bgcolor(0), cursorx(0), cursory(0)
{
    current_font=&emptyfont;
}

// wrtie a line's pixel data start address to screen line index table
void VS23S010::setlindex(uint16_t line, uint16_t addr)
{
    uint32_t ia=INDEX_START_BYTES+(line*3);
    mem_write_byte(ia++,0);
    mem_write_byte(ia++,addr&255);
    mem_write_byte(ia++,addr>>8);
}

// set picture line indexes to point to proto line and image data
void VS23S010::setplindex(uint16_t line, uint32_t byteaddr, uint16_t protoaddr)
{
    uint32_t ia=INDEX_START_BYTES + (line*3);
    mem_write_byte(ia++,((byteaddr << 7) & 0x80) | (protoaddr & 0xf));
    mem_write_byte(ia++,(byteaddr >> 1));
    mem_write_byte(ia++,(byteaddr >> 9));
}

// write limit number of data words to given protoline starting at offset
void VS23S010::protoline(uint16_t line, uint16_t offset, uint16_t limit, uint16_t data)
{
    uint16_t i=0;
    uint32_t w=PROTOLINE_WORD_ADDRESS(line) + offset;
    while (i<=limit) {
        mem_write_word((uint16_t)w++, data);
        i++;
    }
}

// low level SPI interface and command helpers
// these all rely on externally overloaded spi_select() and spi_byte() virtuals
uint16_t VS23S010::spi_word(uint16_t out)
{
    uint16_t w;
    w=spi_byte(out>>8);
    w<<=8;
    w|=spi_byte(out&255);
    return w;
}

void VS23S010::spi_write_program(uint32_t data)
{
    spi_select(true);
    spi_byte(PROGRAM);
    spi_word(data>>16);
    spi_word(data&0xffff);
    spi_select(false);
}

uint16_t VS23S010::mem_write_word(uint32_t addr,uint16_t data)
{
    uint16_t w;
    spi_select(true);
    spi_byte(WRITE);
    addr<<=1;
    spi_byte(addr>>16);
    spi_word(addr);
    w=spi_word(data);
    spi_select(false);
    return w;
}

uint8_t VS23S010::mem_write_byte(uint32_t addr,uint8_t data)
{
    uint8_t b;
    spi_select(true);
    spi_byte(WRITE);
    spi_byte(addr>>16);
    spi_word(addr);
    b=spi_byte(data);
    spi_select(false);
    return b;
}

uint8_t VS23S010::mem_read_byte(uint32_t addr)
{
    uint8_t b;
    spi_select(true);
    spi_byte(READ);
    spi_byte(addr>>16);
    spi_word(addr);
    b=spi_byte(0);
    spi_select(false);
    return b;
}


uint8_t VS23S010::reg_byte(uint8_t regop,uint8_t data)
{
    uint8_t b;
    spi_select(true);
    spi_byte(regop);
    b=spi_byte(data);
    spi_select(false);
    return b;
}

uint8_t VS23S010::reg_word(uint8_t regop,uint16_t data)
{
    uint16_t w;
    spi_select(true);
    spi_byte(regop);
    w=spi_word(data);
    spi_select(false);
    return w;
}

// much of initialization magic comes directly from VLSI forum posts
// although a bit optimized, cleaned up and made switchable between
// PAL and NTSC

void VS23S010::init()
{
    uint16_t i;
    reg_byte(WRITE_MULTIIC,0xe); // only leave chip 0 enables in case of
                                 // multichip setup
    reg_byte(WRITE_STATUS,0x40); // memory access to autoincrementing
                                 // sequential mode
    reg_word(PICSTART,(STARTPIX-1)); // left and right limit of visible
    reg_word(PICEND,(ENDPIX-1));     // screen area
    // enable PLL clock
    reg_word(VDCTRL1,(VDCTRL1_PLL_ENABLE)|(VDCTRL1_SELECT_PLL_CLOCK));
    // Clear memory by filling it with 0. Memory is 65536 16-bit words, and first 24-bits
    // are used for the starting address. The address then autoincrements when the zero 
    // data is being sent.
    // this also clears all protolines, setting them to SYNC_LEVEL which
    // is always 0
    spi_select(true);
    spi_byte(WRITE);
    for (uint32_t a=0; a<65539; a++) {
       spi_word(0); // Address and data.
    }
    spi_select(false);
    // Set length of one complete line (in PLL (VClk) clocks). 
    // Does not include the fixed 10 cycles of sync level at the beginning 
    // of the lines. 
    reg_word(LINELEN,PLLCLKS_PER_LINE);
    // Set microcode program for picture lines. Each OP is one VClk cycle.
    spi_write_program(MICROCODE);
    // Define where Line Indexes are stored in memory
    reg_word(INDEXSTART,INDEX_START_LONGWORDS);
    // Set all line indexes to point to protoline 0 (which by definition
    // is in the beginning of the SRAM)
    for (i=0; i<TOTAL_LINES; i++) {
       setlindex(i,PROTOLINE_WORD_ADDRESS(0));
    }
    // At this time, the chip would continuously output the proto line 0.
    // This protoline will become our most "normal" horizontal line.
    // For TV-Out, fill the line with black level,
    // and insert a few pixels of sync level (0) and color burst to the beginning.
    // Note that the chip hardware adds black level to all nonproto areas so
    // protolines and normal picture have different meaning for the same Y value.
    // In protolines, Y=0 is at sync level and in normal picture Y=0 is at black level (offset +102).

    // In protolines, each pixel is 8 PLLCLKs, which in TV-out modes means one color
    // subcarrier cycle. Each pixel has 16 bits (one word): VVVVUUUUYYYYYYYY.

    for (i=0;i<PROTOLINES;i++) {
        protoline(0,0,COLORCLKS_PER_PROTO_LINE,BLANK_LEVEL);
    }
    protoline(0,0,SYNC_DUR,SYNC_LEVEL);
#ifdef NTSC_VIDEO
    protoline(0,BLANKEND,STARTPIX-BLANKEND,BLACK_LEVEL);
#endif
    enable_color(true);
    // protoline 1, short+short VSYNC line
    protoline(1,0,SHORTSYNC,SYNC_LEVEL);
    protoline(1,COLORCLKS_PROTO_LINE_HALF,SHORTSYNCM,SYNC_LEVEL);
    // protoline 2, long+long VSYNC line
    protoline(2,0,LONGSYNC,SYNC_LEVEL);
    protoline(2,COLORCLKS_PROTO_LINE_HALF,LONGSYNCM,SYNC_LEVEL);
    
#ifdef PAL_VIDEO
    // extra protoline for progressive PAL
    // protoline 3, long+short VSYNC line
    protoline(3,0,LONGSYNC,SYNC_LEVEL);
    protoline(3,COLORCLKS_PROTO_LINE_HALF,SHORTSYNCM,SYNC_LEVEL);
    // now build frame beginning and end
    setlindex(0,PROTOLINE_WORD_ADDRESS(2));
    setlindex(1,PROTOLINE_WORD_ADDRESS(2));
    setlindex(2,PROTOLINE_WORD_ADDRESS(3));
    setlindex(3,PROTOLINE_WORD_ADDRESS(1));
    setlindex(4,PROTOLINE_WORD_ADDRESS(1));        
    // These are three last lines of the frame, lines 310-312
    setlindex(TOTAL_LINES-3,PROTOLINE_WORD_ADDRESS(1));
    setlindex(TOTAL_LINES-2,PROTOLINE_WORD_ADDRESS(1));
    setlindex(TOTAL_LINES-1,PROTOLINE_WORD_ADDRESS(1));
#endif

#ifdef NTSC_VIDEO
    setlindex(0,PROTOLINE_WORD_ADDRESS(1));
    setlindex(1,PROTOLINE_WORD_ADDRESS(1));
    setlindex(2,PROTOLINE_WORD_ADDRESS(1));
    setlindex(3,PROTOLINE_WORD_ADDRESS(1));
    setlindex(4,PROTOLINE_WORD_ADDRESS(2));
    setlindex(5,PROTOLINE_WORD_ADDRESS(2));
    setlindex(6,PROTOLINE_WORD_ADDRESS(2));
    setlindex(7,PROTOLINE_WORD_ADDRESS(1));
    setlindex(8,PROTOLINE_WORD_ADDRESS(1));
    setlindex(9,PROTOLINE_WORD_ADDRESS(1));
#endif
    // Set pic line indexes to point to protoline 0 and their individual picture line.
    for (i=0; i<ENDLINE-STARTLINE; i++) {
        setplindex(i+STARTLINE,PICLINE_BYTE_ADDRESS(i),0);
    }
    spi_select(true);
    spi_byte(BLOCKMVC1);
    spi_word(0);
    spi_word(0);
    spi_byte(LUMAFILTER);
    spi_select(false);
    // Enable Video Display Controller, set video mode,program length and line count
    reg_word(VDCTRL2, 
        VDCTRL2_ENABLE_VIDEO |
#ifdef NTSC_VIDEO
        VDCTRL2_NTSC |
#endif
#ifdef PAL_VIDEO
        VDCTRL2_PAL |
#endif
        VDCTRL2_PROGRAM_LENGTH |
        VDCTRL2_LINECOUNT);
}

// simplest one, set one pixel at coordinates to desired color
//
void VS23S010::set_pixel(int16_t x, int16_t y, uint8_t color)
{
    if ((x<0) || (x>(XPIXELS-1)) || (y<0) || (y>(YPIXELS-1)))
        return;
    uint32_t addr=PICLINE_BYTE_ADDRESS(y)+x;
    mem_write_byte(addr,color);
}

// the block moving feature seems to be one very sick puppy,
// it fails if asked to copy less than 4 bytes, looks like it cannot copy
// overlapping regions if the regions overlap by just a single pixel and who
// knows what other failure scenarios it has. This needs further work to see what
// can it actually do correctly. 
// for verification that the problem is block mover related, a very slow software
// only alternative is also available. currently few different functions work around
// the issues separately, once it is clear what works and what not, I can maybe
// do all workarounds in this function
//
#define HARDWARE_BLITTER
void VS23S010::blitter_op(uint32_t src, uint8_t w, uint8_t h, uint32_t dst, uint8_t backwards)
{
#ifndef HARDWARE_BLITTER
    if (!backwards) {
        for (int16_t i=0;i<h;i++) {
            for (int16_t j=0;j<w;j++) {
                uint8_t b=mem_read_byte(src++);
                mem_write_byte(dst++,b);
            }
            src+=linesize-w;
            dst+=linesize-w;
        }
    }
    else {
        for (int16_t i=0;i<h;i++) {
            for (int16_t j=0;j<w;j++) {
                uint8_t b=mem_read_byte(src--);
                mem_write_byte(dst--,b);
            }
            src-=linesize-w;
            dst-=linesize-w;
        }
    }
#else
    spi_select(true);
    spi_byte(BLOCKMVC1);
    spi_word(src>>1);
    spi_word(dst>>1);
    spi_byte(
        ((src&1)<<2) |
        ((dst&1)<<1) |
        LUMAFILTER |
        (backwards?1:0)); // move direction, 1 is backwards
    spi_select(false);    
    spi_select(true);
    spi_byte(BLOCKMVC2);
    spi_word(linesize-w);
    spi_byte(w);
    spi_byte(h-1);
    spi_select(false);
    // accouring to VLSI forum, the block move paramters are
    // shadowed, so we can do everything up to this point before checking
    // that the previous move has ended. This checking would be more
    // effective with hardware not no free pins on AVR and I would also
    // like to see if SPI only can do it
    while (block_move_active());
    spi_select(true);
    spi_byte(BLOCKMVST);
    spi_select(false);
#endif
}

// filled rectangle drawing first clips the rectangle into visual area
// then draws the top line of the rectangle, and if the rectangle is wider
// than 7 pixels then uses hardware block mover to copy the line down, one
// line at the time.The method currently used has been tested
// and seems to work consistently.
//
void VS23S010::filled_rect(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint8_t color)
{
    if (y1<0 || y1>(height-1) || x1>(width-1))
        return;
    if (x1<0)
        x1=0;
    if (x2>(width-1))
        x2=width-1;
    if (x1>x2)
        return;
    uint32_t addr=PICLINE_BYTE_ADDRESS(y1)+x1;
    int16_t w=x2-x1+1;
    x1=w;
    spi_select(true);
    spi_byte(WRITE);
    spi_byte(addr>>16);
    spi_word(addr);
    while (x1--) {
        spi_byte(color);
    }
    spi_select(false);
    y1++;
    // a single blitter operation is 3 command bytes + 9 data bytes
    // setting up for pixel store is 1 command byte + 3 address bytes
    // so anything up to 8 pixels is cheaper to do without blitter   
    if (w<8) {
        while (y1<=y2) {
            x1=w;
            addr+=linesize;
            spi_select(true);
            spi_byte(WRITE);
            spi_byte(addr>>16);
            spi_word(addr);
            while (x1--) {
                spi_byte(color);
            }
            spi_select(false);
            y1++;
        }
        return;
    }
    while (y1<=y2) {
        x1=0;
        while (x1<w) {
            x2=((w-x1)<250)?w-x1:250;
            blitter_op(addr+x1,x2,1,addr+x1+linesize,0);
            x1+=x2;
        }
        y1++;
        addr+=linesize;
    }
}

// vertical line drawing could also be much more efficient if the
// hardware block mover would be able to do it, but as its one
// pixel wide, cannot do.
//
void VS23S010::vline(int16_t x,int16_t y1,int16_t y2,uint8_t color)
{
    if ((x<0) || x>(width-1) || y1>(height-1))
        return;
    if (y1<0)
        y1=0;
    if (y2>(height-1))
        y2=(height-1);
    if (y1>y2)
        return;
    uint32_t addr=PICLINE_BYTE_ADDRESS(y1)+x;
    while (y1<=y2) {
        mem_write_byte(addr,color);
        addr+=linesize;
        y1++;
    }
}

#define abs(x) (x<0?-x:x)

// generic line drawing. this uses set_pixel for every pixels, and is much
// slower than horizontal or vertial line drawing, hence the checks in
// the beginning    
void VS23S010::line(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint8_t color)
{
  if (x1==x2) {
    vline(x1,y1,y2,color);
    return;
  }
  if (y1==y2) {
    hline(x1,y1,x2,color);
    return;
  }
  int16_t dx=abs(x2-x1);
  int16_t sx=(x1<x2)?1:-1;
  int16_t dy=-abs(y2-y1);
  int16_t sy=(y1<y2)?1:-1;
  int16_t err=dx+dy,e2;
  while (1) {
    set_pixel(x1,y1,color);
    if (x1==x2 && y1==y2)
      break;
    e2=2*err;
    if (e2>=dy) {
      err+=dy;
      x1+=sx;
    }
    if (e2<=dx) {
      err+=dx;
      y1+=sy;
    }
  }
}


void VS23S010::rect(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint8_t color)
{
  hline(x1,y1,x2,color);
  hline(x1,y2,x2,color);
  vline(x1,y1,y2,color);
  vline(x2,y1,y2,color);
}


// this looks up the width of given character in pixels from given font
// of font does not have the character defined, then a 0 width is returned
uint8_t VS23S010::char_width(uint8_t c,const FONT* font)
{
    if ((!font) || (c<font->firstchar) || (c>font->lastchar)) {
        return 0;
    }
    c-=font->firstchar;
    return (font->width)?font->width:pgm_read_byte(&font->widths_P[c]);
}

// this draws a character from font data that is in flash memory
// to screen at specified location. x,y are coordinates of top left
// corner of character cell. current foreground and background colors
// are used, if the background color is set the same as foreground then
// background pixels are not drawn, preserving existing background.
int16_t VS23S010::blitchar(uint8_t c,int16_t x,int16_t y,const FONT* font)
{
    if ((!font) || (c<font->firstchar) || (c>font->lastchar)) {
        return x;
    }
    uint8_t w=char_width(c,font);
    if (!w)
        return x;
    c-=font->firstchar;
    uint8_t h=font->height;
    const uint8_t *bits_P;
    if (font->width)
        bits_P=&(font->bitmaps_P[0][(((w+7)>>3)*(uint16_t)h*(uint16_t)c)]);
    else
        bits_P=font->bitmaps_P[c];
    while (h-- && y<height) {
        int16_t cx=x;
        uint8_t bit=0;
        for (uint8_t i=0;i<w;i++) {
            if (!bit) {
                c=pgm_read_byte(bits_P);
                bits_P++;
                bit=8;
            }
            if (c&0x80)
                set_pixel(cx,y,fgcolor);
            else
                if (fgcolor!=bgcolor)
                    set_pixel(cx,y,bgcolor);
            cx++;
            c<<=1;
            bit--;
        }
        y++;
    }
    return x+w;
}

// this draws a character from currently set font to specified
// screen location, using harware block move function. it is very likely
// that it fails with character widths less than 4, but we currently only
// use wixed with 8 pixel wide characters, and these seem to work ok
int16_t VS23S010::vblitchar(uint8_t c,int16_t x,int16_t y)
{
    if ((c<current_font->firstchar) || (c>current_font->lastchar)) {
        return x;
    }
    c-=current_font->firstchar;
    uint32_t src=vcharinfo+(uint16_t)c*3;
    uint8_t w=mem_read_byte(src++);
    uint16_t offs=mem_read_byte(src++)<<8;
    offs|=mem_read_byte(src);
    src=vmemchars+offs;
    uint32_t dst=PICLINE_BYTE_ADDRESS(y)+x;
    blitter_op(src, w, current_font->height, dst, 0);
    return x+w;
}

// this transfers font character data to video ram, starting after the
// last visible screen line. all defined characters will be pre-rendered
// with currently set foreground and background color (no transparency
// here, block mover cannot do it). block mover also needs the characters
// the same as whay would be rendered on screen, scattered across sequential
// "screen lines", so the prenrendering keeps track of its current 'row' of
// characters, and if screen width limit is reached, creates a new row.
// once the font is set up this way, the accelerated vblitchar can be used
// to copy these invisible character cells to visible screen, and it is
// much faster that doing to pixel by pixel
void VS23S010::set_font(const FONT* font)
{
    if (font)
        current_font=font;
    else
        current_font=&emptyfont;
    vcharinfo=PICLINE_BYTE_ADDRESS(height);
    vmemchars=vcharinfo+(3*256); // reserve space for max number of charinfos
    uint8_t w=current_font->width;
    int16_t x=0;
    uint16_t row=0;
    uint16_t offs;
    uint16_t cc=((uint16_t)current_font->lastchar-current_font->firstchar+1);
    for (uint16_t i=0;i<cc;i++) {
        if (!current_font->width) {
            w=pgm_read_byte(&current_font->widths_P[i]);
        }
        if ((x+w)>=width) {
            row++;
            x=0;
        }
        offs=row*current_font->height*linesize+x;
        mem_write_byte(vcharinfo+i*3,w);
        mem_write_byte(vcharinfo+i*3+1,offs>>8);
        mem_write_byte(vcharinfo+i*3+2,offs&255);

        // make pointer to bitmap of character data
        const uint8_t *bits_P;
        uint8_t h=current_font->height;
        if (current_font->width)
            bits_P=&(current_font->bitmaps_P[0][(((w+7)>>3)*(uint16_t)h*i)]);
        else
            bits_P=font->bitmaps_P[i];
        // expand bitmap into pixel image past visible area in vram
        // with correct line skip values so that blitter can be used
        // to copy these to screen
        while (h--) {
            uint8_t bit=0;
            uint8_t c=0;
            for (uint8_t j=0;j<w;j++) {
                if (!bit) {
                    c=pgm_read_byte(bits_P);
                    bits_P++;
                    bit=8;
                }
                mem_write_byte(vmemchars+offs,((c&0x80)?fgcolor:bgcolor));
                offs++;
                c<<=1;
                bit--;
            }
            offs+=linesize-w;
        }
        x+=w;
    }
}

// elementary teletype style terminal, just
// knows how to go to start of line and next line
// just a primitive beginning for now
int16_t VS23S010::putc(uint8_t c)
{
    switch (c) {
        case 10:
            cursory+=current_font->height;
            return cursorx;
        case 13:
            cursorx=0;
            return cursorx;
    }
    if ((c<current_font->firstchar) || (c>current_font->lastchar))
        cursorx=blitchar(emptyfont.firstchar,cursorx,cursory,&emptyfont);
    else {
        if (vmemchars)
            cursorx=vblitchar(c,cursorx,cursory);
        else
            cursorx=blitchar(c,cursorx,cursory,current_font);
    }
    return cursorx;
}

// two different versions of string output here. did you also notice the
// pgm_read_byte() calls above? Thank the geniuses who thought pure 
// Harward architecture was a brilliant idea. Look it up on Google, and
// you'll get this - "It is modern computer architecture based on Harvard 
// Mark I relay based model." Very modern indeed, you cannot even access
// your data with the same instructions in flash and ram
//
int16_t VS23S010::puts(char *s)
{
    while (s && *s) {
        putc(*s++);
    }
    return cursorx;
}

int16_t VS23S010::puts(const char *s)
{
    while (s && *s) {
        putc(*s++);
    }
    return cursorx;
}

int16_t VS23S010::printn(int32_t n)
{
    if (n<0) {
        putc('-');  
        n=0-n;
    }
    if (n>9)
        printn(n/10);
    return putc((n%10)+'0');
}


// because of hardware block move not playing nice, the scrolling functions
// have to do separate block move operation for every line. Or tather, two
// for every line because the length of the block is limited to 255 which is
// smaller that screen width. if would be massively quicker to move entire screen
// in two moves.

void VS23S010::scroll_up(int16_t lines)
{
    uint32_t src=PICLINE_BYTE_ADDRESS(lines);
    uint32_t dst=PICLINE_BYTE_ADDRESS(0);
    int16_t c=height-lines;
    while (c--) {
        blitter_op(src,width>>1,1,dst,0);
        blitter_op(src+(width>>1),width-(width>>1),1,dst+(width>>1),0);
        src+=linesize;
        dst+=linesize;
    }
    filled_rect(0,height-lines,width-1,height-1,bgcolor);
}

void VS23S010::scroll_down(int16_t lines)
{
    uint32_t src=PICLINE_BYTE_ADDRESS(height-lines-1);
    uint32_t dst=PICLINE_BYTE_ADDRESS(height-1);
    int16_t c=height-lines;
    while (c--) {
        blitter_op(src,width>>1,1,dst,0);
        blitter_op(src+(width>>1),width-(width>>1),1,dst+(width>>1),0);
        src-=linesize;
        dst-=linesize;
    }
    filled_rect(0,0,width-1,lines-1,bgcolor);
}
