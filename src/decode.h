/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
int decodeRecord(DmOpenRef dbRef, UInt16 compressionType, 
                 Char* decodeTo, int recordIndex, int maxDecodeLen);
UInt16 decodedRecordLen(DmOpenRef dbRef, UInt16 compressionType, 
                        int recordIndex);
void drm_unlink_library(void);

