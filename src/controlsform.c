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
static Boolean    _ControlsFormHandleEvent(EventType *e);
static void        _setStates();
static UInt16        _getStates();

FormPtr        formPtr;

Boolean ControlsFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _ControlsFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}

static Boolean _ControlsFormHandleEvent(EventType *e)
{
    switch(e->eType)
    {
        case ctlSelectEvent:

            switch (e->data.ctlSelect.controlID)
            {
                case buttonID_ok:
                    if (0 != _getStates())
                        appStatePtr->UCGUIBits = _getStates();
                    HandleFormCloseEvent();// Because this form doesnt seem to get a frmCloseEvent on FrmReturnToForm.
                    FrmReturnToForm(0);
                    MainForm_UCGUIChanged();
                    return true;
                case buttonID_cancel:
                    HandleFormCloseEvent();// Because this form doesnt seem to get a frmCloseEvent on FrmReturnToForm.
                    FrmReturnToForm(0);
                    return true;
            }
            break;
/*        case frmUpdateEvent:
            if (e->data.frmUpdate.formID == formID_main)
            {
                FrmDrawForm(formPtr);
                return true;
            }
            break;
*/
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
    int i;
    RectangleType ctlBounds;
    int y = 3;
    int x = 5;
    int fieldCount = 0;

    formPtr = FrmGetActiveForm();

    for (i=0; i<Ucgui_getElementCount(); i++)
    {
        UInt16        ctlIndex = FrmGetObjectIndex(formPtr, checkboxID_UCGUI0+i);
        ControlPtr    ctlPtr = FrmGetObjectPtr(formPtr, ctlIndex);

        if (Ucgui_getGroupTitle(i))
        {
            UInt16    fieldID = fieldID_UCGUI0+fieldCount;
            UInt16    fieldIndex = FrmGetObjectIndex(formPtr, fieldID);

            x = 5;
            y += 13;

            //Set text of label
            //FldSetTextPtr((FieldPtr)FrmGetObjectPtr(formPtr, fieldIndex), UcguiElems[i].groupTitle);
            FldSetTextHandle((FieldPtr)FrmGetObjectPtr(formPtr, fieldIndex), (MemHandle)DmGetResource(strRsc, Ucgui_getGroupTitle(i)));

            //Position the label
            FrmSetObjectPosition(formPtr, fieldIndex, x, y);

            //Move cursor to right of label.
            FrmGetObjectBounds(formPtr, fieldIndex, &ctlBounds);
            x += ctlBounds.extent.x+3;

            fieldCount++;
        }

        CtlSetLabel(ctlPtr, "");
        FrmSetObjectPosition(formPtr, ctlIndex, x, y);

        FrmGetObjectBounds(formPtr, ctlIndex, &ctlBounds);
        x += ctlBounds.extent.x-3;
    }

    _setStates();

    FrmDrawForm(formPtr);
}

static void _setStates()
{
    int i;

    for (i=0; i<Ucgui_getElementCount(); i++)
        CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_UCGUI0+i)),
                        Ucgui_getBitmask(i) & appStatePtr->UCGUIBits);
}

static UInt16 _getStates()
{
    int     i;
    UInt16 bits = 0;

    for (i=0; i<Ucgui_getElementCount(); i++)
        if (CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_UCGUI0+i))))
            bits |= Ucgui_getBitmask(i);

    return bits;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormCloseEvent()
{
    int i;
    int fieldCount = 0;
    for (i=0; i<Ucgui_getElementCount(); i++)
    {
        if (Ucgui_getGroupTitle(i))
        {
            UInt16        fieldID = fieldID_UCGUI0+fieldCount;
            UInt16        fieldIndex = FrmGetObjectIndex(formPtr, fieldID);
            FieldPtr    fieldP = (FieldPtr)FrmGetObjectPtr(formPtr, fieldIndex);
            MemHandle    stringH = (MemHandle)FldGetTextHandle(fieldP);

            FldSetTextHandle(fieldP, NULL);
            DmReleaseResource(stringH);

            fieldCount++;
        }
    }
    formPtr = NULL;
}
