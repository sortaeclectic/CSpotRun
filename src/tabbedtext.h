/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "segments.h"

void 
Justify_WinDrawChars (Char* chars, UInt16 len, 
                      Int16 x, Int16 y, 
                      UInt16 extend) TT_SEGMENT;
void 
Tab_WinDrawChars (Char* chars, 
                  UInt16 len, Int16 x, Int16 y) TT_SEGMENT;

#ifdef ENABLE_HYPHEN
UInt16 Hyphen_FntWordWrap (Char* chars, UInt16 extend) TT_SEGMENT;
void LockHyphenResource () TT_SEGMENT;
void UnlockHyphenResource () TT_SEGMENT;
#else
#define Hyphen_FntWordWrap FntWordWrap
#endif

