// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// bootldr\src\display\display.cpp
// 
//

#define FILE_ID	0x44495350
#define DEFINE_COLOURS

//
// Defining ENABLE_TRANSFER_STATS will display simplistic average and point
// transfer rates in bytes per second and provide an estimate of the length of
// time remaining.
//
#define ENABLE_TRANSFER_STATS

#include "bootldr.h"
#include <videodriver.h>
#include <hal.h>

TUint8* Screen;
TInt Screenwidth, Screenheight;		// screen dimentions in pixels
TInt XPos;
TInt YPos;
TUint Colours;
TUint PixelSize = 1;	// size of a pixel in bytes

TUint Progress[2];
TUint Pixels[2];
TUint Limit[2];
TUint32 ProgressTime[2];
TUint32 StartTime;

// Palette entries are in the platform's config.h
const TUint16 Palette[16]=	{
		KPaletteEntBlack		| KPaletteEntPBS,
		KPaletteEntMidBlue,		// 1
		KPaletteEntMidGreen,	// 2
		KPaletteEntMidCyan,		// 3
		KPaletteEntMidRed,		// 4
		KPaletteEntMidMagenta,	// 5
		KPaletteEntMidYellow,	// 6
		KPaletteEntDimWhite,	// 7
		KPaletteEntGray,		// 8
		KPaletteEntBlue,		// 9
		KPaletteEntGreen,		// 10 A
		KPaletteEntCyan,		// 11 B
		KPaletteEntRed,			// 12 C
		KPaletteEntMagenta,		// 13 D
		KPaletteEntYellow,		// 14 E
		KPaletteEntWhite 		// 15 F
		};

const TUint KRgbBlack		= 0x000000;
//const TUint KRgbDarkGray	= 0x555555;
const TUint KRgbDarkRed		= 0x800000;
const TUint KRgbDarkGreen	= 0x008000;
const TUint KRgbDarkYellow	= 0x808000;
const TUint KRgbDarkBlue	= 0x000080;
const TUint KRgbDarkMagenta	= 0x800080;
const TUint KRgbDarkCyan	= 0x008080;
const TUint KRgbRed			= 0xFF0000;
//const TUint KRgbGreen		= 0x00FF00;
const TUint KRgbYellow		= 0xFFFF00;
const TUint KRgbBlue		= 0x0000FF;
const TUint KRgbMagenta		= 0xFF00FF;
const TUint KRgbCyan		= 0x00FFFF;
const TUint KRgbGray		= 0xAAAAAA;
const TUint KRgbWhite		= 0xFFFFFF;

const TUint KRgbDimWhite    = 0xCCCCCC;

const TUint32 Palette32[16] =
{
    KRgbBlack,       // 0
    KRgbDarkBlue,    // 1
    KRgbDarkGreen,   // 2
    KRgbDarkCyan,    // 3
    KRgbDarkRed,     // 4
    KRgbDarkMagenta, // 5
    KRgbDarkYellow,  // 6
    KRgbDimWhite,    // 7 KRgbDarkGray()
    KRgbGray,        // 8
    KRgbBlue,        // 9
    KRgbDarkGreen,   // 10
    KRgbCyan,        // 11
    KRgbRed,         // 12
    KRgbMagenta,     // 13
    KRgbYellow,      // 14
    KRgbWhite        // 15
};

// KForeground, Kbackground and [I]Pcolour entries are indexes into the palette.
// The progress bar colours are of the form "(foreground << 8) | background"
const TUint8 KForeground = 15;
const TUint8 KBackground = 9;
const TUint PColour[2]={ 0xa08, 0xc08 };		//	(10|8) and (12|8)
const TUint IPColour[2]={ 0x809, 0x809 };		//	(8|9) and (8|9)

#define NUM_FONTS	96
#define FONT_HEIGHT	10
#define FONT_WIDTH	8

#define SCREEN_SIZE			(Screenheight*Screenwidth*PixelSize)			// number of pixels (size in bytes)

#define MAX_COLUMN			(Screenwidth/FONT_WIDTH)			// chars per line e.g. 80 or 40
#define MAX_ROW				(Screenheight/FONT_HEIGHT)			// lines per screen e.g. 48 or 24
#define PROGRESSBAR0_ROW	(MAX_ROW-1)							// row in which to draw progress bar 0
#define PROGRESSBAR1_ROW	(MAX_ROW-2)							// row in which to draw progress bar 1
#define STATS_ROW			(MAX_ROW-3)							// DEBUGGING row in which to write eta/bps
//
// ascii character code - 32 == character index into bootldr table
//
extern const TUint8 Font[NUM_FONTS][FONT_HEIGHT] =
 	{
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		{0x30,0x78,0x78,0x78,0x30,0x00,0x30,0x00,0x00,0x00},	//!
		{0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00,0x00,0x00},	//#
		{0x30,0x7C,0xC0,0x78,0x0C,0xF8,0x30,0x00,0x00,0x00},	//$
		{0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00,0x00,0x00},
		{0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00,0x00,0x00},
		{0x60,0x60,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		{0x18,0x30,0x60,0x60,0x60,0x30,0x18,0x00,0x00,0x00},
		{0x60,0x30,0x18,0x18,0x18,0x30,0x60,0x00,0x00,0x00},
		{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,0x00,0x00},
		{0x00,0x30,0x30,0xFC,0x30,0x30,0x00,0x00,0x00,0x00},
		{0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00},
		{0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0x00},	//-
		{0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00},	//.
		{0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00,0x00,0x00},	///

		{0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0x7C,0x00,0x00,0x00},	//0
		{0x30,0x70,0x30,0x30,0x30,0x30,0xFC,0x00,0x00,0x00},
		{0x78,0xCC,0x0C,0x38,0x60,0xCC,0xFC,0x00,0x00,0x00},
		{0x78,0xCC,0x0C,0x38,0x0C,0xCC,0x78,0x00,0x00,0x00},
		{0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00,0x00,0x00},
		{0xFC,0xC0,0xF8,0x0C,0x0C,0xCC,0x78,0x00,0x00,0x00},
		{0x38,0x60,0xC0,0xF8,0xCC,0xCC,0x78,0x00,0x00,0x00},
		{0xFC,0xCC,0x0C,0x18,0x30,0x30,0x30,0x00,0x00,0x00},
		{0x78,0xCC,0xCC,0x78,0xCC,0xCC,0x78,0x00,0x00,0x00},
		{0x78,0xCC,0xCC,0x7C,0x0C,0x18,0x70,0x00,0x00,0x00},	//9
		{0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x00,0x00,0x00},	//:
		{0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x60,0x00,0x00},	//;
		{0x18,0x30,0x60,0xC0,0x60,0x30,0x18,0x00,0x00,0x00},	//<
		{0x00,0x00,0xFC,0x00,0x00,0xFC,0x00,0x00,0x00,0x00},	//=
		{0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00,0x00,0x00},	//>
		{0x78,0xCC,0x0C,0x18,0x30,0x00,0x30,0x00,0x00,0x00},	//?

		{0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x78,0x00,0x00,0x00},	//@
		{0x30,0x78,0xCC,0xCC,0xFC,0xCC,0xCC,0x00,0x00,0x00},	//A
		{0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00,0x00,0x00},	//B
		{0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,0x00,0x00},	//C
		{0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00,0x00,0x00},	//D
		{0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00,0x00,0x00},	//E
		{0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00,0x00,0x00},	//F
		{0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00,0x00,0x00},	//G
		{0xCC,0xCC,0xCC,0xFC,0xCC,0xCC,0xCC,0x00,0x00,0x00},	//H
		{0x78,0x30,0x30,0x30,0x30,0x30,0x78,0x00,0x00,0x00},	//I
		{0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00,0x00,0x00},	//J
		{0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00,0x00,0x00},	//K
		{0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0x00,0x00},	//L
		{0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,0x00,0x00},	//M
		{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00,0x00,0x00},	//N
		{0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00,0x00,0x00},	//O

		{0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,0x00,0x00},	//P
		{0x78,0xCC,0xCC,0xCC,0xDC,0x78,0x1C,0x00,0x00,0x00},	//Q
		{0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00,0x00,0x00},	//R
		{0x78,0xCC,0xE0,0x70,0x1C,0xCC,0x78,0x00,0x00,0x00},	//S
		{0xFC,0x30,0x30,0x30,0x30,0x30,0x30,0x00,0x00,0x00},	//T
		{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00,0x00,0x00},	//U
		{0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00,0x00,0x00},	//V
		{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00,0x00,0x00},	//W
		{0xC6,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x00,0x00,0x00},	//X
		{0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00,0x00,0x00},	//Y
		{0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00,0x00,0x00},	//Z
		{0x78,0x60,0x60,0x60,0x60,0x60,0x78,0x00,0x00,0x00},
		{0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00,0x00,0x00},
		{0x78,0x18,0x18,0x18,0x18,0x18,0x78,0x00,0x00,0x00},
		{0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00,0x00},
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00},

		{0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		{0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00,0x00,0x00},
		{0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00,0x00,0x00},
		{0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00,0x00,0x00},
		{0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00,0x00,0x00},
		{0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00,0x00,0x00},
		{0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00,0x00,0x00},
		{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8,0x00,0x00},
		{0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00,0x00,0x00},
		{0x30,0x00,0x70,0x30,0x30,0x30,0x78,0x00,0x00,0x00},
		{0x0C,0x00,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00,0x00},
		{0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00,0x00,0x00},
		{0x70,0x30,0x30,0x30,0x30,0x30,0x78,0x00,0x00,0x00},
		{0x00,0x00,0xCC,0xFE,0xFE,0xD6,0xC6,0x00,0x00,0x00},
		{0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00,0x00,0x00},
		{0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00,0x00,0x00},

		{0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0,0x00,0x00},
		{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E,0x00,0x00},
		{0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00,0x00,0x00},
		{0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00,0x00,0x00},
		{0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00,0x00,0x00},
		{0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00},
		{0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00,0x00,0x00},
		{0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00,0x00,0x00},
		{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00,0x00,0x00},
		{0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8,0x00,0x00},
		{0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00,0x00,0x00},
		{0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00,0x00,0x00},
		{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0x00,0x00},
		{0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00,0x00,0x00},
		{0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
		{0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00},
	};

inline void ColourPixel(TInt aXPos, TInt aYPos, TUint aColour)
/**
colour a pixel on the screen
@param aXPos	pixel's X coordinate
@param aYPos	pixel's Y coordinate
@param aColour	chosen colour specified as a palette index
*/
{
	TUint8* pV = (TUint8*)Screen + (aYPos * (Screenwidth * PixelSize)) + (aXPos * PixelSize);

	switch(PixelSize)
		{
		case 4:
			*(TUint32*)pV = Palette32[aColour];
			break;
		case 2:
			*pV++ = Palette[aColour] & 0xFF ;
			*pV = Palette[aColour] >> 8 ;
			break;
		case 1:
			*pV = aColour;
			break;
		default:
			// Kern::Fault("BOOTLDR colourpixel: Unsupported bpp", PixelSize);
			BOOT_FAULT();
		}
}

#ifndef USE_ASM_DISPLAYCHAR
void DisplayChar(TInt aChar, TInt aXPos, TInt aYPos, TUint aColours)
/**
Write a character to the framebuffer
@param aChar	Character to draw
@param aXPos	Character's column
@param aYPos	Character's row
@param aColour	Palette index specified (foreground << 8| background)
*/
	{
	// Convert the ascii value into an index for the Font table
	TInt tmpChar = aChar - 32;

	// Validate this is a valid font index
	if (tmpChar < 0 || tmpChar >= NUM_FONTS)
		return;

	// Validate coordinates lie within range.
	if (aXPos > MAX_COLUMN-1 || aYPos > MAX_ROW-1)
		return;

	// for each line of the font
	for (TUint line=0 ; line < FONT_HEIGHT ; line++)
		{
		TUint8 tmpLine = Font[tmpChar][line];
		TUint8 tmpMask = 0x80;
		// for each pixel of the line
		for (TUint pixel=0 ; pixel < FONT_WIDTH ; pixel++)
			{
			ColourPixel(aXPos * FONT_WIDTH + pixel,
						aYPos * FONT_HEIGHT + line,
						(tmpLine & tmpMask) ? aColours >> 8 : aColours & 0xFF);

			// Shift the mask one bit downward to examine the next pixel
			tmpMask = tmpMask >> 1;
			}
		}

	return;
	}
#endif


GLDEF_C void ClearScreen()
/**
Set all the pixels on the screen to the background
*/
	{
	for (TInt y = 0 ; y<Screenheight ; y++)
		for (TInt x = 0 ; x<Screenwidth ; x++)
			ColourPixel(x, y, KBackground);

	XPos=0;
	YPos=0;
	Colours=(KForeground<<8)|KBackground;
	}

void ScrollScreen()
/**
Move all the pixels in the scrolling text area up one line and fill the
last line with the background colour
*/
	{
	TUint8* pV=Screen;
	wordmove(pV, pV+(Screenwidth*PixelSize*FONT_HEIGHT), (Screenwidth*PixelSize*(Screenheight-(2*FONT_HEIGHT))-(Screenwidth*PixelSize*FONT_HEIGHT)));
	pV+=(Screenwidth*PixelSize*(Screenheight-(2*FONT_HEIGHT))-(Screenwidth*PixelSize*FONT_HEIGHT));

	TInt StartLine = (Screenheight-(4*FONT_HEIGHT));
	for (TInt y = StartLine ; y<(StartLine+FONT_HEIGHT) ; y++)
		for (TInt x = 0 ; x<Screenwidth ; x++)
			ColourPixel(x, y, KBackground);
	}

void ScrollScreenDown()
/**
Move all the pixels in the scrolling text area down one line and fill the
first line with the background colour
*/
	{
	TUint8* pV=Screen;
	wordmove(pV+(Screenwidth*PixelSize*FONT_HEIGHT), pV, (Screenwidth*PixelSize*(Screenheight-(2*FONT_HEIGHT))-(Screenwidth*PixelSize*FONT_HEIGHT)));

	for (TInt y = 0 ; y<FONT_HEIGHT ; y++)
		for (TInt x = 0 ; x<Screenwidth ; x++)
			ColourPixel(x, y, KBackground);

	}

void CurDown()
/**
Move the cursor down the screen, scrolling the text area if necessary
*/
	{
	if (++YPos==(MAX_ROW-3))
		{
		YPos=(MAX_ROW-4);
		ScrollScreen();
		}
	}

void CurUp()
/**
Move the cursor up the screen, scrolling the text area if necessary
*/
	{
	if (--YPos<0)
		{
		YPos=0;
		ScrollScreenDown();
		}
	}

void CurRight()
/**
Move the cursor along the screen, scrolling the text area if necessary
*/
	{
	if (++XPos==MAX_COLUMN)
		{
		XPos=0;
		CurDown();
		}
	}

void CurLeft()
/**
Move the cursor backwards along the screen, scrolling the text area if necessary
*/
	{
	if (--XPos<0)
		{
		XPos=MAX_COLUMN-1;
		CurUp();
		}
	}

void SetPos(TInt X, TInt Y)
/**
Set the cursor to a point on the screen
@param X	Character's column
@param Y	Character's row
*/
	{
	XPos=X;
	YPos=Y;
	}

void PutChar(TUint aChar)
/**
Write a character to the screen then moves the cursor to the next position,
scrolling the screen if necessary
@param aChar	Character to write
*/
	{
	switch (aChar)
		{
		case 0x08: CurLeft(); return;		// BS '\b'
		case 0x09: CurRight(); return;		// HT '\t'
		case 0x0a: CurDown(); return;		// LF '\n'
		case 0x0b: CurUp(); return;			// VT '\v'
		case 0x0c: ClearScreen(); return;	// FF '\f'
		case 0x0d: XPos=0; return;			// CR '\r'
		default: break;
		}
	DisplayChar(aChar,XPos,YPos,Colours);
	if (++XPos==MAX_COLUMN)					// chars per line
		{
		XPos=0;
		if (++YPos==(MAX_ROW-3))	// lines per screen - 3 for progress bars & stats
			{
			YPos=(MAX_ROW-4);		// lines per screen - 4
			ScrollScreen();
			}
		}
	}

GLDEF_C void PrintToScreen(TRefByValue<const TDesC> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list,aFmt);

	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	PutString(aBuf);
	//RDebug::RawPrint(aBuf);
	}

void PutString(const TDesC& aBuf)
/**
For each character in the given string write it to the screen
@param aString	String of characters to write
*/
	{
	const TText* pS=aBuf.Ptr();
	const TText* pE=pS+aBuf.Length();
	if (pS!=pE)
		{
		while (pS<pE)
			PutChar(*pS++);
		}
	}

GLDEF_C void InitProgressBar(TInt aId, TUint aLimit, const TDesC& aBuf)
/**
Initialise a progress bar, writes a label and sets the pixels to the
background colour
@param aId		configure Bar0 or Bar1
@param aLimit	Sets the upper bound of the value given to Update
@param aTitle	Label to place before the progress bar (7 characters wide)
*/
	{
	const TText* aTitle=aBuf.Ptr();
	TInt line=aId ? PROGRESSBAR0_ROW : PROGRESSBAR1_ROW;
	Progress[aId]=0;
	if (aId==0)
			StartTime=0;

	Pixels[aId]=0;
	Limit[aId]=aLimit;
	char c;
	TInt x=0;
	while ((c=*aTitle++)!=0 && x<7)
		{
		DisplayChar(c,x,line,Colours);
		++x;
		}
	if (x<7)
		x=7;
	for (; x<(MAX_COLUMN-1); ++x)
		DisplayChar(0x7f,x,line,IPColour[aId]);
	}

GLDEF_C void UpdateProgressBar(TInt aId, TUint aProgress)
/**
Update a progress bar filling in the bar to a new position
If ENABLE_TRANSFER_STATS has been defined at build time the function will
generate some simple statistics.
@param aId			update Bar0 or Bar1
@param aProgress	The point value between 0 and the given aLimit
*/
	{
	TInt line=aId ? PROGRESSBAR0_ROW : PROGRESSBAR1_ROW;
	TUint old_pixels=Pixels[aId];

#ifdef ENABLE_TRANSFER_STATS
	// turn this on and get a few statistics as it loads
	if (aId==0) {
			if (StartTime == 0) {
					StartTime = User::NTickCount();
					ProgressTime[aId] = StartTime;
			}
			TUint avg_bps=0, bps=0, eta=0;
			TUint32 timenow = User::NTickCount();
			TUint32 delta_time = timenow - ProgressTime[aId];
			ProgressTime[aId] = timenow;
			if (delta_time) {
					bps = ((aProgress - Progress[aId]) * 1000) / delta_time;		// delta bytes / delta milliseconds

					delta_time = (timenow - StartTime);					// milliseconds since started
					if (delta_time) {
							avg_bps = ((TUint64)aProgress  * 1000) / delta_time;
							eta = (Limit[aId] - aProgress) / avg_bps;
					}
			}

			TInt savedXPos=XPos, savedYPos=YPos;
			XPos=0;
			YPos=STATS_ROW;
//			Printf("C/s avg: %7d point: %7d ETA: %d    ", avg_bps, bps, eta);
			PrintToScreen(_L("point: %7d ETA: %d    "), bps, eta);
			XPos=savedXPos;
			YPos=savedYPos;
	}
#endif

	Progress[aId]=aProgress;
	Int64 prog64=aProgress;
	prog64*=(Screenwidth - (8*FONT_WIDTH));		// screenwidth in pixels - 8 characters
	prog64/=Limit[aId];
	TUint pixels=(TUint)prog64;
	if (pixels>old_pixels)
		{
		Pixels[aId]=pixels;

		while (old_pixels<pixels)
			{
			for (TInt i=0; i<6; ++i)
				ColourPixel( old_pixels + (7*FONT_WIDTH), (line * FONT_HEIGHT) + 2 + i  , (PColour[aId]>>8));

			++old_pixels;
			}
		}
	}

void InitDisplay()
/**
Reads the properties of the screen from the HAL and configures the settings so
this driver can colour in pixels.
*/
    {
	TInt iDisplayMode;
	TInt iScreenAddress;
	TInt iBitsPerPixel;
	TInt iIsPalettized;
	TInt offset;

	HAL::Get(HAL::EDisplayMode, iDisplayMode);
	iBitsPerPixel = iDisplayMode; //getbpp needs the current mode as its param
	HAL::Get(HAL::EDisplayBitsPerPixel, iBitsPerPixel);	

	iIsPalettized = iDisplayMode; //ispalettized needs the current mode as its param
	HAL::Get(HAL::EDisplayIsPalettized, iIsPalettized);	

	HAL::Get(HAL::EDisplayMemoryAddress, iScreenAddress);
	
	offset = iDisplayMode;
	HAL::Get(HAL::EDisplayOffsetToFirstPixel, offset);
	iScreenAddress += offset;

	// get the dimentions of the screen
	HAL::Get(HAL::EDisplayXPixels, Screenwidth);
	HAL::Get(HAL::EDisplayYPixels, Screenheight);
	
	Screen = (TUint8 *)iScreenAddress;

#if 0
	RDebug::Printf("EDisplayMode %d", iDisplayMode);
	RDebug::Printf("EDisplayBitsPerPixel %d", iBitsPerPixel);
	RDebug::Printf("EDisplayIsPalettized %d", iIsPalettized);
	RDebug::Printf("EDisplayMemoryAddress 0x%x (after offset)", iScreenAddress);
	RDebug::Printf("EDisplayOffsetToFirstPixel  %d",offset);
	RDebug::Printf("EDisplayXPixels  %d",Screenwidth);
	RDebug::Printf("EDisplayYPixels  %d", Screenheight);
#endif
	
	// get the bits per pixel to work out the size
	if (iBitsPerPixel == 32)
		{
		PixelSize = 4;
		}
	else if (iBitsPerPixel == 16)
		{
		PixelSize = 2;
		}
	else if (iBitsPerPixel == 8)
		{
#if 0
		// derive where the palette is located - assumes when no offset to first
		// pixel the palette is at end of framebuffer.
		TUint8* pV;
		if(vidBuf().iOffsetToFirstPixel)
			pV=(TUint8*)(vidBuf().iVideoAddress);
		else
			pV=(TUint8*)(vidBuf().iVideoAddress)+SCREEN_SIZE;

		// Kern::Printf("Screenbuffer 0x%x palette 0x%x (%dx%d)",Screen,pV,Screenwidth,Screenheight);
		PixelSize = 1;
		memcpy(pV,Palette,32);
#endif
		}
	else
		{
		RDebug::Printf("BOOTLDR display: Unsupported bpp %d",iBitsPerPixel);
		BOOT_FAULT();
		}

	ClearScreen();
	}
