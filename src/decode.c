/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2000  by Bill Clagett (wtc@pobox.com)
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

// The decompression is implemented from the description at
// http://pc-28134.on.rogers.wave.ca/Programming/PilotDoc.htm

#include <Common.h>
#include <System/SysAll.h>
#include "decode.h"

static int decodeFromBuffer(UCharPtr decodeTo, UCharPtr decodeFrom, int decodeFromLen, int maxDecodeLen);
static int decodeLen(UCharPtr decodeFrom, int recordIndex);
static const char unknownCompressionMessage[] = "Unknown record compression type! ";

int decodedRecordLen(DmOpenRef dbRef, Word compressionType, int recordIndex)
{
    VoidHand    recordHandle = NULL;
    int            recordLen = 0;
    CharPtr        recordPtr = NULL;
    int            decodedLen = 0;

    if (dbRef == NULL)
        return 0;

    recordHandle = DmQueryRecord(dbRef, recordIndex);
    recordLen = MemHandleSize(recordHandle);
    recordPtr = MemHandleLock(recordHandle);

    switch (compressionType)
    {
        case 2:
            decodedLen = decodeLen(recordPtr, recordLen);
            break;
        case 1:
            decodedLen = recordLen;
            break;
        default:
            decodedLen = StrLen(unknownCompressionMessage);
    }

    MemHandleUnlock(recordHandle);

    return decodedLen;}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int decodeRecord(DmOpenRef dbRef, Word compressionType, CharPtr decodeTo, int recordIndex, int maxDecodeLen)
{
    VoidHand    recordHandle = NULL;
    int            recordLen = 0;
    CharPtr        recordPtr = NULL;
    int            decodedLen = 0;

    if (dbRef == NULL)
    {
        decodeTo[0]='\0';
        return StrLen(decodeTo);
    }

    recordHandle = DmQueryRecord(dbRef, recordIndex);
    recordLen = MemHandleSize(recordHandle);
    recordPtr = MemHandleLock(recordHandle);

    switch (compressionType)
    {
        case 2:
            decodedLen = decodeFromBuffer(decodeTo, recordPtr, recordLen, maxDecodeLen);
            break;
        case 1:
            decodedLen = min(maxDecodeLen, recordLen);
            MemMove(decodeTo, recordPtr, decodedLen);
            break;
        default:
            decodedLen = StrLen(unknownCompressionMessage);
            MemMove(decodeTo, unknownCompressionMessage, decodedLen + 1);
    }

    MemHandleUnlock(recordHandle);

    return decodedLen;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
static int decodeFromBuffer(UCharPtr decodeTo, UCharPtr decodeFrom, int decodeFromLen, int maxDecodeLen)
{
    UCharPtr fromTooFar = &decodeFrom[decodeFromLen];
    UCharPtr toTooFar = &decodeTo[maxDecodeLen];
    UCharPtr decodeStart = decodeTo;
    unsigned int c;

    // These are used only in B commands
    int     windowLen;
    int     windowDist;
    UCharPtr windowCopyFrom;

    while(decodeFrom < fromTooFar && decodeTo < toTooFar)
    {
        c = *(decodeFrom++);

        // type C command (space + char)
        if (c >= 0xC0)
        {
            *(decodeTo++)=' ';
            if (decodeTo < toTooFar)
                *(decodeTo++)=c & 0x7F;
        }
        // type B command (sliding window sequence)
        else if (c >= 0x80)
        {
            c = (c<<8) | *(decodeFrom++);        // Move this to high bits and read low bits
            windowLen = 3 + (c & 0x7);        // 3 + low 3 bits (Beirne's 'n'+3)
            windowDist = (c >> 3) & 0x07FF;    // next 11 bits (Beirne's 'm')
            windowCopyFrom = decodeTo - windowDist;

            windowLen = min(windowLen, toTooFar - decodeTo);
            while(windowLen--)
                *(decodeTo++) = *(windowCopyFrom++);
        }
        //self-representing, no command.
        else if (c >= 0x09)
        {
            *(decodeTo++)=c;
        }
        //type A command (next c chars are literal)
        else if (c >= 0x01)
        {
            c = min(c, toTooFar - decodeTo);
            while(c--)
                *(decodeTo++) = *(decodeFrom++);
        }
        //c == 0, also self-representing
        else
        {
            *(decodeTo++)=c;
        }
    }
    return decodeTo - decodeStart;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
static int decodeLen(UCharPtr decodeFrom, int decodeFromLen)
{
    UCharPtr fromTooFar = &decodeFrom[decodeFromLen];
    unsigned int c;
    int len = 0;

    while(decodeFrom < fromTooFar)
    {
        c = *(decodeFrom++);

        // type C command (space + char)
        if (c >= 0xC0)
        {
            len+=2;
        }
        // type B command (sliding window sequence)
        else if (c >= 0x80)
        {
            c = (c<<8) | *(decodeFrom++);        // Move this to high bits and read low bits
            len += 3 + (c & 0x7);        // 3 + low 3 bits (Beirne's 'n'+3)
        }
        //self-representing, no command.
        else if (c >= 0x09)
        {
            len++;
        }
        //type A command (next c chars are literal)
        else if (c >= 0x01)
        {
            len+=c;
            decodeFrom+=c;
        }
        //c == 0, also self-representing
        else
        {
            len++;
        }
    }
    return len;
}
