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

// Most of the witchcraft below is collected from various code fragments posted in
// VLSI Solution forum. Most notably, the color creation parameters and microcode
// which came with this suggestion:
//
// To promote software and artwork portability, this highly versatile palette,
// wherever possible, shall be considered the default 256-color palette of
// VS23S010D. It is recommended that all circuit boards and software examples
// that use 8-bit colors, shall configure the board to this color mode and
// use this palette by default.
// VS23S010D Default PAL Palette: P:EE:A22:B22:Y44:N10; Colorburst 0xEE, Microcode C0 9C 4A 0A.
// VS23S010D Default NTSC Palette: N:0D:B22:A22:Y44:N10; Colorburst 0x0D, Microcode C0 9C 0A 4A.
// To decrease the color intensity (saturation), increase the color burst's amplitude.
// A low intensity color version of this palette would be P:BB:A22:B22:Y44:N10 for PAL
// and N:09:B22:A22:Y44:N10 for NTSC.

// General VS23 commands
#define WRITE_STATUS    0x01 // Write Status Register
#define WRITE           0x02 // Write SRAM
#define READ            0x03 // Read SRAM
#define READ_STATUS     0x05 // Read Status Register
#define WRITE_MULTIIC   0xb8 // Write Multi-IC Access Control
#define READ_MULTIIC    0xb7 // Read Multi-IC Access Control
#define READ_ID         0x9f // Read Manufacturer and Device ID
// VS23 video commands
#define PROGRAM         0x30
#define PICSTART        0x28
#define PICEND          0x29
#define LINELEN         0x2a
#define YUVBITS         0x2b
#define INDEXSTART      0x2c
#define LINECFG         0x2d
#define VTABLE          0x2e
#define UTABLE          0x2f
#define BLOCKMVC1       0x34
#define BLOCKMVC2       0x35
#define BLOCKMVST       0x36
#define CURLINE         0x53

#define GPIOCTL         0x82

// Bit definitions
#define VDCTRL1             0x2B
#define VDCTRL1_UVSKIP      (1<<0)
#define VDCTRL1_DACDIV      (1<<3)
#define VDCTRL1_PLL_ENABLE  (1<<12)
#define VDCTRL1_SELECT_PLL_CLOCK (1<<13)
#define VDCTRL1_USE_UVTABLE (1<<14)
#define VDCTRL1_DIRECT_DAC  (1<<15)
#define VDCTRL2             0x2D
#define VDCTRL2_NTSC        (0<<14)
#define VDCTRL2_PAL         (1<<14)
#define VDCTRL2_ENABLE_VIDEO (1<<15)
#define LINELEN_VGP_OUTPUT  (1<<15)

#define CURLINE_MVBS        (1<<14)
#define BLOCKMVC1_PYF       (1<<4)
#define BLOCKMVC1_DACC      (1<<3)

// pattern generator microcode helpers
// Bits 7:6
#define PICK_V (0<<6)       // 00=a (V)
#define PICK_U (1<<6)       // 01=b (U)
#define PICK_Y (2<<6)       // 10=y (Y)
#define PICK_NOTHING (3<<6) // 11=-
// Bits 5:3, pick 1..8
#define PICK_BITS(a) (((a)-1)<<3) 
// Bits 2:0, Shift 0..6
#define SHIFT_BITS(a) (a)



#ifdef PAL_VIDEO
    #define YPIXELS 240
    #define XPIXELS 320
    #define TOTAL_LINES 313
    #define VSYNCLINES 8
    #define XTAL_MHZ 4.43361875
    #define LINE_LENGTH_US 64.0
    // PAL blanking is 12.05 us. this is sync+back porch+visual info+front
    // porch where
    // front porch (before sync) is 1.65 us
    // sync is 4.7us
    // back porch 7.3 us
    // visual info is 52us
    //
    // color burst duration is 2.25 us
    // it starts 5.6 us from sync start
    // line blanking duration is 12 us
    // from sync start to line blanking end is 10.5 us 
    
    #define SYNC_US 0.0 // chip times everything from sync start
    #define SYNC_DUR_US 4.7
    #define BURST_US 5.6 // Color burst starts at 5.6 us
    #define BURST_DUR_US 2.25 // Color burst duration is 2.25 us
    #define BLANK_END_US 10.5 // sync to blanking end time is 10.5 us
    #define FRPORCH_US 62.35 // Front porch starts at the end of the line, at 62.35us

    #define SHORT_SYNC_US 2.35
    #define LONG_SYNC_US 27.3

    #define PLLCLKS_PER_PIXEL 5    // Width, in PLL clocks, of each pixel
    // On which line the picture area begins, the Y direction.

    #define BEXTRA 3 // if pic-to-proto border atrifacts occur, try 8
    #define PROTOLINES 4  // lines are used for sync timing, porch and border area

    #define BLANK_LEVEL 0x5b //0x005b // 43 IRE
    #define BLACK_LEVEL 0x5b //0x005b
    #define BURST_LEVEL 0x2e5b // 2e5b   
    #define WHITE_LEVEL 0x00FF // 143 IRE should be 0x12e 

    #define UBITS 2
    #define VBITS 2
    #define YBITS 4
    
    /// 8 bits per pixel, U2 V2 Y4
    // #define OP1 (uint32_t)(PICK_U + PICK_BITS(UBITS) + SHIFT_BITS(UBITS))
    // #define OP2 (uint32_t)(PICK_V + PICK_BITS(VBITS) + SHIFT_BITS(VBITS))
    // #define OP3 (uint32_t)(PICK_Y + PICK_BITS(YBITS) + SHIFT_BITS(YBITS))
    // #define OP4 (uint32_t)(PICK_NOTHING)
    #define OP4 (uint32_t)0xC0
    #define OP3 (uint32_t)0x9C
    #define OP2 (uint32_t)0x4A
    #define OP1 (uint32_t)0x0A
#endif


#ifdef NTSC_VIDEO

// NTSC is completely untested for now

    #define YPIXELS 200
    #define XPIXELS 320
    #define VSYNCLINES 10
    #define XTAL_MHZ 3.579545 // Crystal frequency in MHZ
    #define LINE_LENGTH_US 63.5555 // Line length in us
    #define TOTAL_LINES 263    // Frame length in lines (visible+nonvisible)
                              // Amount has to be odd for NTSC and RGB colors
    #define SYNC_US 0.0       // chip times everything from sync start
    #define SYNC_DUR_US 4.7  // Normal visible picture line sync length is 4.7 us
    #define SHORT_SYNC_US 2.542 // NTSC short sync duration is 2.35 us
    #define LONG_SYNC_US 27.33275 // NTSC long sync duration is 27.3 us
    #define BURST_US 5.3 // Color burst starts at 5.6 us
    #define BURST_DUR_US 2.67 // Color burst duration is 2.25 us
    #define BLANK_END_US 9.155 // NTSC sync to blanking end time is 10.5 us
    #define FRPORCH_US 61.8105 // Front porch starts at the end of the line, at 62.5us

    #define PLLCLKS_PER_PIXEL 5    // Width, in PLL clocks, of each pixel
    // On which line the picture area begins, the Y direction.

    #define BEXTRA 0  // try 8 if pic-to-proto border artifacts occur
    #define PROTOLINES 3  // lines are used for sync timing, porch and border area

    // These are for proto lines and so format is VVVVUUUUYYYYYYYY 
    // 285 mV to 75 ohm load
    #define BLANK_LEVEL 0x0d66 // 0x0066
    // 339 mV to 75 ohm load
    #define BLACK_LEVEL 0x0066
    // Color burst
    #define BURST_LEVEL (0x0D00 + BLACK_LEVEL)
    #define WHITE_LEVEL 0x00ff
    
    #define UBITS 2
    #define VBITS 2
    #define YBITS 4
    
    //#define OP1 (unsigned long)(PICK_U + PICK_BITS(UBITS) + SHIFT_BITS(UBITS))
    //#define OP2 (unsigned long)(PICK_V + PICK_BITS(VBITS) + SHIFT_BITS(VBITS))
    //#define OP3 (unsigned long)(PICK_Y + PICK_BITS(YBITS) + SHIFT_BITS(YBITS))
    //#define OP4 (unsigned long)(PICK_NOTHING)
    #define OP4 (uint32_t)0xC0
    #define OP3 (uint32_t)0x9C
    #define OP2 (uint32_t)0x0A
    #define OP1 (uint32_t)0x4A

#endif

#define SYNC_LEVEL 0x0000
#define PLL_MHZ (XTAL_MHZ * 8.0) // PLL output frequency
#define PROTO_PLL_CLKS 8.0 // in protolines, pixels are always 8 PLL clocks

// 10 first pllclks, which are not in the counters are decremented here
#define PLLCLKS_PER_LINE ((uint16_t)((LINE_LENGTH_US * PLL_MHZ)+0.5-10))
#define COLORCLKS_PER_PROTO_LINE ((uint16_t)(LINE_LENGTH_US * XTAL_MHZ+0.5-10.0/PROTO_PLL_CLKS))
#define COLORCLKS_PROTO_LINE_HALF ((uint16_t)(LINE_LENGTH_US * XTAL_MHZ/2+0.5-10.0/PROTO_PLL_CLKS))
#define SYNC_DUR ((uint16_t)(SYNC_DUR_US*XTAL_MHZ-10.0/PROTO_PLL_CLKS))
#define BURST ((uint16_t)(BURST_US*XTAL_MHZ-10.0/PROTO_PLL_CLKS))
#define BURSTDUR ((uint16_t)(BURST_DUR_US*XTAL_MHZ))
#define BLANKEND ((uint16_t)(BLANK_END_US*XTAL_MHZ-10.0/PROTO_PLL_CLKS))
#define SHORTSYNC ((uint16_t)(SHORT_SYNC_US*XTAL_MHZ-10.0/PROTO_PLL_CLKS))
// For the middle of the line the whole duration of sync pulse is used.
#define SHORTSYNCM ((uint16_t)(SHORT_SYNC_US*XTAL_MHZ))
#define LONGSYNC ((uint16_t)(LONG_SYNC_US*XTAL_MHZ))
#define LONGSYNCM ((uint16_t)(LONG_SYNC_US*XTAL_MHZ))

/// Protoline length is the real lenght of protoline (optimal memory 
/// layout, but visible lines' prototype must always be proto 0)
#define PROTOLINE_LENGTH_WORDS ((uint16_t)(LINE_LENGTH_US*XTAL_MHZ+0.5))
#define PROTOLINE_BYTE_ADDRESS(n) (PROTOLINE_LENGTH_WORDS) * 2 *(n)) // 512 * 2 * n = 1024*n
#define PROTOLINE_WORD_ADDRESS(n) (PROTOLINE_LENGTH_WORDS * (n)) // 512 * n = 512*n
#define PROTO_AREA_WORDS (PROTOLINE_LENGTH_WORDS * PROTOLINES) 
#define INDEX_START_LONGWORDS ((PROTO_AREA_WORDS+1)/2)
#define INDEX_START_WORDS (INDEX_START_LONGWORDS * 2)
#define INDEX_START_BYTES (INDEX_START_WORDS * 2)

#define PIXEL_TIME (1.0/PLL_MHZ*PLLCLKS_PER_PIXEL)
#define VISIBLE_TIME (FRPORCH_US-BLANK_END_US)
#define VISIBLE_PIXELS (VISIBLE_TIME/PIXEL_TIME)
#define EXTRA_PIXELS (VISIBLE_PIXELS-XPIXELS)

#ifndef STARTLINE
#define STARTLINE 40 // (uint16_t)(22+((TOTAL_LINES-VSYNCLINES-YPIXELS)/2))
#endif
#define ENDLINE STARTLINE + YPIXELS

#ifndef STARTPIX
#define STARTPIX ((uint16_t)((EXTRA_PIXELS+0.5)/3)+BLANKEND)
#endif
#define ENDPIX ((uint16_t)(STARTPIX+PLLCLKS_PER_PIXEL*XPIXELS/8))

// Calculate picture lengths in pixels and bytes, coordinate areas for picture area
#define PICLENGTH (ENDPIX - STARTPIX)
#define PICBITS (UBITS+VBITS+YBITS)
#define PICX ((uint16_t)(PICLENGTH * 8 / PLLCLKS_PER_PIXEL))
#define PICLINE_LENGTH_BYTES ((uint16_t)(PICX*PICBITS/8+0.5+1))

// Picture area memory start point
#define PICLINE_START ((INDEX_START_BYTES + TOTAL_LINES*3+1)+1)

// Picture area line start addresses
#define PICLINE_WORD_ADDRESS(n) (PICLINE_START/2+(PICLINE_LENGTH_BYTES/2+BEXTRA/2)*(n))
#define PICLINE_BYTE_ADDRESS(n) ((uint32_t)(PICLINE_START+((uint32_t)(PICLINE_LENGTH_BYTES)+BEXTRA)*(n)))
#define PICLINE_TOTAL_BYTES ((uint32_t)PICLINE_LENGTH_BYTES+BEXTRA)
#define VDCTRL2_LINECOUNT   ((TOTAL_LINES-1)<<0)
#define VDCTRL2_PROGRAM_LENGTH ((PLLCLKS_PER_PIXEL-1)<<10)

#define MICROCODE (uint32_t)((OP4 << 24) | (OP3 << 16) | (OP2 << 8) | (OP1))
