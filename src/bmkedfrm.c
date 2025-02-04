/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
 *
 * 27 Apr 2001, added bookmarks support, Alexey Raschepkin (apr@direct.ru)
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
#include "bmkedfrm.h"
#include "bmknamefrm.h"
#include "bmk.h"

#ifdef ENABLE_BMK

static Boolean  _BmkEdFormHandleEvent(EventType *e);


static FormPtr  formPtr;
static ListPtr  listPtr;

Boolean BmkEdFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _BmkEdFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}


/*
 * this form works with the bookmark list and so before
 * returning to any other form it should enqueue
 * bmkRedrawListEvt to give a chance any other user of
 * the bookmark subsystem redraw their lists
 */
static Boolean _BmkEdFormHandleEvent(EventType *e)
{
    EventType listRedrawEvt;
    Err err;
    /* sel is static because it needs to keep selection */
    /* position after returning from the bmknamefrm */
    static Int16 sel = noListSelection;

    MemSet(&listRedrawEvt, sizeof(listRedrawEvt), 0);
    listRedrawEvt.eType = bmkRedrawListEvt;

    switch(e->eType)
    {
    case ctlSelectEvent:
        switch (e->data.ctlSelect.controlID)
        {
        case buttonID_cancel:
            EvtAddEventToQueue(&listRedrawEvt);
            FrmReturnToForm(0);
            return true;

        case buttonID_bmkGoTo:
            sel = LstGetSelection(listPtr);
            if(sel == noListSelection)
                return true;

            BmkGoTo(sel, 0);
            EvtAddEventToQueue(&listRedrawEvt);
            FrmReturnToForm(0);
#if 0
/*
 * this form update causes flashing on some roms
 * after returning to the main form
 * think this is because update event is enqueued
 * twice, once by the os and then by FrmUpdateForm()
 */
            FrmUpdateForm(formID_main, 0);
#endif
            return true;

        case buttonID_bmkDelete:
            sel = LstGetSelection(listPtr);
            if(sel == noListSelection)
                return true;

            err = BmkDelete(sel);

            /* if current list selection is 0  */
            /* then, since sel is unsigned,    */
            /* substricting of 1 will wrap sel */
            /* and it will hold 0xffff which is */
            /* equal to 'noListSelection'      */
            if(sel == LstGetNumberOfItems(listPtr) - 1)
                sel--;

            goto RET_REDRAW;

        case buttonID_bmkRename:
            sel = LstGetSelection(listPtr);
            if(sel == noListSelection)
                return true;
                FrmPopupForm(formID_bmkName);
            return true;

        case buttonID_bmkMoveUp:
            sel = LstGetSelection(listPtr);
            if(sel == noListSelection)
                return true;

            err = BmkMove(MOVE_UP, sel);

            if(!err) sel = sel ? --sel : 0;

            goto RET_REDRAW;

        case buttonID_bmkMoveDown:
            sel = LstGetSelection(listPtr);
            if(sel == noListSelection)
                return true;

            if(sel == LstGetNumberOfItems(listPtr) - 1)
                return true;

            err = BmkMove(MOVE_DOWN, sel);

            if(!err) sel++;

            goto RET_REDRAW;

        case buttonID_bmkSort:
    {
        int s = FrmAlert(alertID_bmkSort);
        if (s < 2) {
            err = BmkSort(s == 0 ? SORT_NAME : SORT_POS);
            if(err)
            BmkReportError(err);
                goto RET_REDRAW;
        } else
            return true;
    }
        case buttonID_bmkDelAll:
            if (FrmAlert(alertID_bmkConfirmDel) == 0) {
             err = BmkDeleteAll();
                 goto RET_REDRAW;
        }
        return true;
        }
        break;


    case frmOpenEvent:
        sel = noListSelection;
        formPtr = FrmGetActiveForm();
        listPtr = FrmGetObjectPtr(formPtr,
            FrmGetObjectIndex(formPtr, listID_bmkEd));
        BmkPopulateList(listPtr, 0, 0);
        FrmDrawForm(formPtr);
        return true;

    case bmkNameFrmOkEvt:
        /* rename the bookmark, new name is in 'bmkName' */
        err = BmkRename(sel, bmkName);
        goto RET_REDRAW;
    }

    return false;

RET_REDRAW:
    if(!err) {
        err = BmkPopulateList(listPtr, 0, 0);
        LstDrawList(listPtr);
        LstSetSelection(listPtr, sel);
    } else {
        BmkReportError(err);
    }

    return true;
}


#endif

