/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
void Justify_WinDrawChars (Char* chars, UInt16 len, Int16 x, Int16 y, UInt16 extend);
void Tab_WinDrawChars (Char* chars, UInt16 len, Int16 x, Int16 y);

#ifdef ENABLE_HYPHEN
UInt16 Hyphen_FntWordWrap (Char* chars, UInt16 extend);
void LockHyphenResource ();
void UnlockHyphenResource ();
#else
#define Hyphen_FntWordWrap FntWordWrap
#endif