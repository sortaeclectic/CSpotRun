/* This is from the the old CharAttr.h.
 * They took it out of the SDK. But the trap must still be
 * there, right? */

#ifdef NON_INTERNATIONAL
const UInt8 *GetCharCaselessValue (void)
SYS_TRAP(sysTrapGetCharCaselessValue);
#else
#defineGetCharCaselessValue()_Obsolete__use_TxtCaselessCompare
#endif
