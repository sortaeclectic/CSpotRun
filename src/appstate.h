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
    Word version;
    Word UCGUIBits;
    Word hideControls       : 1;
    Word reversePageUpDown  : 1;
    Word showPreviousLine   : 1;
#ifdef ENABLE_AUTOSCROLL
    Word autoScrollSpeed0   : 8;
    Word autoScrollSpeed1   : 8;
    Word autoScrollButton   : 1;
    enum AUTOSCROLL_TYPE_ENUM   autoScrollType;
#endif
    Word caseSensitive      : 1;
    enum TAP_ACTION_ENUM tapAction;
    struct DOC_PREFS_STR defaultDocPrefs;
};

extern struct APP_STATE_STR *appStatePtr;
extern VoidHand searchStringHandle;
extern Boolean    searchFromTop;
