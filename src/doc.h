/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "rotate.h"

#define DOC_CREATOR    'REAd'
#define DOC_TYPE       'TEXt'

#define PAGEDIR_UP      (-1)
#define PAGEDIR_DOWN    (+1)

void            Doc_open(UInt16 cardNo, 
                         LocalID dbID, 
                         char name[dmDBNameLength]);
void            Doc_close();
void            Doc_drawPage();
void            Doc_scroll(int dir, enum TAP_ACTION_ENUM ta);
Boolean         Doc_linesDown(UInt16 linesToMove);
void            Doc_linesUp(UInt16 linesToMove);
void            Doc_setBounds(RectanglePtr bounds);
RectanglePtr    Doc_getGadgetBounds();
void            Doc_setFont(FontID f);
void            Doc_setPosition(UInt32 pos);
void            Doc_setPercent(UInt16 per);
UInt32          Doc_getPosition();
UInt16          Doc_getPercent();
void            Doc_setLineHeightAdjust(UInt16 i);
FontID          Doc_getFont();
UInt16          Doc_getLineHeightAdjust();
void            Doc_setOrientation(OrientationType o);
OrientationType Doc_getOrientation();
Boolean         Doc_inBottomHalf();
int             Doc_translatePageButton(int dir);
void            Doc_makeSettingsDefault();
void            Doc_doSearch(MemHandle searchStringHandle, 
                             Boolean searchFromTop, 
                             Boolean caseSensitive, 
                             UInt16 formId);
void            Doc_prepareForPixelScrolling();

UInt16          Doc_getDbMode();
DmOpenRef       Doc_getDbRef();
UInt16          Doc_getNumRecs();

#ifdef ENABLE_AUTOSCROLL
void            Doc_pixelScroll();
void            Doc_pixelScrollClear(Boolean forceReset);
#endif
