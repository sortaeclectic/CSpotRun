#include "rotate.h"

#define DOC_CREATOR    'REAd'
#define DOC_TYPE       'TEXt'

#define PAGEDIR_UP      (-1)
#define PAGEDIR_DOWN    (+1)

void            Doc_open(UInt cardNo, LocalID dbID, char name[dmDBNameLength]);
void            Doc_close();
void            Doc_drawPage();
void            Doc_scroll(int dir, enum TAP_ACTION_ENUM ta);
void            Doc_linesDown(Word linesToMove);
void            Doc_linesUp(Word linesToMove);
void            Doc_setBounds(RectanglePtr bounds);
RectanglePtr    Doc_getGadgetBounds();
void            Doc_setFont(FontID f);
void            Doc_setPercent(UInt per);
UInt            Doc_getPercent();
void            Doc_setLineHeightAdjust(UShort i);
FontID          Doc_getFont();
UShort          Doc_getLineHeightAdjust();
void            Doc_setOrientation(OrientationType o);
OrientationType Doc_getOrientation();
Boolean         Doc_inBottomHalf();
int             Doc_translatePageButton(int dir);
void            Doc_makeSettingsDefault();
void            Doc_doSearch(VoidHand searchStringHandle, Boolean searchFromTop, Boolean caseSensitive, Word formId);
