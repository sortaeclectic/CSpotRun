/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

/* The decompression is implemented from the description at
 * http://pc-28134.on.rogers.wave.ca/Programming/PilotDoc.htm
 *
 * 6/16/2001: That page is long gone. Here are some that work today:
 * http://cr945328-a.flfrd1.on.wave.home.com/Programming/PilotDoc.htm
 * http://www.math.ohio-state.edu/~nevai/palm/format.html
 * http://www.nicholson.com/rhn/pilot/doc.txt
 */

#include <PalmOS.h>
#include "decode.h"
#include "csrdrmlib.h"

#define min(a,b) ((a)<(b)?(a):(b))

static int decodeFromBuffer(UInt8* decodeTo, UInt8* decodeFrom, int decodeFromLen, int maxDecodeLen);
static int decodeLen(UInt8* decodeFrom, int recordIndex);
static int drm_decodeFromBuffer(UInt16 moduleNum, UInt8* decodeTo, UInt8* decodeFrom, int decodeFromLen, int maxDecodeLen);
static void drm_link_library(UInt16 moduleNum);
static const char unknownCompressionMessage[] 
                  = "Unknown record compression type! ";
static char unknownCompressionMessageDRM[] 
            = "Unknown record compression type! You need a DRM module. "\
              "See http://csrdrm.sourceforge.net/ for information on "\
              "CSRDRM00.PRC\n\n";
static const char hexcnvt[] = "0123456789ABCDEF";
#define NO_LIBRARY 0xffff
static UInt16 ModuleNum = NO_LIBRARY;
static UInt16 refNum;

UInt16 
decodedRecordLen(DmOpenRef dbRef, UInt16 compressionType, int recordIndex)
{
    MemHandle    recordHandle = NULL;
    int            recordLen = 0;
    Char*        recordPtr = NULL;
    int            decodedLen = 0;
    
    if (dbRef == NULL)
        return 0;
    
    recordHandle = DmQueryRecord(dbRef, recordIndex);
    recordLen = MemHandleSize(recordHandle);
    recordPtr = MemHandleLock(recordHandle);

    if ((compressionType >= 0x100) && (compressionType < 0x200)) {
        zrecord *zrec = (zrecord *)(recordPtr);
        decodedLen = zrec->sizel | (zrec->sizeh << 8);
    } else  
        switch (compressionType)
        {
        case 1026:
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

    return decodedLen;
}

int decodeRecord(DmOpenRef dbRef, UInt16 compressionType, Char* decodeTo, int recordIndex, int maxDecodeLen)
{
    MemHandle    recordHandle = NULL;
    int            recordLen = 0;
    Char*        recordPtr = NULL;
    int            decodedLen = 0;
    
    if (dbRef == NULL)
    {
        decodeTo[0]='\0';
        return StrLen(decodeTo);
    }
    
    recordHandle = DmQueryRecord(dbRef, recordIndex);
    recordLen = MemHandleSize(recordHandle);
    recordPtr = MemHandleLock(recordHandle);
    
    if ((compressionType >= 0x100) && (compressionType < 0x200)) {
        decodedLen = drm_decodeFromBuffer(compressionType - 0x100, 
                                          decodeTo, recordPtr, 
                                          recordLen, maxDecodeLen);
    } else  
      switch (compressionType)
      {
        case 1026:
        case 2:
            decodedLen = decodeFromBuffer(decodeTo, recordPtr, 
                                          recordLen, maxDecodeLen);
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

static int 
decodeFromBuffer(UInt8* decodeTo, UInt8* decodeFrom, 
                 int decodeFromLen, int maxDecodeLen)
{
    UInt8* fromTooFar = &decodeFrom[decodeFromLen];
    UInt8* toTooFar = &decodeTo[maxDecodeLen];
    UInt8* decodeStart = decodeTo;
    unsigned int c;

    // These are used only in B commands
    int     windowLen;
    int     windowDist;
    UInt8* windowCopyFrom;

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
            // Move this to high bits and read low bits
            c = (c<<8) | *(decodeFrom++);        
            // 3 + low 3 bits (Beirne's 'n'+3)
            windowLen = 3 + (c & 0x7);        
            // next 11 bits (Beirne's 'm')
            windowDist = (c >> 3) & 0x07FF;    
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
static int decodeLen(UInt8* decodeFrom, int decodeFromLen)
{
    UInt8* fromTooFar = &decodeFrom[decodeFromLen];
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
            // Move this to high bits and read low bits
            c = (c<<8) | *(decodeFrom++);        
            // 3 + low 3 bits (Beirne's 'n'+3)
            len += 3 + (c & 0x7);        
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

void fill_with_unknownCompressionMessageDRM(UInt8* decodeTo, int size)
{
    int len = StrLen(unknownCompressionMessageDRM);
    
    while (size > 0) {
        if (len > size)
            len = size;
        MemMove(decodeTo, unknownCompressionMessageDRM, len);
        decodeTo += len;
        size -= len;
    }
    return;
}

static int 
drm_decodeFromBuffer(UInt16 moduleNum, UInt8* decodeTo, UInt8* decodeFrom, 
                     int decodeFromLen, int maxDecodeLen)
{
    Err err;
    zrecord *zrec = (zrecord *)decodeFrom;
    int size;
    UInt8 *buffer;

    if (ModuleNum != moduleNum) {
        drm_unlink_library();
        drm_link_library(moduleNum);
    }
    if (ModuleNum == NO_LIBRARY) {
        fill_with_unknownCompressionMessageDRM(decodeTo, maxDecodeLen);
        return (maxDecodeLen);
    }
    
    size = zrec->sizel | (zrec->sizeh << 8);
    if (size > maxDecodeLen) {
        buffer = MemPtrNew(size);
        ErrFatalDisplayIf(!buffer, "Could not allocate decord buffer "\
                          "in CSpotRun!");
        err = csr_drm_lib_decrypt(refNum, buffer, decodeFrom, decodeFromLen);
        if (err) {
            fill_with_unknownCompressionMessageDRM(buffer, size);
        }
        MemMove(decodeTo, buffer, maxDecodeLen);
        MemPtrFree(buffer);
        return(maxDecodeLen);
    } else {
        err = csr_drm_lib_decrypt(refNum, decodeTo, decodeFrom, decodeFromLen);
        if (err) {
            fill_with_unknownCompressionMessageDRM(decodeTo, size);
        }
        return(size);
    }
}

void drm_unlink_library(void)
{
    if (ModuleNum == NO_LIBRARY)
        return;
    
    if (csr_drm_lib_close(refNum))
        SysLibRemove(refNum);
}

static void drm_link_library(UInt16 moduleNum)
{
    char libname[] = "CSRDRM00";
    Err err;
    UInt32 creator_id = 'CSDR';
    UInt8 *cid = (UInt8 *)&creator_id;
    UInt16 version = CSR_DRM_LIB_INTERFACE_VERSION;
    
    libname[7] = hexcnvt[(moduleNum & 0xf)];
    libname[6] = hexcnvt[((moduleNum & 0xf0) >> 4)];
    unknownCompressionMessageDRM[StrLen(unknownCompressionMessageDRM)-7] 
        = hexcnvt[(moduleNum & 0xf)];
    unknownCompressionMessageDRM[StrLen(unknownCompressionMessageDRM)-8] 
        = hexcnvt[((moduleNum & 0xf0) >> 4)];
    cid[2] = 0xf3;
    cid[3] = moduleNum & 0xff;
    
    err = SysLibFind(libname, &refNum);
    if (err)
        err = SysLibLoad('libr', creator_id, &refNum);
    if (!err)
        err = csr_drm_lib_open(refNum);
    if (!err)
        err = csr_drm_lib_get_int_version(refNum, &version);
    /* if major version numbers don't match we can't use this library */
    if ((version & 0xff00) != (CSR_DRM_LIB_INTERFACE_VERSION & 0xff00))
        err = 2;
    if (!err)
        err = csr_drm_lib_get_decrypt(refNum, (UInt32 *)&decrypt, &_ctext);
    if (!err)
        ModuleNum = moduleNum;
    else
        ModuleNum = NO_LIBRARY;
}


