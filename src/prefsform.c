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
#include "callback.h"
#include "app.h"
#include "appstate.h"
#include "controlsform.h"
#include "ucgui.h"
#include "mainform.h"
#include "doc.h"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void        HandleFormOpenEvent();
static void        HandleFormCloseEvent();
static Boolean    _PrefsFormHandleEvent(EventType *e);
static void        _prefsToGui();
static void        _guiToPrefs();

#ifdef ENABLE_AUTOSCROLL
static Char         scrollSpeedString0[] = "xxx";
static Char         scrollSpeedString1[] = "xxx";
static void         _updateAutoScrollSpeed(int which, int selection);
#endif


FormPtr        formPtr;

Boolean PrefsFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _PrefsFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}

static Boolean _PrefsFormHandleEvent(EventType *e)
{
    switch(e->eType)
    {
        case ctlSelectEvent:

            switch (e->data.ctlSelect.controlID)
            {
                case buttonID_ok:
                    _guiToPrefs();
                    FrmReturnToForm(0);
                    return true;
                case buttonID_cancel:
                    FrmReturnToForm(0);
                    return true;
            }
            break;
#ifdef ENABLE_AUTOSCROLL
        case popSelectEvent:
            switch (e->data.popSelect.listID)
            {
                case listID_autoScrollSpeed0:
                    _updateAutoScrollSpeed(ATYPE_PIXEL, e->data.popSelect.selection);
                    return true;
                break;
                case listID_autoScrollSpeed1:
                    _updateAutoScrollSpeed(ATYPE_LINE, e->data.popSelect.selection);
                    return true;
                break;
            }
        break;
#endif
        case frmOpenEvent:
            HandleFormOpenEvent();
            return true;
        case frmCloseEvent:
            HandleFormCloseEvent();
            return false;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormOpenEvent()
{
    formPtr = FrmGetActiveForm();
    _prefsToGui();
    FrmDrawForm(formPtr);
}

static void _prefsToGui()
{
    int i;

    for (i=0; i<TA_ACTION_COUNT; i++)
        CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, pushID_tapAction0+i)), i == appStatePtr->tapAction);

#ifdef ENABLE_AUTOSCROLL
    for (i=0; i <ATYPE_COUNT; i++)
        CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, pushID_autoScrollType0+i)), i == appStatePtr->autoScrollType);
#endif

    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_reversePageButtons)), appStatePtr->reversePageUpDown);
    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_showLine)), appStatePtr->showPreviousLine);
#ifdef ENABLE_AUTOSCROLL
    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_toggleAsAddButt)), appStatePtr->autoScrollButton);

    StrPrintF(scrollSpeedString0, "%d", (int)appStatePtr->autoScrollSpeed0);

    CtlSetLabel(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_autoScrollSpeed0)), scrollSpeedString0);

    StrPrintF(scrollSpeedString1, "%d", (int)appStatePtr->autoScrollSpeed1);

    CtlSetLabel(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_autoScrollSpeed1)), scrollSpeedString1);
#endif
}

static void  _guiToPrefs()
{
    int i;

    for (i=0; i<TA_ACTION_COUNT; i++)
        if (CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, pushID_tapAction0+i))))
            appStatePtr->tapAction = i;

#ifdef ENABLE_AUTOSCROLL
    for (i=0; i<ATYPE_COUNT; i++)
        if (CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, pushID_autoScrollType0+i))))
            appStatePtr->autoScrollType = i;
#endif

    appStatePtr->reversePageUpDown = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_reversePageButtons)));
    appStatePtr->showPreviousLine = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_showLine)));
#ifdef ENABLE_AUTOSCROLL
    appStatePtr->autoScrollButton = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_toggleAsAddButt)));
#endif
}


#ifdef ENABLE_AUTOSCROLL
void _updateAutoScrollSpeed(int which, int selection)
{
    if(which == ATYPE_PIXEL)
    {
        appStatePtr->autoScrollSpeed0 = ((selection + 1) * 5);

        StrPrintF(scrollSpeedString0, "%d", appStatePtr->autoScrollSpeed0);

        CtlSetLabel(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_autoScrollSpeed0)), scrollSpeedString0);
    }
    else if(which == ATYPE_LINE)
    {
        appStatePtr->autoScrollSpeed1 = ((selection + 10) * 10);

        StrPrintF(scrollSpeedString1, "%d", appStatePtr->autoScrollSpeed1);

        CtlSetLabel(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, popupID_autoScrollSpeed1)), scrollSpeedString1);
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormCloseEvent()
{
    formPtr = NULL;
}

