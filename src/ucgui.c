/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2000  by Bill Clagett (wtc@pobox.com)
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

#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>

#include "resources.h"
#include "ucgui.h"
#include "appstate.h"
#include "fontselect.h"

struct UCGUI_ELEM_STR
{
    Word    id;
    Word    bitmask;
    Word    groupTitle;
};

#ifdef ENABLE_ROTATION
    #define ROTATION_CHOICES 2
#endif

#define UCGUI_ELEM_COUNT (1 + 1 + LINE_SPACING_GADGET_COUNT + \
                                  AUTOSCROLL_GADGET_COUNT + \
                                  FONT_BUTTON_COUNT + \
                                  ROTATION_BUTTON_COUNT + \
                                  SEARCH_BUTTON_COUNT)


#define DEFAULT_UCGUI_WORD (0xFFFF - (1<<11) - (1<<12))

//Note that these get added in this order!
static const struct UCGUI_ELEM_STR gElements[UCGUI_ELEM_COUNT] =
{
    {pushID_font0,            1<<5, stringID_ucguiFonts},//0x020},
    {pushID_font1,            1<<6, 0},//0x040},
    {pushID_font2,            1<<7, 0},//0x080},
    {pushID_font3,            1<<8, 0},//0x100},
#ifdef ENABLE_ROTATION
    {buttonID_rotateLeft,    1<<9, stringID_ucguiRotate},//0x020},
    {buttonID_rotateRight,    1<<10, 0},//0x020},
#endif
    {gadgetID_lineSpacing0,    1<<2, stringID_ucguiSpacing},//0x004},
    {gadgetID_lineSpacing1,    1<<3, 0},//0x008},
    {gadgetID_lineSpacing2,    1<<4, 0},//0x010},

#ifdef ENABLE_AUTOSCROLL
    {gadgetID_autoScroll,      1<<13, stringID_ucguiAutoScroll}, // 2^13
#endif

#ifdef ENABLE_SEARCH
    {buttonID_search,        1<<11, stringID_ucguiSearch},
    {buttonID_searchAgain,    1<<12, stringID_ucguiSearchAgain},
#endif

    {popupID_percent,        1<<1, stringID_ucguiGotoPerc},//0x002},
    {popupID_doc,            1<<0, stringID_ucguiPickDoc} //0x001},
};

#define OS2_UCGUI_MASK (~((Word)0x100))
#define GAP_AFTER_MASK (1<<8 | 1<<4 | 1<<10)

static void     moveListToPopup(FormPtr formPtr, Word popupID, Word listID);
static Boolean  isControlAllowed(Word id);

inline Word Ucgui_getDefaultWord() {
    return (Word) DEFAULT_UCGUI_WORD;
}

inline int Ucgui_getElementCount() {
    return UCGUI_ELEM_COUNT;
}

inline Word Ucgui_getGroupTitle(int i)
{
    return gElements[i].groupTitle;
}

inline Word Ucgui_getBitmask(int i)
{
    return gElements[i].bitmask;
}

void Ucgui_layout(FormPtr formPtr, Word visibleControlMask)
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
        Word            objectIndex =    FrmGetObjectIndex(formPtr, gElements[i].id);
        void*            objectPtr =        FrmGetObjectPtr(formPtr, objectIndex);
        //FormObjectKind    objectType =    FrmGetObjectType(formPtr, objectIndex);
        RectangleType   objectBounds;

        FrmGetObjectBounds(formPtr, objectIndex, &objectBounds);

        if((visibleControlMask & gElements[i].bitmask) && isControlAllowed(gElements[i].id))
        {
            if (x+1+objectBounds.extent.x > 160)
            {
                x = 1;
                y -= 1+rowHeight;
            }
            FrmSetObjectPosition(formPtr, objectIndex, x, y);
            FrmShowObject(formPtr, objectIndex);
            x += 1+objectBounds.extent.x;
        }
        else
        {
            FrmHideObject(formPtr, objectIndex);
            FrmSetObjectPosition(formPtr, objectIndex, 50, 200+i*2*rowHeight);
        }
        if (GAP_AFTER_MASK & gElements[i].bitmask)
            x+=2;
    }

    //basterdizing formBounds
    formBounds.extent.y = y;
    if (y < 160) formBounds.extent.y--; //Because selectors paint outside their bounds?

    formBounds.topLeft.x = formBounds.topLeft.y = 0;
    //formBounds.extent.y=(formBounds.extent.y*3)/4;//xxx for testing
    FrmSetObjectBounds(formPtr, FrmGetObjectIndex(formPtr, gadgetID_text), &formBounds);

    moveListToPopup(formPtr, popupID_doc, listID_doc);
    moveListToPopup(formPtr, popupID_percent, listID_percent);
}

static void moveListToPopup(FormPtr formPtr, Word popupID, Word listID)
{
    SWord x, y;
    Word listIndex =    FrmGetObjectIndex(formPtr, listID);
    Word popupIndex =    FrmGetObjectIndex(formPtr, popupID);

    FrmGetObjectPosition(formPtr, popupIndex, &x, &y);
    FrmSetObjectPosition(formPtr, listIndex, x, y);
}

static Boolean isControlAllowed(Word id)
{
    if (IS_FONTSELECT_PUSHID(id) && ! FS_fontIsLegal(FS_getFontByIndex(FONTSELECT_PUSHID_TO_INDEX(id))))
        return false;
    else
        return true;
}

