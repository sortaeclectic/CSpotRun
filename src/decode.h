/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "segments.h"
int decodeRecord(DmOpenRef dbRef, UInt16 compressionType, 
                 Char* decodeTo, int recordIndex, int maxDecodeLen) DECODE_SEGMENT;
UInt16 decodedRecordLen(DmOpenRef dbRef, UInt16 compressionType, 
                        int recordIndex) DECODE_SEGMENT;
void drm_unlink_library(void) DECODE_SEGMENT;

