/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include <PalmOS.h>

#include "resources.h"
#include "mainform.h"
#include "callback.h"
#include "app.h"
#include "doclist.h"
#include "rotate.h"
#include "appstate.h"
#include "doc.h"
#include "docprefs.h"
#include "ucgui.h"
#include "fontselect.h"
#include "bmk.h"
#include "bmknamefrm.h"
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void        HandleFormOpenEvent();
static void        HandleFormCloseEvent();
static Boolean    _MainFormHandleEvent(EventType *e);
static void        HandleDocSelect(int documentIndex);

static void        _drawLineSpacingGadgets();
static void        _drawLineSpacingGadget(int index);
static void        _changeLineSpacing(int index);

#ifdef ENABLE_BMK
static void        _redrawBmkList(void);
static void        _popupBmkEd(void);
#endif

#ifdef ENABLE_AUTOSCROLL
static void         _drawAutoScrollGadget();
#endif

static void        _openDefaultDoc();
static void        _deleteDoc();
static void        _updatePercent();
static void        _setHandyPointers();
static void        _scroll(int direction, enum TAP_ACTION_ENUM ta);
static void        _layoutForm();
static void        _rotate(int dir);


FormPtr     formPtr;
ListPtr     docListPtr;
ListPtr     bmkListPtr;
ControlPtr  docPopupPtr;
ControlPtr  percentPopupPtr;

UInt16      bmkListIndex;

Char        percentString[] = "xxx%";
UInt16      selectableLineSpacings[LINE_SPACING_GADGET_COUNT] = {0, 1, 2};
int         _documentIndex = -1;

#ifdef ENABLE_AUTOSCROLL
static Boolean      autoScrollEnabled = false;
#endif

/* from doc.c */
extern UInt16  _dbMode;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

Boolean MainFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _MainFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}

static Boolean _MainFormHandleEvent(EventType *e)
{
    int a;
    Err err;

    _setHandyPointers();
    switch(e->eType)
    {
        case ctlSelectEvent:
            if (IS_FONTSELECT_PUSHID(e->data.ctlSelect.controlID))
            {
                FS_changeFont(FONTSELECT_PUSHID_TO_INDEX(e->data.ctlSelect.controlID));
                return true;
            }

            switch (e->data.ctlSelect.controlID)
            {
#ifdef ENABLE_ROTATION
                case buttonID_rotateLeft:
                    _rotate(-1);
                    return true;
                case buttonID_rotateRight:
                    _rotate(1);
                    return true;
#endif
#ifdef ENABLE_SEARCH
                case buttonID_search:
                    FrmPopupForm(formID_search);
                    return true;
                case buttonID_searchAgain:
                    Doc_doSearch(searchStringHandle, false, appStatePtr->caseSensitive, formID_main);
                    return true;
#endif
}
            break;
        case popSelectEvent:
            switch(e->data.popSelect.listID)
            {
                case listID_doc:
                    //if (e->data.popSelect.selection != e->data.popSelect.priorSelection)
                    HandleDocSelect(e->data.popSelect.selection);
                    return true;
                case listID_percent:
                    Doc_setPercent(10*e->data.popSelect.selection);
                    Doc_drawPage();
                    _updatePercent();
                    return true;
#ifdef ENABLE_BMK
                case listID_bmk:
		    a = BmkGetAction(e->data.popSelect.selection);
		    if(a == A_NEW) {
			    FrmPopupForm(formID_bmkName);
			    return true;
		    } else if(a == A_EDIT) {
                            _popupBmkEd();
			    return true;
		    }
		    
		    BmkGoTo(e->data.popSelect.selection, 1);
                    Doc_drawPage();
                    _updatePercent();
                    return true;
#endif
            }
            break;
        case menuEvent:
            switch (e->data.menu.itemID)
            {
#ifdef ENABLE_SEARCH
                case menuitemID_search:
                    MenuEraseStatus(NULL);
                    FrmPopupForm(formID_search);
                    return true;
                case menuitemID_searchAgain:
                    MenuEraseStatus(NULL);
                    Doc_doSearch(searchStringHandle, false, appStatePtr->caseSensitive, formID_main);
                    return true;
#endif
                case menuitemID_makeDefault:
                    MenuEraseStatus(NULL);
                    Doc_makeSettingsDefault();
                    return true;
                case menuitemID_about:
                    FrmHelp(stringID_aboutBody);
                    return true;
                case menuitemID_delete:
                    MenuEraseStatus(NULL);
                    _deleteDoc();
                    return true;
                case menuitemID_controlPrefs:
                    MenuEraseStatus(NULL);
                    FrmPopupForm(formID_controls);
                    return true;
                case menuitemID_globalPrefs:
                    FrmPopupForm(formID_globalPrefs);
                    return true;
                case menuitemID_controls:
                    MenuEraseStatus(NULL);
                    appStatePtr->hideControls = ~appStatePtr->hideControls;
                    MainForm_UCGUIChanged();
                    return true;
#ifdef ENABLE_AUTOSCROLL
                case menuitemID_autoScroll:
                    MenuEraseStatus(NULL);
                    MainForm_ToggleAutoScroll();
                    return true;
#endif
                case menuitemID_doc:
                    MenuEraseStatus(NULL);
                    CtlHitControl(docPopupPtr);
                    return true;
                case menuitemID_percent:
                    MenuEraseStatus(NULL);
                    CtlHitControl(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_percent)));
                    return true;
#ifdef ENABLE_ROTATION
                case menuitemID_rotateLeft:
                    MenuEraseStatus(NULL);
                    _rotate(-1);
                    return true;
                case menuitemID_rotateRight:
                    MenuEraseStatus(NULL);
                    _rotate(1);
                    return true;
#endif

#ifdef ENABLE_BMK
		case menuitemID_bmkAdd:
                    MenuEraseStatus(NULL);
		    FrmPopupForm(formID_bmkName);
		    return true;

		case menuitemID_bmkEd:
		    MenuEraseStatus(NULL);
                    _popupBmkEd();
		    return true;
#endif
            }

            MenuEraseStatus(NULL);
            if (IS_FONTSELECT_MENUID(e->data.menu.itemID))
            {
                FS_changeFont(FONTSELECT_MENUID_TO_INDEX(e->data.menu.itemID));
                FS_updateFontButtons(formPtr);
                return true;
            }
            if (e->data.menu.itemID >= menuitemID_lineSpacing0 && e->data.menu.itemID < (menuitemID_lineSpacing0 + LINE_SPACING_GADGET_COUNT))
            {
                _changeLineSpacing(e->data.ctlSelect.controlID - menuitemID_lineSpacing0);
                return true;
            }

            break;
        case frmUpdateEvent:
            if (e->data.frmUpdate.formID == formID_main)
            {
#ifdef ENABLE_BMK
		_redrawBmkList();
#endif

                FrmDrawForm(formPtr);
                Doc_drawPage();
                _updatePercent();
                _drawLineSpacingGadgets();
#ifdef ENABLE_AUTOSCROLL
                _drawAutoScrollGadget();
#endif
                return true;
            }
            break;
        case penDownEvent:
            {
                RectangleType r;
                int i;
                Int16 index;
                // If user clicked on the document
                if (RctPtInRectangle (e->screenX, e->screenY, Doc_getGadgetBounds()))
                {
                    _scroll(Doc_inBottomHalf(e->screenX, e->screenY) ? PAGEDIR_DOWN : PAGEDIR_UP, appStatePtr->tapAction);
                    return true;
                }
                // If user clicked on a line spacing doodad
                for (i = 0; i < LINE_SPACING_GADGET_COUNT; i++)
                {
                    index = FrmGetObjectIndex(formPtr, gadgetID_lineSpacing0+i);    
                    FrmGetObjectBounds (formPtr, index, &r);
                    if (RctPtInRectangle (e->screenX, e->screenY, &r) 
                        && Ucgui_gadgetVisible(formPtr, index))
                    {
                        _changeLineSpacing(i);
                        return true;
                    }
                }
#ifdef ENABLE_AUTOSCROLL
                index = FrmGetObjectIndex(formPtr, gadgetID_autoScroll);    
                FrmGetObjectBounds (formPtr, index, &r);
                if ( RctPtInRectangle (e->screenX, e->screenY, &r)
                     && Ucgui_gadgetVisible(formPtr, index))  {
                    MainForm_ToggleAutoScroll();
                    return true;
                }
#endif
            }
            break;
        case keyDownEvent:
            switch (e->data.keyDown.chr)
            {
                case pageDownChr:
                    _scroll(Doc_translatePageButton(PAGEDIR_DOWN), TA_PAGE);
                    return true;
                case pageUpChr:
                    _scroll(Doc_translatePageButton(PAGEDIR_UP), TA_PAGE);
                    return true;
            }
            break;
        case frmOpenEvent:
            HandleFormOpenEvent();
            return true;
        case frmCloseEvent:
            HandleFormCloseEvent();
            return false;

#ifdef ENABLE_BMK
        case bmkNameFrmOkEvt:
            /* add the bookmark, new name is in 'bmkName' */
            err = BmkAdd(bmkName, 0);
            if(err)
                BmkReportError(err);
            _redrawBmkList();
            return true;

        case bmkRedrawListEvt:
            Doc_drawPage();
            _updatePercent();
            _redrawBmkList();
            return true;
#endif
    }
    return false;
}

static void    HandleDocSelect(int documentIndex)
{
    Doc_close();
    _documentIndex = documentIndex;

    Doc_open(DocList_getCardNo(documentIndex),
        DocList_getID(documentIndex), DocList_getTitle(documentIndex));

    // When the doc was opened, prefs changed.

    //Set the document popup label
    //Changed popup trigger to simply "Doc", otherwise it resizes which causes UCGUI funkiness.
    //CtlSetLabel(docPopupPtr, DocList_getTitle(documentIndex));


    FS_updateFontButtons(formPtr);

    /* set menu accroding to the document mode */
    if(_dbMode == dmModeReadOnly)
        FrmSetMenu(formPtr, menuID_main_ro);
    else
        FrmSetMenu(formPtr, menuID_main);

    FrmUpdateForm(formID_main, 0);
}


static void _setHandyPointers()
{
    formPtr = FrmGetActiveForm();

    docListPtr = FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, listID_doc));
#ifdef ENABLE_BMK
    bmkListIndex = FrmGetObjectIndex(formPtr, listID_bmk);
    bmkListPtr = FrmGetObjectPtr(formPtr, bmkListIndex);
#endif
    docPopupPtr = FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_doc));
    percentPopupPtr = FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_percent));
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormOpenEvent()
{
    _setHandyPointers();
    DocList_populateList(docListPtr);
    _layoutForm();
    _openDefaultDoc();
}

static void _scroll(int direction, enum TAP_ACTION_ENUM ta)
{
    Doc_scroll(direction, ta);
    Doc_drawPage();
    _updatePercent();
}

static void _openDefaultDoc()
{
    int index;
    char name[dmDBNameLength];
    DocPrefs_getRecentDocName(name);

    index = DocList_getIndex(name);

    // If doc not found, open the first one
    if (index < 0 && DocList_getDocCount())
        index = 0;

    if (index >= 0)
    {
        LstSetSelection(docListPtr, index);
        HandleDocSelect(index);
    }
    else
    {
        EventType    newEvent;
        MemHandle    noDocsH;

        // No documents installed here!
        noDocsH = DmGetResource(strRsc, stringID_noDocs);
        FrmCustomAlert(alertID_error, (Char*) MemHandleLock(noDocsH), " ", " ");
        MemHandleUnlock(noDocsH);
        DmReleaseResource(noDocsH);

        // Start the App app (from page 72 of ref2.pdf)
        newEvent.eType = keyDownEvent;
        newEvent.data.keyDown.chr = launchChr;
        newEvent.data.keyDown.modifiers = commandKeyMask;
        EvtAddEventToQueue(&newEvent);
    }
}


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormCloseEvent()
{
    DocList_freeList();
    Doc_close();
    formPtr = NULL;
}

void MainForm_UCGUIChanged()
{
    _setHandyPointers();
    FrmEraseForm(formPtr);
    _layoutForm();
    FrmUpdateForm(formID_main, 0);
}


static void _layoutForm()
{
    RectangleType textBounds;
    _setHandyPointers();

    Ucgui_layout(formPtr, appStatePtr->hideControls ? 0 : appStatePtr->UCGUIBits);

    FrmGetObjectBounds(formPtr, FrmGetObjectIndex(formPtr, gadgetID_text), &textBounds);
    Doc_setBounds(&textBounds);
}

#ifdef ENABLE_AUTOSCROLL
static void _drawAutoScrollGadget()
{
    RectangleType bounds;

    Int16 objectIndex = FrmGetObjectIndex(formPtr, gadgetID_autoScroll);

    if (!Ucgui_gadgetVisible(formPtr, objectIndex))
        return;

    FrmGetObjectBounds(formPtr, objectIndex, &bounds);

    WinEraseRectangle(&bounds, 0);
    WinDrawRectangleFrame(roundFrame, &bounds);

    if(autoScrollEnabled) // show pause
    {
        WinDrawLine(bounds.topLeft.x + 2, bounds.topLeft.y + 3, bounds.topLeft.x + 2, bounds.topLeft.y + bounds.extent.y - 3);
        WinDrawLine(bounds.topLeft.x + bounds.extent.x - 3, bounds.topLeft.y + 3, bounds.topLeft.x + bounds.extent.x - 3, bounds.topLeft.y + bounds.extent.y - 3);
    }
    else // show play
    {
        WinDrawLine(bounds.topLeft.x + 2, bounds.topLeft.y + 3, bounds.topLeft.x + 2, bounds.topLeft.y + bounds.extent.y - 3);
        WinDrawLine(bounds.topLeft.x + 3, bounds.topLeft.y + 4, bounds.topLeft.x + 3, bounds.topLeft.y + bounds.extent.y - 4);
        WinDrawLine(bounds.topLeft.x + 4, bounds.topLeft.y + 5, bounds.topLeft.x + 4, bounds.topLeft.y + bounds.extent.y - 5);
        WinDrawLine(bounds.topLeft.x + 5, bounds.topLeft.y + 6, bounds.topLeft.x + 5, bounds.topLeft.y + bounds.extent.y - 6);
    }
}

void MainForm_ToggleAutoScroll()
{
    autoScrollEnabled = ! autoScrollEnabled;

    if (autoScrollEnabled)
    {
        if (ATYPE_LINE == appStatePtr->autoScrollType)
        {
            //Force it to the even pixel, even if it didnt move.
            Doc_pixelScrollClear(true);
        } else {
            Doc_pixelScrollClear(false);
            //Draw the partial line
            Doc_prepareForPixelScrolling();
        }
    }

    _drawAutoScrollGadget();
}

void MainForm_UpdateAutoScroll()
{
    if(autoScrollEnabled == true)
    {
        EvtResetAutoOffTimer();
        if(appStatePtr->autoScrollType == ATYPE_PIXEL)
        {
            Doc_pixelScroll();
        }
        else
        {
            Doc_linesDown(1);
            Doc_drawPage();
        }
        _updatePercent();
    }
}

Boolean MainForm_AutoScrollEnabled()
{
    return autoScrollEnabled;
}
#endif


static void _drawLineSpacingGadgets()
{
    int i;
    for (i = 0; i < LINE_SPACING_GADGET_COUNT; i++)
    {
        _drawLineSpacingGadget(i);
    }
}

static void _drawLineSpacingGadget(int i)
{
    RectangleType bounds;
    Boolean isSelected;
    int row;

    Int16 index = FrmGetObjectIndex(formPtr, gadgetID_lineSpacing0+i);
    
    if (!Ucgui_gadgetVisible(formPtr, index))
        return;

    FrmGetObjectBounds(formPtr, index, &bounds);

    WinDrawRectangleFrame(rectangleFrame, &bounds);

    isSelected = (selectableLineSpacings[i] == Doc_getLineHeightAdjust());

    for(row = 0; row < bounds.extent.y; row++)
    {
        Boolean rowOn;
        rowOn = (row > 0)
                    && (row < bounds.extent.y-1)
                    && (0 == ((row-selectableLineSpacings[i]+4) % (4-selectableLineSpacings[i])));
        if (rowOn)
            WinDrawGrayLine(bounds.topLeft.x, bounds.topLeft.y+row, bounds.topLeft.x+bounds.extent.x-1, bounds.topLeft.y+row);
        else
            if (isSelected)
                WinDrawLine(bounds.topLeft.x, bounds.topLeft.y+row, bounds.topLeft.x+bounds.extent.x-1, bounds.topLeft.y+row);
            else
                WinEraseLine(bounds.topLeft.x, bounds.topLeft.y+row, bounds.topLeft.x+bounds.extent.x-1, bounds.topLeft.y+row);
    }
}

static void    _updatePercent()
{
    StrPrintF(percentString, "%d%s", Doc_getPercent(), "%");
    CtlSetLabel(percentPopupPtr, percentString);
}

static void _deleteDoc()
{
    if (0 == FrmCustomAlert(alertID_confirmDelete, DocList_getTitle(_documentIndex), " ", " "))
    {
        Doc_close();
        DmDeleteDatabase(DocList_getCardNo(_documentIndex), DocList_getID(_documentIndex));
        _documentIndex = -1;
        DocList_freeList();
        DocList_populateList(docListPtr);
        _openDefaultDoc();
    }

    DocPrefs_cleanUpPrefs();
}

static void _changeLineSpacing(int index)
{
    UInt16 lineHeightAdjust = selectableLineSpacings[index];

    if (Doc_getLineHeightAdjust() != lineHeightAdjust)
    {
        Doc_setLineHeightAdjust(lineHeightAdjust);
        Doc_drawPage();
    }
    _drawLineSpacingGadgets();
}

#ifdef ENABLE_ROTATION
static void        _rotate(int dir)
{
    OrientationType or = Doc_getOrientation();

    or = (or+ORIENTATION_COUNT+dir) % ORIENTATION_COUNT;
    Doc_setOrientation(or);
    Doc_drawPage();
}
#endif

#ifdef ENABLE_BMK
void _redrawBmkList(void)
{
	RectangleType r;
	RectangleType rf;
	Err err;

	err = BmkPopulateList(bmkListPtr, 1, 1);
	if(err)
		BmkReportError(err);

	FrmGetFormBounds(formPtr, &rf);
	FrmGetObjectBounds(formPtr, bmkListIndex, &r);
	r.topLeft.y = rf.extent.y - r.extent.y;
	FrmSetObjectBounds(formPtr, bmkListIndex, &r);
}

void _popupBmkEd(void)
{
	if(_dbMode == dmModeReadWrite)
		FrmPopupForm(formID_bmkEd);
	else
		FrmPopupForm(formID_bmkEd_ro);
}
#endif

