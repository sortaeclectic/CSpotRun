/* This header file is licensed under the GPL when included in cspotrun and
 * under the zlib licensed when included with one of the libraries.
 */
/*
 * The library code is released under the zlib license as most of the code used
 * is straight from zlib. The zlib 1.1.3 that is linked in is modified 
 * slightly from the Palm zlib-1.1.3.7.
 *
 * Greg Weeks greg@durendal.org
 */

#ifndef _CSRDRMLIB_H
#define _CSRDRMLIB_H

#ifdef _BUILDING_LIB
#define _LIB_TRAP(trapNum)
#else
#define _LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


typedef struct zrecord_s {
  UInt8 sizeh;
  UInt8 sizel;
  UInt8 data[9000];
} zrecord;

typedef struct {
  UInt32 btext;
  UInt32 etext;
} ctext;

/* major version is 1 minor version is 0 */
#define CSR_DRM_LIB_INTERFACE_VERSION 0x0100

#ifndef _BUILDING_LIB
extern void start(void);
extern void _RelocateChain(void);
ctext _ctext = {(UInt32)start, (UInt32)_RelocateChain};
Err (*decrypt)(UInt16, UInt8*, UInt8*, int);
#define csr_drm_lib_decrypt(a,b,c,d) (*decrypt)(a,b,c,d)
#endif

Err csr_drm_lib_open(UInt16 refNum) _LIB_TRAP(sysLibTrapOpen);
Err csr_drm_lib_close(UInt16 refNum) _LIB_TRAP(sysLibTrapClose);
Err csr_drm_lib_get_int_version(UInt16, UInt16*) _LIB_TRAP(sysLibTrapCustom);
Err csr_drm_lib_get_decrypt(UInt16, UInt32*, ctext*) _LIB_TRAP(sysLibTrapCustom+1);

#endif
