/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998,1999  by Bill Clagett (wtc@pobox.com)
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
#include "callback.h"
#include "app.h"
#include "appstate.h"
#include "controlsform.h"
#include "ucgui.h"
#include "mainform.h"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void        HandleFormOpenEvent();
static void        HandleFormCloseEvent();
static Boolean    _PrefsFormHandleEvent(EventType *e);
static void        _prefsToGui();
static void        _guiToPrefs();

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

    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_reversePageButtons)), appStatePtr->reversePageUpDown);
    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_showLine)), appStatePtr->showPreviousLine);
}

static void  _guiToPrefs()
{
    int i;

    for (i=0; i<TA_ACTION_COUNT; i++)
        if (CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, pushID_tapAction0+i))))
            appStatePtr->tapAction = i;

    appStatePtr->reversePageUpDown = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_reversePageButtons)));
    appStatePtr->showPreviousLine = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_showLine)));
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormCloseEvent()
{
    formPtr = NULL;
}

