/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
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
#include "doclist.h"
#include "rotate.h"
#include "docprefs.h"
#include "appstate.h"

#define PERDOC_PREFS_DB_TYPE    'DCPF'

static char _startupDocName[dmDBNameLength] = "";

////////////////////////////////////////////////////////////////////////////////
//

void DocPrefs_setStartupDocName(char startupDocName[dmDBNameLength])
{
    StrNCopy(_startupDocName, startupDocName, dmDBNameLength);
}

void DocPrefs_savePrefs(struct DOC_PREFS_STR * prefs)
{
    DmOpenRef                prefDbRef = NULL;
    struct DOC_PREFS_STR    *p = NULL;
    UInt16                    recNo;
    Boolean                    recFound = false;

    //Open Db
    prefDbRef = DmOpenDatabaseByTypeCreator(PERDOC_PREFS_DB_TYPE, appId, dmModeReadWrite);
    if (!prefDbRef)
    {
        Err err = DmCreateDatabase(0, "CSpotRunPrefs", appId, PERDOC_PREFS_DB_TYPE, false);
        ErrFatalDisplayIf(err, "k");
        DocPrefs_savePrefs(prefs);
        return;
    }
    // Check each record for matching title.
    recNo = 0;
    while (recNo < DmNumRecords(prefDbRef))
    {
        p = (struct DOC_PREFS_STR *) MemHandleLock(DmQueryRecord(prefDbRef, recNo));
        recFound = (0 == StrCompare(prefs->name, p->name));
        MemPtrUnlock(p); p = NULL;

        if (recFound)
            break;
        else
            recNo++;
    }

    // If record found, lock it.
    if (recFound)
    {
        p = MemHandleLock(DmGetRecord(prefDbRef, recNo));
        ErrFatalDisplayIf(!p, "L");
    }
    //Else no record found, create a new one.
    else
    {
        recNo = 0;
        p = MemHandleLock(DmNewRecord(prefDbRef, &recNo, sizeof(*p)));
        ErrFatalDisplayIf(!p, "M");
    }

    // Write preferences to the record
    DmWrite(p, 0, prefs, sizeof(*p));
    MemPtrUnlock(p); p = NULL;
    DmReleaseRecord(prefDbRef, recNo, true);

    //Move the saved record to the first.
    DmMoveRecord(prefDbRef, recNo, 0);

    DmCloseDatabase(prefDbRef);
}

////////////////////////////////////////////////////////////////////////////////
// if this fails, it will leave prefsPtr alone.

void DocPrefs_loadPrefs(char name[dmDBNameLength], struct DOC_PREFS_STR * prefsPtr)
{
    DmOpenRef            prefDbRef = NULL;
    UInt16                i;
    Boolean                recLoaded = false;
    struct DOC_PREFS_STR *p = NULL;

    //Open Db
    prefDbRef = DmOpenDatabaseByTypeCreator(PERDOC_PREFS_DB_TYPE, appId, dmModeReadWrite);
    if (prefDbRef)
    {
        //Adjust to defaults in case load fails.
        MemMove(prefsPtr, &appStatePtr->defaultDocPrefs, sizeof(*prefsPtr));
        prefsPtr->location.record = 1;
        prefsPtr->location.ch = 0;
        StrNCopy(prefsPtr->name, name, dmDBNameLength);

        //scan through doc pref records looking for matching name,
        for(i = 0; i < DmNumRecords(prefDbRef) && !recLoaded; i++)
        {
            p = (struct DOC_PREFS_STR *) MemHandleLock(DmGetRecord(prefDbRef, i));
            if (0 == StrCompare(name, p->name))
            {
                MemMove(prefsPtr, p, sizeof(*p));
                recLoaded = true;
            }
            MemPtrUnlock(p);
            DmReleaseRecord(prefDbRef, i, false);
        }
    }

    if (prefDbRef)
        DmCloseDatabase(prefDbRef);

}

////////////////////////////////////////////////////////////////////////////////

void DocPrefs_getRecentDocName(char name[dmDBNameLength])
{
    DmOpenRef            prefDbRef = NULL;

    name[0]='\0';

    // If a docname is already specified, use that
    if (_startupDocName[0] != '\0') {
        StrCopy(name, _startupDocName);
        _startupDocName[0]='\0';
        return;
    }

    prefDbRef = DmOpenDatabaseByTypeCreator(PERDOC_PREFS_DB_TYPE, appId, dmModeReadWrite);
    if (prefDbRef)
    {
        if(DmNumRecords(prefDbRef) > 0)
        {
            struct DOC_PREFS_STR *p = NULL;

            p = (struct DOC_PREFS_STR *) MemHandleLock(DmGetRecord(prefDbRef, 0));
            StrCopy(name, p->name);
            MemPtrUnlock(p);
            DmReleaseRecord(prefDbRef, 0, false);
        }
        DmCloseDatabase(prefDbRef);
    }
}

////////////////////////////////////////////////////////////////////////////////

void DocPrefs_initPrefs(struct DOC_PREFS_STR *p, char name[dmDBNameLength])
{
    p->font = stdFont;
    p->lineHeightAdjust = 1;
    p->location.record = 1;
    p->location.ch = 0;
    p->orient = angle0;
    StrNCopy(&(p->name[0]), name, dmDBNameLength);
}


void DocPrefs_cleanUpPrefs()
{
    DmOpenRef                prefDbRef = NULL;
    struct DOC_PREFS_STR    *p = NULL;
    UInt16                   recNo;
    Boolean                  deleteThis;

    //Open Db
    prefDbRef = DmOpenDatabaseByTypeCreator(PERDOC_PREFS_DB_TYPE, appId, dmModeReadWrite);
    if (!prefDbRef)
        return;

    // For each pref record

    recNo = 0;
    while(recNo < DmNumRecords(prefDbRef))
    {
        //If the record has no corresponding doc, delete it.
        p = (struct DOC_PREFS_STR *) MemHandleLock(DmQueryRecord(prefDbRef, recNo));
        deleteThis = 0 > DocList_getIndex(p->name);
        MemPtrUnlock(p); p = NULL;
        if (deleteThis)
            DmRemoveRecord(prefDbRef, recNo);
        else
            recNo++;
    }
    DmCloseDatabase(prefDbRef);
}
