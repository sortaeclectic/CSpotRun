/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "docprefs.h"

enum TAP_ACTION_ENUM
{
    TA_PAGE = 0,
    TA_HALFPAGE,
    TA_LINE,
    TA_ACTION_COUNT
};

enum AUTOSCROLL_TYPE_ENUM
{
    ATYPE_PIXEL = 0,
    ATYPE_LINE,
    ATYPE_COUNT
};

struct APP_STATE_STR
{
    UInt16 version;
    UInt16 UCGUIBits;
    UInt16 hideControls       : 1;
    UInt16 reversePageUpDown  : 1;
    UInt16 showPreviousLine   : 1;
#ifdef ENABLE_AUTOSCROLL
    UInt16 autoScrollSpeed0   : 8;
    UInt16 autoScrollSpeed1   : 8;
    UInt16 autoScrollButton   : 1;
    enum AUTOSCROLL_TYPE_ENUM   autoScrollType;
#endif
    UInt16 caseSensitive      : 1;
    enum TAP_ACTION_ENUM tapAction;
    struct DOC_PREFS_STR defaultDocPrefs;
};

extern struct APP_STATE_STR *appStatePtr;
extern MemHandle searchStringHandle;
extern Boolean    searchFromTop;
