/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998,1999  by Bill Clagett (wtc@pobox.com)
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

#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>

#include "rotate.h"
#include <UI/ScrDriverNew.h>

#define BYTE_WITH_BITS(pos, len, v)    ((v)<<(8-(len)-(pos)))
#define PIXEL_BYTE_INDEX(x, y, rbts, bpp)    ((y)*(rbts)+((x)*(bpp))/8)
#define X_ROTATE(x, y, a) ((x)*cosTable[(a)] - (y)*sinTable[(a)])
#define Y_ROTATE(x, y, a) ((y)*cosTable[(a)] + (x)*sinTable[(a)])
#define PIXEL_BIT_INDEX(x, bpp) (((x)*(bpp))%8)

#ifdef ENABLE_ROTATION

static int sinTable[] = {0,  1,  0, -1};
static int cosTable[] = {1,  0, -1,  0};

int RotateY(int x, int y, OrientationType a)
{
    return Y_ROTATE(x,y,a);
}

static int getBpp()
{
    DWord depth;
    ScrDisplayMode(scrDisplayModeGet, NULL, NULL, &depth, NULL);
    return (int) depth;
}

// This is all crap, of course.
//
// It will surely break on the next os or device. If the API gave us
// a supported way to write pixels, I'd love to use it.
//
// This is going to hurt a little. Try to relax.

void RotCopyWindow(WinHandle fromWindowH, int ox, int oy, OrientationType a)
{
    int toX, toY;               //writing to
    unsigned int fromX, fromY;  //copying from
    int xOffset, yOffset;       //Base delta to the lower right hand corner pixel's coordinates
    WinHandle toWindowH = WinGetDrawWindow();

    Byte* to =        toWindowH->displayAddrV20;
    Byte* from =    fromWindowH->displayAddrV20;

    BytePtr fromPtr;
    int dToXdFromY, dToYdFromY; //Change in to coords per change in from y coords

    Byte v;
    BytePtr toByte;

    int fromBpp;
    int toBpp;
    unsigned int fromWidth, fromHeight;
    unsigned int toWidth, toHeight;
    unsigned int fromRowBytes;
    unsigned int toRowBytes;

    int            dToXdx, dToYdx;
    int            dToByte;
    Byte        toBitMask;
    Byte        fromBitMask;
    Byte        startingFromBitMask;
    int byteCol = -1;
    Byte        toBlackBits;

    if (fromWindowH->gDeviceP)
    {
        //OS2
        fromRowBytes = fromWindowH->gDeviceP->rowBytes;
        toRowBytes = toWindowH->gDeviceP->rowBytes;
        fromBpp = fromWindowH->gDeviceP->pixelSize;
        toBpp = toWindowH->gDeviceP->pixelSize;
        fromWidth = fromWindowH->displayWidthV20;
        fromHeight = fromWindowH->displayHeightV20;
        toWidth = toWindowH->displayWidthV20;
        toHeight = toWindowH->displayHeightV20;
    }
    else
    {
        //OS3
        SWord x,y;

        WinSetDrawWindow(fromWindowH);
        WinGetWindowExtent(&x,&y);
        fromWidth = x;
        fromHeight = y;

        WinSetDrawWindow(toWindowH);
        WinGetWindowExtent(&x,&y);
        toWidth = x;
        toHeight = y;

        fromBpp = 1;
        fromRowBytes = 20;//(fromWidth*fromBpp+7)/8;
        toBpp = getBpp();
        toRowBytes = 20;//(toWidth*toBpp+7)/8;
    }

    toBlackBits = ~(0xFF << toBpp);

    xOffset = X_ROTATE(fromWidth-1, fromHeight-1, a);
    xOffset = (xOffset < 0 ? -xOffset : 0);
    yOffset = Y_ROTATE(fromWidth-1, fromHeight-1, a);
    yOffset = (yOffset < 0 ? -yOffset : 0);

    dToXdx = X_ROTATE(1,0,a);
    dToYdx = Y_ROTATE(1,0,a);
    if (dToYdx > 0)
        dToByte = toRowBytes;
    else if (dToYdx < 0)
        dToByte = -toRowBytes;
    else
        dToByte = 0;

    startingFromBitMask = ~(((Byte)0xFF)>>fromBpp);
    for (fromY=0; fromY < fromHeight; fromY++)
    {
        fromX = 0;
        toX    = X_ROTATE(0, fromY, a)+xOffset+ox;
        toY    = Y_ROTATE(0, fromY, a)+yOffset+oy;

        //If the not sideways, and only this pixel were on, toByte would be equal to
        //toBitMask. Note that upon moving to the pixels to the immediate left or right,
        //toBitMask just needs to be shifted by toBpp.
        toBitMask = BYTE_WITH_BITS(PIXEL_BIT_INDEX(toX, toBpp), toBpp, toBlackBits);

        //If only this pixel were on, *fromPtr would be equal to fromBitMask. Same shifting
        //silliness.
        fromBitMask = startingFromBitMask;

        fromPtr = &from[PIXEL_BYTE_INDEX(0, fromY, fromRowBytes, fromBpp)];

        toByte = to + PIXEL_BYTE_INDEX(toX, toY, toRowBytes, toBpp);

        //If sideways and on new byte column, wipe out whole column of bytes.
        if (dToYdx)
        {
            int newByteCol = (toByte-to)%toRowBytes;
            if (byteCol != newByteCol)
            {
                byteCol = newByteCol;
                for (; fromX < fromWidth; fromX++)
                {
                    *toByte = 0;
                    toByte += dToByte;
                }
                fromX = 0;
                toByte = to + PIXEL_BYTE_INDEX(toX, toY, toRowBytes, toBpp);
            }
        }

        for (; fromX < fromWidth; fromX++)
        {
            //If any bit in the from-pixel is on, make a black to-pixel.
            if (*fromPtr & fromBitMask)
                *toByte |= toBitMask;
            //else
                //*toByte &= ~toBitMask;

            if (dToYdx)
            {
                //We're sideways.
                toByte += dToByte;
            }
            else if (dToXdx<0)
            {
                //Upside-down
                toBitMask <<= toBpp;
                if (!toBitMask)
                {
                    //We left this to-byte, move on to the next
                    toBitMask = BYTE_WITH_BITS(8-toBpp, toBpp, toBlackBits);
                    toByte--;
                    *toByte=0;
                }
            }
            else if (dToXdx>0)
            {
                //Normal orientation
                toBitMask >>= toBpp;
                if (!toBitMask)
                {
                    //We left this to-byte, move on to the next
                    toBitMask = BYTE_WITH_BITS(0, toBpp, toBlackBits);
                    toByte++;
                    *toByte=0;
                }
            }

            if (!(fromBitMask >>= fromBpp))
            {
                //We left this fromByte, move to the next
                fromBitMask = startingFromBitMask;
                fromPtr++;
            }
        }
    }
}


#endif
