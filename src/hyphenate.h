#include "segments.h"

#ifdef ENABLE_HYPHEN
UInt16 Hyphen_FntWordWrap (Char* chars, UInt16 extend) TT_SEGMENT;
void LockHyphenResource () TT_SEGMENT;
void UnlockHyphenResource () TT_SEGMENT;
#else
#define Hyphen_FntWordWrap FntWordWrap
#endif

