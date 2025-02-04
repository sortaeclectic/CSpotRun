/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <PalmOS.h>
#include "resources.h"
#include "ucgui.h"
#include "appstate.h"
#include "rotate.h"

struct UCGUI_ELEM_STR
{
    UInt16    id;
    UInt16    bitmask;
    UInt16    groupTitle;
};

#ifdef ENABLE_ROTATION
    #define ROTATION_CHOICES 2
#endif

#ifdef ENABLE_BMK
    #define BMK_UC_COUNT 1
#else
    #define BMK_UC_COUNT 0
#endif

#define UCGUI_ELEM_COUNT (1 + 1 + LINE_SPACING_GADGET_COUNT + \
                                  AUTOSCROLL_GADGET_COUNT + \
                                  FONT_BUTTON_COUNT + \
                                  ROTATION_BUTTON_COUNT + \
                                  SEARCH_BUTTON_COUNT + BMK_UC_COUNT + \
                                  JUSTIFY_GADGET_COUNT)


#define DEFAULT_UCGUI_WORD (0xFFFF - (1<<11) - (1<<12))

//Note that these get added in this order!
static const struct UCGUI_ELEM_STR gElements[] =
{
    {pushID_font,            1<<5, stringID_ucguiFonts},//0x020},
#ifdef ENABLE_ROTATION
    {buttonID_rotateLeft,    1<<9, stringID_ucguiRotate},//0x020},
    {buttonID_rotateRight,    1<<10, 0},//0x020},
#endif
    {gadgetID_lineSpacing0,    1<<2, stringID_ucguiSpacing},//0x004},
    {gadgetID_lineSpacing1,    1<<3, 0},//0x008},
    {gadgetID_lineSpacing2,    1<<4, 0},//0x010},

    {gadgetID_justify,      1<<15, stringID_ucguiJustify}, // 2^15

#ifdef ENABLE_AUTOSCROLL
    {gadgetID_autoScroll,      1<<13, stringID_ucguiAutoScroll}, // 2^13
#endif

#ifdef ENABLE_SEARCH
    {buttonID_search,        1<<11, stringID_ucguiSearch},
    {buttonID_searchAgain,    1<<12, stringID_ucguiSearchAgain},
#endif

    {popupID_percent,        1<<1, stringID_ucguiGotoPerc},//0x002},
    {popupID_doc,            1<<0, stringID_ucguiPickDoc}, //0x001},

#ifdef ENABLE_BMK
    {popupID_bmk,   1<<14, stringID_ucguiBmk},
#endif
};

#define GAP_AFTER_MASK (1<<5 | 1<<8 | 1<<4 | 1<<10)

static void     moveListToPopup(FormPtr formPtr, UInt16 popupID, UInt16 listID);
static Boolean  isControlAllowed(UInt16 id);

inline UInt16 Ucgui_getDefaultUInt16() {
    return (UInt16) DEFAULT_UCGUI_WORD;
}

inline int Ucgui_getElementCount() {
    return UCGUI_ELEM_COUNT;
}

inline UInt16 Ucgui_getGroupTitle(int i)
{
    return gElements[i].groupTitle;
}

inline UInt16 Ucgui_getBitmask(int i)
{
    return gElements[i].bitmask;
}

Boolean Ucgui_gadgetVisible(FormPtr formPtr, UInt16 objectIndex)
{
    return NULL != FrmGetGadgetData(formPtr, objectIndex);
}

static void myShowObject(FormPtr formPtr, UInt16 objectIndex, Boolean show)
{
    if (FrmGetObjectType(formPtr, objectIndex) != frmGadgetObj)
        if (show)
            FrmShowObject(formPtr, objectIndex);
        else
            FrmHideObject(formPtr, objectIndex);
    else
        FrmSetGadgetData(formPtr, objectIndex, show ? (void*)0x1 : NULL );
}

void Ucgui_layout(FormPtr formPtr, UInt16 visibleControlMask)
{
    int rowHeight = 12; //xxx
    int x, y;
    int i;

    RectangleType formBounds;

    FrmGetFormBounds(formPtr, &formBounds);

    x = 160;
    y = formBounds.extent.y;

    for (i=0; i<UCGUI_ELEM_COUNT; i++)
    {
        UInt16 objectIndex =  FrmGetObjectIndex(formPtr, gElements[i].id);
        void* objectPtr =  FrmGetObjectPtr(formPtr, objectIndex);
        RectangleType objectBounds;

        FrmGetObjectBounds(formPtr, objectIndex, &objectBounds);

        if((visibleControlMask & gElements[i].bitmask)
           && isControlAllowed(gElements[i].id))
        {
            if (x+1+objectBounds.extent.x > 160)
            {
                x = 1;
                y -= 1+rowHeight;
            }
            FrmSetObjectPosition(formPtr, objectIndex, x, y);
            myShowObject(formPtr, objectIndex, true);
            x += 1+objectBounds.extent.x;
        }
        else
        {
            myShowObject(formPtr, objectIndex, false);
        }
        if (GAP_AFTER_MASK & gElements[i].bitmask)
            x+=2;
    }

    //basterdizing formBounds
    formBounds.extent.y = y;
    if (y < 160)
        formBounds.extent.y-=2; //Because selectors paint outside their bounds?

    formBounds.topLeft.x = formBounds.topLeft.y = 0;
    FrmSetObjectBounds(formPtr, FrmGetObjectIndex(formPtr, gadgetID_text), &formBounds);

    moveListToPopup(formPtr, popupID_doc, listID_doc);
    moveListToPopup(formPtr, popupID_percent, listID_percent);
}

static void moveListToPopup(FormPtr formPtr, UInt16 popupID, UInt16 listID)
{
    Int16 x, y;
    UInt16 listIndex =    FrmGetObjectIndex(formPtr, listID);
    UInt16 popupIndex =    FrmGetObjectIndex(formPtr, popupID);

    FrmGetObjectPosition(formPtr, popupIndex, &x, &y);
    FrmSetObjectPosition(formPtr, listIndex, x, y);
}

static Boolean isControlAllowed(UInt16 id)
{
#ifdef ENABLE_ROTATION
    if (id == buttonID_rotateLeft || id == buttonID_rotateRight)
        return RotCanDoRotation();
#endif
    return true;
}

