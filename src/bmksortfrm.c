/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2000  by Bill Clagett (wtc@pobox.com)
 *
 * 28 Apr 2001, added bookmarks support, Alexey Raschepkin (apr@direct.ru)
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
#include <System/SysEvtMgr.h>
#include <UI/UIAll.h>

#include "resources.h"
#include "callback.h"
#include "bmksortfrm.h"
#include "bmk.h"

#ifdef ENABLE_BMK

static Boolean  _BmkSortFormHandleEvent(EventType *e);

FormPtr     formPtr;

Boolean BmkSortFormHandleEvent(EventType *e)
{
    Boolean        handled = false;

    CALLBACK_PROLOGUE
    handled = _BmkSortFormHandleEvent(e);
    CALLBACK_EPILOGUE

    return handled;
}

static Boolean _BmkSortFormHandleEvent(EventType *e)
{
    EventType listRedrawEvt = {bmkListRedrawEvt, 0, 0, 0, {}};
    Err err;

    switch(e->eType)
    {
    case ctlSelectEvent:
        switch (e->data.ctlSelect.controlID)
        {
        case buttonID_cancel:
            FrmReturnToForm(0);
            return true;

        case buttonID_bmkSortPosition:
            err = BmkSort(SORT_POS);
            if(err)
                BmkReportError(err);
            EvtAddEventToQueue(&listRedrawEvt);
            FrmReturnToForm(0);
            return true;

        case buttonID_bmkSortName:
            err = BmkSort(SORT_NAME);
            if(err)
                BmkReportError(err);
            EvtAddEventToQueue(&listRedrawEvt);
            FrmReturnToForm(0);
            return true;
        }
        break;

    case frmOpenEvent:
        formPtr = FrmGetActiveForm();
        FrmDrawForm(formPtr);
        return true;
    }
    return false;
}

#endif

