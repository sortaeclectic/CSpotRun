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

#ifdef ENABLE_SEARCH
#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>

#include "resources.h"
#include "callback.h"
#include "app.h"
#include "appstate.h"
#include "searchform.h"
#include "mainform.h"
#include "doc.h"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void        HandleFormOpenEvent();
static void        HandleFormCloseEvent();
static Boolean    _SearchFormHandleEvent(EventType *e);

FormPtr        formPtr;
FieldPtr    inputFieldPtr;

Boolean SearchFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _SearchFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}

static Boolean _SearchFormHandleEvent(EventType *e)
{
    switch(e->eType)
    {
        case ctlSelectEvent:

            switch (e->data.ctlSelect.controlID)
            {
                case buttonID_ok:
                    searchFromTop = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_searchFromTop)));
                    appStatePtr->caseSensitive = CtlGetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_caseSensitive)));
                    Doc_doSearch(searchStringHandle, searchFromTop, appStatePtr->caseSensitive, formID_main);

                    HandleFormCloseEvent();// Because this form doesnt seem to get a frmCloseEvent on FrmReturnToForm.
                    FrmReturnToForm(0);
                    return true;
                case buttonID_cancel:
                    HandleFormCloseEvent();// Because this form doesnt seem to get a frmCloseEvent on FrmReturnToForm.
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
    inputFieldPtr = FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, fieldID_searchString));

    FldSetTextHandle(inputFieldPtr, (Handle) searchStringHandle);
    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_searchFromTop)), false);
    CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, checkboxID_caseSensitive)), appStatePtr->caseSensitive);

    FrmDrawForm(formPtr);
    FrmSetFocus(formPtr, FrmGetObjectIndex(formPtr, fieldID_searchString));
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void HandleFormCloseEvent()
{
    FldSetTextHandle(inputFieldPtr, NULL);
    formPtr = NULL;
}
#endif
