/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "segments.h"
UInt16    Ucgui_getDefaultUInt16() UCGUI_SEGMENT;
UInt16    Ucgui_getGroupTitle(int i) UCGUI_SEGMENT;
UInt16    Ucgui_getBitmask(int i) UCGUI_SEGMENT;
int     Ucgui_getElementCount() UCGUI_SEGMENT;
void    Ucgui_layout(FormPtr formPtr, UInt16 visibleControlMask) UCGUI_SEGMENT;
Boolean Ucgui_gadgetVisible(FormPtr formPtr, UInt16 objectIndex) UCGUI_SEGMENT;
