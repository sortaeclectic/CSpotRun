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
#include "app.h"
#include "doclist.h"
#include "rotate.h"
#include "appstate.h" //for tap_action_enum
#include "doc.h" //really just creator and type ids

//#define MAX_DOCS            20

struct DB_INFO_STR
{
    char    name[dmDBNameLength]; //includes null, according to DataMgr.h
    UInt16  cardNo;
};

struct DB_INFO_STR    *_dbInfoArray = NULL;
int                    _dbCount = 0;
UInt16*                _map = NULL;
static char**        _listItemPtrs = NULL;

static void _buildMap();

void DocList_freeList()
{
    if (_dbInfoArray)
    {
        MemPtrFree(_dbInfoArray); _dbInfoArray = NULL;
        MemPtrFree(_listItemPtrs); _listItemPtrs = NULL;
        MemPtrFree(_map); _map = NULL;
    }
}

void DocList_populateList(ListPtr listPtr)
{
    Boolean                newSearch = true;
    DmSearchStateType    searchState;
    UInt16            cardNo;
    LocalID            dbID;
    char            name[dmDBNameLength];
    int                i;

    // Count documents
    _dbCount = 0;
    while (0 == DmGetNextDatabaseByTypeCreator(newSearch, &searchState,
                                               DOC_TYPE, DOC_CREATOR,
                                               false, &cardNo, &dbID))
    {
        _dbCount++;
        newSearch = false;
    }

    if (_dbCount)
    {
        // Allocate arrays to hold info on all docs
        _dbInfoArray =
            MemHandleLock(MemHandleNew(_dbCount * sizeof(*_dbInfoArray)));
        _listItemPtrs =
            MemHandleLock(MemHandleNew(_dbCount * sizeof(*_listItemPtrs)));
        _map =
            MemHandleLock(MemHandleNew(_dbCount * sizeof(*_map)));
    }

    // populate those arrays.
    newSearch = true;
    _dbCount = 0;
    while (0 == DmGetNextDatabaseByTypeCreator(newSearch, &searchState,
                                               DOC_TYPE, DOC_CREATOR,
                                               false, &cardNo, &dbID))
    {
        UInt16 recordCount = 0;
        DmOpenRef db;
        newSearch = false;

        DmDatabaseInfo(cardNo, dbID, name,
                       NULL, NULL, NULL, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL);

        db = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
        if (db) {
            recordCount = DmNumRecords(db);
            DmCloseDatabase(db);
        }
        if (recordCount < 2)
            continue;

        MemMove(_dbInfoArray[_dbCount].name, name, dmDBNameLength);
        _dbInfoArray[_dbCount].cardNo = cardNo;
        _dbCount++;
    }

    _buildMap();

    // set up gui list
    for (i = 0; i < _dbCount; i++)
    {
        _listItemPtrs[i] = _dbInfoArray[_map[i]].name;
    }
    LstSetListChoices(listPtr, _listItemPtrs, _dbCount);
    LstSetHeight(listPtr, _dbCount);
}

Char* DocList_getTitle(int i)
{
    return _dbInfoArray[_map[i]].name;
}
LocalID DocList_getID(int i)
{
    return DmFindDatabase(_dbInfoArray[_map[i]].cardNo,
                          _dbInfoArray[_map[i]].name);
}
UInt16 DocList_getCardNo(int i)
{
    return _dbInfoArray[_map[i]].cardNo;
}

Int16 DocList_getIndex(char name[dmDBNameLength])
{
    int i;
    for (i = 0; i < _dbCount; i++)
        if (0 == StrCompare(name, _dbInfoArray[_map[i]].name))
            return i;
    return -1;
}

UInt16    DocList_getDocCount()
{
    return _dbCount;
}


static void _buildMap()
{
    int i,j;
    UInt16 tmp;

    for (i = 0; i < _dbCount; i++)
        _map[i]=i;

    for (i = 0; i < _dbCount-1; i++)
    {
        for(j = i+1; j < _dbCount; j++)
        {
            if (0 < StrCaselessCompare(_dbInfoArray[_map[i]].name,
                                       _dbInfoArray[_map[j]].name))
            {
                tmp = _map[i];
                _map[i] = _map[j];
                _map[j] = tmp;
            }
        }
    }
}
