/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <PalmOS.h>
#include "rotate.h"
#include "app.h"

typedef UInt16 Pyte;
#define BITS_PER_PYTE (8 * sizeof(Pyte))

#define PYTE_WITH_BITS(pos, len, v)    ((v)<<(BITS_PER_PYTE-(len)-(pos)))
#define PIXEL_PYTE_INDEX(x, y, rbts, bpp)    ((y)*(rbts)+((x)*(bpp))/BITS_PER_PYTE)
#define X_ROTATE(x, y, a) ((x)*cosTable[(a)] - (y)*sinTable[(a)])
#define Y_ROTATE(x, y, a) ((y)*cosTable[(a)] + (x)*sinTable[(a)])
#define PIXEL_BIT_INDEX(x, bpp) (((x)*(bpp))%BITS_PER_PYTE)

#ifdef ENABLE_ROTATION

static int sinTable[] = {0,  1,  0, -1};
static int cosTable[] = {1,  0, -1,  0};

int RotateY(int x, int y, OrientationType a)
{
    return Y_ROTATE(x,y,a);
}

// This is all crap, of course.
//
// It will surely break on the next os or device. If the API gave us
// a supported way to write pixels, I'd love to use it.
//
// This is going to hurt a little. Try to relax.

Pyte* windowBits(WinHandle win)
{
    if (UtilOSIsAtLeast(3, 5)) {
        BitmapType *bmp = WinGetBitmap(win);
        return BmpGetBits(bmp);
    } else {
        return win->displayAddrV20;
    }
}

void RotCopyWindow(WinHandle fromWindowH, int startRow, int stopRow, OrientationType a)
{
    register Pyte  fromPyte;
    Coord toX, toY;               //writing to
    Coord fromX, fromY;  //copying from
    int xOffset, yOffset;       //Base delta to the lower right hand corner pixel's coordinates
    WinHandle toWindowH = WinGetDrawWindow();

    Pyte* to =      NULL;
    Pyte* from =    NULL;
    Pyte* fromPtr;
    int   dToXdFromY, dToYdFromY; //Change in to coords per change in from y coords

    Pyte v;
    Pyte* toPyte;

    const int fromBpp = 1;
    const int toBpp = 1;
    Coord fromWidth, fromHeight;
    Coord toWidth, toHeight;
    unsigned int fromRowPytes;
    unsigned int toRowPytes;

    int         dToXdx, dToYdx;
    int         dToPyte;
    Pyte       toBitMask;
    Pyte       fromBitMask;
    Pyte       startingFromBitMask;
    int          pyteCol = -1;
    Pyte       toBlackBits;

    to = windowBits(toWindowH);
    from = windowBits(fromWindowH);

    if (UtilOSIsAtLeast(3, 5)) {
        BitmapType *bmp;

        bmp = WinGetBitmap(fromWindowH);
        fromRowPytes = (bmp->rowBytes * 8) / BITS_PER_PYTE;
        fromWidth = bmp->width;
        fromHeight = bmp->height;

        bmp = WinGetBitmap(toWindowH);
        toRowPytes = (bmp->rowBytes * 8) / BITS_PER_PYTE;
        toWidth = bmp->width;
        toHeight = bmp->height;
    }
    else if (fromWindowH->bitmapP)
    {
        //OS3

        //MyGDeviceType is copied from the GDeviceType
        //structure in WindowNew.h in the 3.1 SDK.
        struct MyGDeviceType {
            void*   baseAddr;        // base address
            UInt16  width;           // width in pixels
            UInt16  height;          // height in pixels
            UInt16  rowBytes;        // rowBytes of display
            UInt8   pixelSize;       // bits/pixel
            // There's more crap that we don't care about....
        };
        struct MyGDeviceType *fromDeviceP, *toDeviceP;

        fromDeviceP = (struct MyGDeviceType*) fromWindowH->bitmapP;
        fromRowPytes = (fromDeviceP->rowBytes * 8) / BITS_PER_PYTE;
        fromWidth = fromWindowH->displayWidthV20;
        fromHeight = fromWindowH->displayHeightV20;

        toDeviceP  = (struct MyGDeviceType*) toWindowH->bitmapP;
        toRowPytes = (toDeviceP->rowBytes * 8) / BITS_PER_PYTE;
        toWidth = toWindowH->displayWidthV20;
        toHeight = toWindowH->displayHeightV20;
    }
    else
    {
        //OS2
        Int16 x,y;

        WinSetDrawWindow(fromWindowH);
        WinGetWindowExtent(&x,&y);
        fromWidth = x;
        fromHeight = y;

        WinSetDrawWindow(toWindowH);
        WinGetWindowExtent(&x,&y);
        toWidth = x;
        toHeight = y;

        //Assuming rows are multiples of words.
        fromRowPytes =  (fromWidth + 15) / BITS_PER_PYTE;
        toRowPytes = (toWidth + 15) / BITS_PER_PYTE;
    }

    fromHeight = stopRow + 1;

    toBlackBits = ~((~(Pyte)0x0) << toBpp);

    xOffset = X_ROTATE(fromWidth-1, fromHeight-1, a);
    xOffset = (xOffset < 0 ? -xOffset : 0);
    yOffset = Y_ROTATE(fromWidth-1, fromHeight-1, a);
    yOffset = (yOffset < 0 ? -yOffset : 0);

    dToXdx = X_ROTATE(1,0,a);
    dToYdx = Y_ROTATE(1,0,a);
    if (dToYdx > 0)
        dToPyte = toRowPytes;
    else if (dToYdx < 0)
        dToPyte = -toRowPytes;
    else
        dToPyte = 0;

    if (a == angle90 || a == angle270) {
        while ((startRow * toBpp) % BITS_PER_PYTE)
            startRow--;
        while ((stopRow * toBpp) % BITS_PER_PYTE)
            stopRow++;
        stopRow--;
    }

    startingFromBitMask = ~((~(Pyte)0x0)>>fromBpp);
    for (fromY=startRow; fromY < fromHeight; fromY++)
    {
        fromX = 0;
        toX    = X_ROTATE(0, fromY, a)+xOffset;
        toY    = Y_ROTATE(0, fromY, a)+yOffset;

        //If the not sideways, and only this pixel were on, toUInt8 would be equal to
        //toBitMask. Note that upon moving to the pixels to the immediate left or right,
        //toBitMask just needs to be shifted by toBpp.
        toBitMask = PYTE_WITH_BITS(PIXEL_BIT_INDEX(toX, toBpp), toBpp, toBlackBits);

        //If only this pixel were on, *fromPtr would be equal to fromBitMask. Same shifting
        //silliness.
        fromBitMask = startingFromBitMask;

        fromPtr = &from[PIXEL_PYTE_INDEX(0, fromY, fromRowPytes, fromBpp)];

        toPyte = to + PIXEL_PYTE_INDEX(toX, toY, toRowPytes, toBpp);

        //If sideways and on new byte column, wipe out whole column of bytes.
        if (dToYdx)
        {
            int newPyteCol = (toPyte-to)%toRowPytes;
            if (pyteCol != newPyteCol)
            {
                pyteCol = newPyteCol;
                for (; fromX < fromWidth; fromX++)
                {
                    *toPyte = 0;
                    toPyte += dToPyte;
                }
                fromX = 0;
                toPyte = to + PIXEL_PYTE_INDEX(toX, toY, toRowPytes, toBpp);
            }
        }

        fromPyte = *fromPtr;
        for (; fromX < fromWidth; fromX++)
        {
            //If any bit in the from-pixel is on, make a black to-pixel.
            if (fromPyte & fromBitMask)
                *toPyte |= toBitMask;
            //else
                //*toUInt8 &= ~toBitMask;

            if (dToYdx)
            {
                //We're sideways.
                toPyte += dToPyte;
            }
            else if (dToXdx<0)
            {
                //Upside-down
                toBitMask <<= toBpp;
                if (!toBitMask)
                {
                    //We left this to-byte, move on to the next
                    toBitMask = PYTE_WITH_BITS(BITS_PER_PYTE-toBpp, toBpp, toBlackBits);
                    toPyte--;
                    if (toPyte >= to)
                        *toPyte=0;
                }
            }
            else if (dToXdx>0)
            {
                //Normal orientation
                toBitMask >>= toBpp;
                if (!toBitMask)
                {
                    //We left this to-byte, move on to the next
                    toBitMask = PYTE_WITH_BITS(0, toBpp, toBlackBits);
                    toPyte++;
                    if (fromX + 1 < fromWidth)
                        *toPyte=0;
                }
            }

            if (!(fromBitMask >>= fromBpp))
            {
                //We left this fromUInt8, move to the next
                fromBitMask = startingFromBitMask;
                fromPtr++;
                fromPyte = *fromPtr;
            }
        }
    }
}

void RotScrollRectangleUp(RectangleType *rect, OrientationType o)
{
    WinDirectionType dir = winUp; //Initialized only to make the compiler happy.
    RectangleType vacated;

    switch (o)
    {
        case angle0:
            dir = winUp;
            break;
        case angle90:
            dir = winRight;
            break;
        case angle180:
            dir = winDown;
            break;
        case angle270:
            dir = winLeft;
            break;
    }

    WinScrollRectangle(rect, dir, 1, &vacated);

}
#endif
