#include "docprefs.h"

enum TAP_ACTION_ENUM
{
    TA_PAGE = 0,
    TA_HALFPAGE,
    TA_LINE,
    TA_ACTION_COUNT
};

struct APP_STATE_STR
{
    Word version;
    Word UCGUIBits;
    Word hideControls        : 1;
    Word reversePageUpDown    : 1;
    Word showPreviousLine    : 1;
    Word caseSensitive        : 1;
    enum TAP_ACTION_ENUM tapAction;
    struct DOC_PREFS_STR defaultDocPrefs;
};

extern struct APP_STATE_STR *appStatePtr;
extern VoidHand searchStringHandle;
extern Boolean    searchFromTop;

