/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
 *
 * 30 Apr 2001, added bookmarks support, Alexey Raschepkin (apr@direct.ru)
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

#include "bmk.h"
#include "appstate.h"
#include "doc.h"
#include "resources.h"

#ifdef ENABLE_BMK


/* globals */

static char **      bmkListBuf = NULL;
static int          bmkLBSize = 0;      /* number of pointers */
static int          scuts_num = 0;

                        /* in the list buffer */
static Char*      bmkAddStr = NULL;
static Char*      bmkEdStr = NULL;
MemHandle bmkAddStrHandle;
MemHandle bmkEdStrHandle;

/* local functions */

static int  BmkFindByName(char *);
static int  BmkFindByNameAndPos(char *, UInt32);
static Err  DoBmkAdd(char *name);

static Err  DoBmkReplace(UInt16 idx, char *name);
static Err  DoBmkReplaceWithPos(UInt16 idx, char *name, UInt32 pos);

static Err  DoBmkMoveUp(UInt16 idx);
static Err  DoBmkMoveDown(UInt16 idx);

static Err  BmkInsertionSort(int);

static Err  BmkRefillListBuf(void);
static void BmkClearListBuf(void);

/*
 * BmkStart()
 * initialize the bookmark subsystem
 */
Err BmkStart(void)
{
    Err err;

    bmkAddStrHandle = DmGetResource(strRsc, stringID_bmkAdd);
    bmkAddStr = (Char*)MemHandleLock(bmkAddStrHandle);
    bmkEdStrHandle = DmGetResource(strRsc, stringID_bmkEd);
    bmkEdStr = (Char*)MemHandleLock(bmkEdStrHandle);

    return 0;
}


/*
 * BmkStop()
 * cleanup the bookmark subsystem
 */
void BmkStop(void)
{
    BmkClearListBuf();

    /* release all string resources */

    if(bmkAddStr) {
        MemHandleUnlock(bmkAddStrHandle);
        DmReleaseResource(bmkAddStrHandle);
    }

    if(bmkEdStr) {
        MemHandleUnlock(bmkEdStrHandle);
        DmReleaseResource(bmkEdStrHandle);
    }
}


/*
 * BmkCloseDoc()
 * close current doc and free associated resources
 */
Err BmkCloseDoc(void)
{
    BmkClearListBuf();
    return 0;
}


/*
 * BmkAdd()
 * add bookmark for the current document
 * with the current position
 * and update the list buffer
 *
 * if force is true, the bookmark will be added
 * even if a bookmark with the same name exists.
 */
Err BmkAdd(char *name, int force)
{
    UInt16 a_ret;
    int f_rec = -1;

    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    if(!force)
        f_rec = BmkFindByName(name);

    if(f_rec != -1) {
        /* dup alert returns 0 to replace */
        /* 1 to create new and 2 to cancel */
        a_ret = FrmAlert(alertID_bmkDupName);

        if(a_ret == 2)
            return 0;

        if(a_ret == 0)
            return DoBmkReplace((UInt16)f_rec, name);
    }

    /* create new bmk entry for the document */
    return DoBmkAdd(name);
}


Err DoBmkAdd(char *name)
{
    UInt16 idx = dmMaxRecordIndex;
    MemHandle h;
    void* p;
    bmk_rec_t b; /* temp copy of the record. */

    /* create new record */
    h = DmNewRecord(Doc_getDbRef(), &idx, sizeof(bmk_rec_t));
    if(!h)
        return DmGetLastErr();

    p = MemHandleLock(h);

    /* initialize the record */
    b.pos = Doc_getPosition();
    StrNCopy(b.name, name, 15);
    b.name[15] = '\0';
    DmWrite(p, 0, &b, sizeof(b));

    MemHandleUnlock(h);
    DmReleaseRecord(Doc_getDbRef(), idx, 0);

    /* update the list buffer */
    BmkClearListBuf();

    return 0;
}


/*
 * BmkRename()
 * here I assume that selection comes from the list
 * without shortcuts
 */
Err BmkRename(int sel, char *name)
{
    bmk_rec_t *b;
    int idx = -1;

    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    sel += scuts_num;

    b = (bmk_rec_t *)bmkListBuf[sel];
    idx = BmkFindByNameAndPos(b->name, b->pos);

    if(idx == -1)
        return DmGetLastErr();

    /* DoBmkREplaceWithPos() will clear the list buf */
    return DoBmkReplaceWithPos((UInt16)idx, name, b->pos);
}


Err DoBmkReplaceWithPos(UInt16 idx, char *name, UInt32 pos)
{
    MemHandle h;
    bmk_rec_t b;
    void* p;

    ErrFatalDisplayIf(Doc_getDbRef() == NULL,
        "DoBmkReplaceWithPos: document is not opened");

    BmkClearListBuf();

    h = DmGetRecord(Doc_getDbRef(), idx);
    if(!h)
        return DmGetLastErr();

    p = MemHandleLock(h);
    b.pos = pos;
    StrNCopy(b.name, name, 15);
    b.name[15] = '\0';
    DmWrite(p, 0, &b, sizeof(b));

    MemHandleUnlock(h);
    DmReleaseRecord(Doc_getDbRef(), idx, 0);

    return 0;
}

Err DoBmkReplace(UInt16 idx, char *name)
{
    return DoBmkReplaceWithPos(idx, name, Doc_getPosition());
}


/*
 * BmkDelete()
 * here I assume that selection comes from the list
 * without shortcuts
 */
Err BmkDelete(int sel)
{
    bmk_rec_t *b;
    int idx = -1;

    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    sel += scuts_num;

    b = (bmk_rec_t *)bmkListBuf[sel];
    idx = BmkFindByNameAndPos(b->name, b->pos);

    if(idx == -1)
        return DmGetLastErr();

    BmkClearListBuf();
    return DmRemoveRecord(Doc_getDbRef(), (UInt16)idx);
}


/*
 * BmkDeleteAll()
 * delete all bookmarks for the current document
 */
Err BmkDeleteAll(void)
{
    bmk_rec_t *b;
    UInt16 i, rn;
    Err err;

    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    BmkClearListBuf();

    rn = DmNumRecords(Doc_getDbRef());
    for(i = Doc_getNumRecs() + 1; i < rn; rn--) {
        err = DmRemoveRecord(Doc_getDbRef(), i);
        if(err)
            return err;
    }

    return 0;
}


/*
 * BmkMove()
 * here I assume that selection comes from the list
 * without shortcuts
 */
Err BmkMove(int action, int sel)
{
    bmk_rec_t *b;
    int idx = -1;

    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    sel += scuts_num;

    b = (bmk_rec_t *)bmkListBuf[sel];
    idx = BmkFindByNameAndPos(b->name, b->pos);

    if(idx == -1)
        return DmGetLastErr();

    BmkClearListBuf();

    switch(action) {
        case MOVE_UP:
            return DoBmkMoveUp((UInt16)idx);

        case MOVE_DOWN:
            return DoBmkMoveDown((UInt16)idx);

        default:
            ErrFatalDisplay("BmkMove: incorrect action");
            return 0;
    }

    return 0;
}


Err BmkSort(int action)
{
    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    BmkClearListBuf();

    switch(action) {
        case SORT_POS:
        case SORT_NAME:
            return BmkInsertionSort(action);

        default:
            ErrFatalDisplay("BmkSort: incorrect action");
            return 0;
    }

    return 0;
}


Err DoBmkMoveUp(UInt16 idx)
{
    int i;
    bmk_rec_t *b;
    Err err = 0;

    if(idx > Doc_getNumRecs() + 1)
        err = DmMoveRecord(Doc_getDbRef(), idx, idx - 1);

    return err;
}


Err DoBmkMoveDown(UInt16 idx)
{
    UInt16 i, rn;
    bmk_rec_t *b;
    Err err = 0;

    rn = DmNumRecords(Doc_getDbRef());
    if(idx < rn - 1)
        err = DmMoveRecord(Doc_getDbRef(), idx, idx + 2);

    return err;
}


/*
 * BmkRefillListBuf()
 * populates list area with pointers to bookmark names
 * the list buffer should be cleared before with
 * BmkClearListBuf, otherwise this function does
 * nothing
 */
Err BmkRefillListBuf(void)
{
    MemHandle h;
    UInt16 rn, i;
    bmk_rec_t *b;
    Err err = 0;

    ErrFatalDisplayIf(Doc_getDbRef() == NULL,
        "BmkRefillListBuf: document is not opened");

    if(bmkLBSize)
        return 0;

    rn = DmNumRecords(Doc_getDbRef());

    /* allocate memory for the list buffer */
    /* take into account shortcut pointers */
    if(bmkListBuf)
        MemPtrFree(bmkListBuf);

    h = MemHandleNew((rn - Doc_getNumRecs() - 1 + 2) * sizeof(char *));
    if(!h)
        return memErrNotEnoughSpace;
    bmkListBuf = MemHandleLock(h);

    /* add pointers to 'add' and 'edit' strings */
    if(Doc_getDbMode() == dmModeReadWrite) {
        *bmkListBuf     = bmkAddStr;
        bmkLBSize++;
    }

    *(bmkListBuf + bmkLBSize)   = bmkEdStr;
    bmkLBSize++;

    scuts_num = bmkLBSize;

    /* add pointers to bookmark names to the list buffer */
    for(i = Doc_getNumRecs() + 1; i < rn; i++) {
        b = (bmk_rec_t *)MemHandleLock(DmQueryRecord(Doc_getDbRef(), i));
        if(!b) {
            err = DmGetLastErr();
            goto ERR;
        }

        *(bmkListBuf + bmkLBSize) = b->name;
        bmkLBSize++;
    }

    return 0;

ERR:
    BmkClearListBuf();
    return err;
}


/*
 * BmkPopulateList()
 * initialize the given List
 * if 'shortcuts' is 1, menu shortcuts
 * will be inserted
 */
Err BmkPopulateList(ListPtr l, int shortcuts, int resize)
{
    Err err = 0;

    if(!Doc_getDbRef())
        return bmkErrDocNotOpened;

    err = BmkRefillListBuf();

    if(err)
        return err;

    if(shortcuts)
        LstSetListChoices(l, bmkListBuf, bmkLBSize);
    else
        LstSetListChoices(l, bmkListBuf + scuts_num,
                bmkLBSize - scuts_num);

    if(resize)
        LstSetHeight(l, shortcuts ? bmkLBSize : bmkLBSize - scuts_num);

    return 0;
}



/*
 * BmkClearListBuf()
 * walk through the list buf and unlock all pointers
 */
void BmkClearListBuf(void)
{
    int n = 0;
    int i;
    char *p;

    if(!bmkListBuf)
        return;

    n = MemPtrSize(bmkListBuf) / sizeof(char *);
    for(i = 0; i < n && i < bmkLBSize; i++) {
        p = bmkListBuf[i];
        if((p != bmkAddStr) && (p != bmkEdStr))
            MemPtrUnlock(p);
    }

    MemPtrFree(bmkListBuf);
    bmkListBuf = NULL;
    bmkLBSize = scuts_num = 0;
    return;
}



/*
 * BmkGetAction()
 * return specific action to perform if a special
 * item in the bookmarks list was chosen
 */
int BmkGetAction(int selection)
{
    ErrFatalDisplayIf(bmkListBuf == NULL,
        "BmkGetAction: list buffer doesn't exist");

    if(bmkListBuf[selection] == bmkAddStr)
        return A_NEW;
    if(bmkListBuf[selection] == bmkEdStr)
        return A_EDIT;
    return 0;
}


/*
 * BmkGoTo()
 * update document's position according to the selected
 * bookmark
 */
void BmkGoTo(int selection, int shortcuts)
{
    bmk_rec_t *b;

    ErrFatalDisplayIf(bmkListBuf == NULL,
        "BmkGoTo: bmk list buffer doesn't exist");

    if(!shortcuts)
        selection += scuts_num;

    b = (bmk_rec_t *)bmkListBuf[selection];
    Doc_setPosition(b->pos);
}


/*
 * BmkFindByName()
 * return the index of record
 * returns -1 in case of error or if not found
 */
int BmkFindByName(char *name)
{
    UInt16 rn, i;
    bmk_rec_t *b;

    ErrFatalDisplayIf(Doc_getDbRef() == NULL,
        "BmkFindByName: document is not opened");

    rn = DmNumRecords(Doc_getDbRef());
    for(i = Doc_getNumRecs() + 1; i < rn; i++) {
        b = (bmk_rec_t *)MemHandleLock(DmQueryRecord(Doc_getDbRef(), i));
        if(!b)
            return -1;

        if(StrCompare(b->name, name) == 0) {
            MemPtrUnlock(b);
            return i;
        }

        MemPtrUnlock(b);
    }

    return -1;
}


int BmkFindByNameAndPos(char *name, UInt32 pos)
{
    UInt16 rn, i;
    bmk_rec_t *b;

    ErrFatalDisplayIf(Doc_getDbRef() == NULL,
        "BmkFindByNameAndPos: document is not opened");

    rn = DmNumRecords(Doc_getDbRef());
    for(i = Doc_getNumRecs() + 1; i < rn; i++) {
        b = (bmk_rec_t *)MemHandleLock(DmQueryRecord(Doc_getDbRef(), i));
        if(!b)
            return -1;

        if(StrCompare(b->name, name) == 0 && b->pos == pos) {
            MemPtrUnlock(b);
            return i;
        }

        MemPtrUnlock(b);
    }

    return -1;
}


Err BmkInsertionSort(int action)
{
    UInt16 i, j, n, cmp;
    bmk_rec_t *b1, *b2;
    Err err = 0;

    ErrFatalDisplayIf(Doc_getDbRef() == NULL,
        "BmkInsertionSort: document is not opened");

    n = DmNumRecords(Doc_getDbRef());
    for(i = Doc_getNumRecs() + 2; i < n; i++) {
        b1 = (bmk_rec_t *)MemHandleLock(DmQueryRecord(Doc_getDbRef(), i));

        j = i;
        while(j > (Doc_getNumRecs() + 1)) {
            b2 = (bmk_rec_t *)MemHandleLock(
                    DmQueryRecord(Doc_getDbRef(), j - 1));

            if(action == SORT_NAME)
                cmp = StrCompare(b2->name, b1->name) > 0;
            else
                cmp = b2->pos > b1->pos;

            MemPtrUnlock(b2);

            if(!cmp)
                break;

            j--;
        }

        MemPtrUnlock(b1);

        if((err = DmMoveRecord(Doc_getDbRef(), i, j)))
            return err;
    }

    return 0;
}


void BmkReportError(Err e)
{
    MemHandle    errH;
    Int16 stringID = -1;
    char s[20] = "Error: 0x";
    char d[9];

    switch(e){
        case bmkErrDocNotOpened:
            stringID = stringID_bmkDocNotOpened;
            break;

        case memErrNotEnoughSpace:
            stringID = stringID_noMem;
            break;
    }

    if(stringID != -1) {
        errH = DmGetResource(strRsc, stringID);
        FrmCustomAlert(alertID_error,
                (Char*) MemHandleLock(errH), " ", " ");
        MemHandleUnlock(errH);
        DmReleaseResource(errH);
    } else {
        StrIToH(d, (UInt32)e);
        StrCat(s, d);
        FrmCustomAlert(alertID_error, s, " ", " ");
    }
}

#endif

