/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
 *
 * 26 Apr 2001, added bookmarks support, Alexey Raschepkin (apr@direct.ru)
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
#include "bmknamefrm.h"
#include "bmk.h"

#ifdef ENABLE_BMK

/* global variable                          */
/* all users of name form read the typed    */
/* string from this variable                */
/* it will contain empty string if the user */
/* canceled the operation                   */
char bmkName[BMK_NAME_SIZE + 1];

static Boolean  _BmkNameFormHandleEvent(EventType *e);

static FormPtr     formPtr;
static FieldPtr    nameFieldPtr;

Boolean BmkNameFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _BmkNameFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}

/*
 * bmk name form will enqueue bmkNameFrmOkEvt event
 * if the bookmark name has been typed and the user
 * pressed ok
 * the caller can catch this event and read 'bmkName'
 * global variable for the name
 */
static Boolean _BmkNameFormHandleEvent(EventType *e)
{
    EventType bmkNameOkEvt;
    Err err = 0;
    Char* fStr;
    MemSet(&bmkNameOkEvt, sizeof(bmkNameOkEvt), 0);
    bmkNameOkEvt.eType = bmkNameFrmOkEvt;

    switch(e->eType)
    {
    case ctlSelectEvent:
        switch (e->data.ctlSelect.controlID)
        {
        case buttonID_ok:
            fStr = FldGetTextPtr(nameFieldPtr);
            if(fStr) {
                /* only if the user typed something */
                StrNCopy(bmkName, fStr, BMK_NAME_SIZE);
                bmkName[BMK_NAME_SIZE] = '\0';
                EvtAddEventToQueue(&bmkNameOkEvt);
                FrmReturnToForm(0);
            }
            return true;

        case buttonID_cancel:
            FrmReturnToForm(0);
            return true;
        }
        break;

    case frmOpenEvent:
        formPtr = FrmGetActiveForm();
        nameFieldPtr = FrmGetObjectPtr(formPtr,
            FrmGetObjectIndex(formPtr, fieldID_bmkName));
        FldSetMaxChars(nameFieldPtr, BMK_NAME_SIZE);
    bmkName[0] = '\0';
        FrmDrawForm(formPtr);
        FrmSetFocus(formPtr,
            FrmGetObjectIndex(formPtr, fieldID_bmkName));
        return true;
    }

    return false;
}

#endif

