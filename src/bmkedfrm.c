/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2000  by Bill Clagett (wtc@pobox.com)
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

#include <Common.h>
#include <System/SysAll.h>
#include <System/SysEvtMgr.h>
#include <UI/UIAll.h>
#include <List.h>

#include "resources.h"
#include "callback.h"
#include "bmkedfrm.h"
#include "bmknamefrm.h"
#include "bmk.h"

#ifdef ENABLE_BMK

static Boolean	_BmkEdFormHandleEvent(EventType *e);

FormPtr	formPtr;
ListPtr	listPtr;

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
 * bmkListRedrawEvt to give a chance any other user of
 * the bookmark subsystem redraw their lists
 */
static Boolean _BmkEdFormHandleEvent(EventType *e)
{
	EventType listRedrawEvt = {bmkListRedrawEvt, 0, 0, 0, {}};
	UInt16 sel;
	Err err;

	switch(e->eType)
	{
	case ctlSelectEvent:
		switch (e->data.ctlSelect.controlID)
		{
		case buttonID_cancel:
			EvtAddEventToQueue(&listRedrawEvt);
			/* FrmUpdateForm(formID_main, 0); */
			FrmReturnToForm(0);
			return true;

		case buttonID_bmkGoTo:
			sel = LstGetSelection(listPtr);
			if(sel == noListSelection)
				return true;

			BmkGoTo(sel, 0);
			FrmUpdateForm(formID_main, 0);
			FrmReturnToForm(0);
			return true;

		case buttonID_bmkDelete:
			sel = LstGetSelection(listPtr);
			if(sel == noListSelection)
				return true;

			err = BmkDelete(sel);
			goto RET_REDRAW;

		case buttonID_bmkRename:
			sel = LstGetSelection(listPtr);
			if(sel == noListSelection)
				return true;
			bmkCurSel = sel;
		    	FrmPopupForm(formID_bmkName);
			return true;

		case buttonID_bmkMoveUp:
			sel = LstGetSelection(listPtr);
			if(sel == noListSelection)
				return true;

			err = BmkMove(MOVE_UP, sel);

			if(!err)
				LstSetSelection(listPtr, sel ? sel - 1 : 0);

			goto RET_REDRAW;

		case buttonID_bmkMoveDown:
			sel = LstGetSelection(listPtr);
			if(sel == noListSelection)
				return true;

			if(sel == LstGetNumberOfItems(listPtr) - 1)
				return true;

			err = BmkMove(MOVE_DOWN, sel);

			if(!err)
				LstSetSelection(listPtr, sel + 1);

			goto RET_REDRAW;
		}
		break;


	case frmOpenEvent:
		formPtr = FrmGetActiveForm();
		listPtr = FrmGetObjectPtr(formPtr,
			FrmGetObjectIndex(formPtr, listID_bmkEd));
		BmkPopulateList(listPtr, 0, 0);
		FrmDrawForm(formPtr);
		return true;

	case bmkListRedrawEvt:
		err = 0;
		goto RET_REDRAW;
	}
	
	return false;

RET_REDRAW:
	if(!err) {
		err = BmkPopulateList(listPtr, 0, 0);
		LstDrawList(listPtr);
	}

	if(err)
		BmkReportError(err);

	return true;
}


#endif

