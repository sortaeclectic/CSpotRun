#ifndef __SEGMENTS_H__
#define __SEGMENTS_H__

#define BMK_SEGMENT  __attribute__ ((section ("bmk")))
#define TT_SEGMENT //__attribute__ ((section ("tt")))
#define UCGUI_SEGMENT //__attribute ((section ("ucgui")))
#define PREFS_SEGMENT // __attribute ((section ("prefs")))
#define DECODE_SEGMENT __attribute ((section ("decode")))
#define ROTATE_SEGMENT __attribute ((section ("rotate")))
#define MAINFORM_SEGMENT __attribute ((section ("mf")))
#endif /* __SEGMENTS_H__ */
