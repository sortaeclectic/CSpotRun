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

// The format of record 0 was taken from
// http://www.concentric.net/~rbram/makedoc7.cpp
// Is this documented elsewhere?


#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>
#include <UI/CharAttr.h>
#include "appstate.h"
#include "rotate.h"
#include "doc.h"
#include "resources.h"
#include "mainform.h"
#include "decode.h"
#include "docprefs.h"
#include "tabbedtext.h"
#include <limits.h>

#define WORD_MAX USHRT_MAX
////////////////////////////////////////////////////////////////////////////////
// private functions
////////////////////////////////////////////////////////////////////////////////
static void         _loadCurrentRecord();
static void         _setLineHeight();
static void         _scrollUpIfLastPage();
static DWord        _fixStoryLen(Word *recLens);
static void         _postDecodeProcessing();
static void         _drawPage(RectanglePtr boundsPtr,
                              Boolean drawOnscreenPart, Boolean drawOffscreenPart);
static void         _setApparentTextBounds();
static Boolean      _findString(CharPtr haystack, CharPtr needle, WordPtr foundPos, Boolean caseSensitive);
static void         _rewindToStartOfWord();
static void         _movePastWord();
static Boolean      _onLastPage();

////////////////////////////////////////////////////////////////////////////////
// private data
////////////////////////////////////////////////////////////////////////////////
struct RECORD0_STR
{
    Byte    crap;    //Because wVersion was 1026 (1024+2) in one doc when it should have been 2.
    Byte    wVersion;                // 1=plain text, 2=compressed
    Word    wSpare;                    //??
    DWord    munged_dwStoryLen;        // in bytes, when decompressed.  Appears to be wrong in some DOCs.
    Word    wNumRecs;                // text records only; equals tDocHeader.wNumRecs-1. Use this, because AportisDoc adds records beyond these.
    Word    wRecSize;                // usually 0x1000
    DWord    dwSpare2;                //??
};

//This is crap.
//Should have a document structure to pass to the document functions, object style.

VoidHand            _record0Handle = NULL;
struct RECORD0_STR* _record0Ptr = NULL;
DmOpenRef           _dbRef = NULL;
VoidHand            _decodeBufHandle = NULL;

CharPtr             _decodeBuf = NULL;
// These describe the state of _decodeBuf
UShort              _decodeBufLen = 0;            //Size of buffer allocated
UShort              _decodeLen = 0;               //Characters decoded
UShort              _recordDecoded = 0;           //The first record held in "_decodeBuf"
UShort              _decodedRecordLen = 0;        //Characters in record #_recordDecoded which were decoded

DWord               _fixedStoryLen;

RectangleType       _textGadgetBounds;
RectangleType       _apparentTextBounds;
UShort              _lineHeight = 0;

#ifdef ENABLE_AUTOSCROLL
static UShort       _pixelOffset = 0;
static Boolean      _locationChanged = true;
static UShort       _osExtraForAS; //How many extra rows are added to the osPageWindow
#endif
static Boolean      _boundsChanged = true;
static WinHandle    osPageWindow = NULL;

struct DOC_PREFS_STR _docPrefs = DEFAULT_DOCPREFS;

VoidHand        _recLensHandle    = NULL;
Word            *_recLens        = NULL;

void Doc_makeSettingsDefault()
{
    MemMove(&appStatePtr->defaultDocPrefs, &_docPrefs, sizeof(_docPrefs));
}


////////////////////////////////////////////////////////////////////////////////
// Doc_open
////////////////////////////////////////////////////////////////////////////////
void Doc_open(UInt cardNo, LocalID dbID, char name[dmDBNameLength])
{
    ErrFatalDisplayIf(_dbRef != NULL, "g");

    //Open Db
    _dbRef = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
    ErrFatalDisplayIf(!_dbRef, "a");

    //lock record0
    _record0Handle = DmQueryRecord(_dbRef, 0);
    ErrFatalDisplayIf(!_record0Handle, "b");

    _record0Ptr = (struct RECORD0_STR *) MemHandleLock(_record0Handle);
    ErrFatalDisplayIf(!_record0Ptr, "c");

    //allocate decode buffer
    _decodeBufLen = _record0Ptr->wRecSize + 640; // needs to hold a record + 1 page + \0.
    _decodeBufHandle = MemHandleNew(_decodeBufLen);
    ErrFatalDisplayIf(!_decodeBufHandle, "d");
    _decodeBuf = MemHandleLock(_decodeBufHandle);
    ErrFatalDisplayIf(!_decodeBuf, "e");

    //initialize decode buffer to be empty
    _decodeLen = 0;
    _decodedRecordLen = 0;
    _recordDecoded = 0;

    //allocate record len array
    _recLensHandle =    MemHandleNew(_record0Ptr->wNumRecs*sizeof(*_recLens));
    ErrFatalDisplayIf(!_recLensHandle, "q");

    _recLens =            MemHandleLock(_recLensHandle);
    ErrFatalDisplayIf(!_recLens, "r");

    _fixedStoryLen = _fixStoryLen(_recLens);

    //load prefs for this document
    DocPrefs_loadPrefs(name, &_docPrefs);

    //if location is bad, go to beginning.
    if (_docPrefs.location.record > _record0Ptr->wNumRecs || _docPrefs.location.ch >= _recLens[_docPrefs.location.record-1])
    {
        _docPrefs.location.record = _docPrefs.location.ch = 0;
    }

    _setLineHeight(); //line height is dependent on the docPrefs.
    _setApparentTextBounds(); //rotation is from docPrefs.
    _loadCurrentRecord();

}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void Doc_close()
{
    //If a document is open...
    if (_dbRef != NULL)
    {
        DocPrefs_savePrefs(&_docPrefs);

        //Free decode buffer
        MemHandleUnlock(_decodeBufHandle); //_decodeBuf = NULL;
        MemHandleFree(_decodeBufHandle); _decodeBufHandle = NULL;
        _decodeBufLen = 0;

        //unlock record0
        MemHandleUnlock(_record0Handle); _record0Ptr = NULL;
        _record0Handle = NULL;


        MemHandleUnlock(_recLensHandle); _recLens = NULL;
        MemHandleFree(_recLensHandle); _recLensHandle = NULL;

        //Close Db
        DmCloseDatabase(_dbRef); _dbRef = NULL;
    }
}

void Doc_drawPage()
{
    Word        errorWord;
    WinHandle   screenWindow = NULL;

    screenWindow = WinGetDrawWindow();

    if(_boundsChanged == true)
    {
        if(osPageWindow)
            WinDeleteWindow(osPageWindow, false);
#ifdef ENABLE_AUTOSCROLL
        osPageWindow = WinCreateOffscreenWindow(_apparentTextBounds.extent.x,
                        _apparentTextBounds.extent.y + _osExtraForAS,
                        screenFormat,
                        &errorWord);
#else
        osPageWindow = WinCreateOffscreenWindow(_apparentTextBounds.extent.x,
                        _apparentTextBounds.extent.y,
                        screenFormat,
                        &errorWord);
#endif
        ErrFatalDisplayIf(NULL == osPageWindow, "f1");

        _boundsChanged = false;
    }

    // Actual draw process, happens to osPageWindow
    WinSetDrawWindow(osPageWindow);
    WinEraseWindow();

    _drawPage(&_apparentTextBounds, true, false);

    WinSetDrawWindow(screenWindow);

    if (_docPrefs.orient == angle0)
    {
#ifdef ENABLE_AUTOSCROLL
        WinCopyRectangle(osPageWindow, screenWindow, &_apparentTextBounds,
                        _textGadgetBounds.topLeft.x, _textGadgetBounds.topLeft.y,
                        scrCopy);
#else
        WinRestoreBits(osPageWindow,
                        _textGadgetBounds.topLeft.x,
                        _textGadgetBounds.topLeft.y);
#endif
    }
#ifdef ENABLE_ROTATION
    else
    {
        RotCopyWindow(osPageWindow,
                        0,
                        _apparentTextBounds.extent.y-1,
                        _docPrefs.orient);

    }
#endif
}

void Doc_linesDown(Word linesToMove)
{
    char*    p;
    FontID oldFont;

    if (_onLastPage())
        return;

    oldFont = FntGetFont();
    FntSetFont(_docPrefs.font);

    p = & _decodeBuf[_docPrefs.location.ch];

    // advance p through linesToShow lines of worth of characters
    while(linesToMove--)
        p += FntWordWrap(p, _apparentTextBounds.extent.x);

    // Advance the current location by how far we advanced p
    _docPrefs.location.ch += (p - & _decodeBuf[_docPrefs.location.ch]);
    _loadCurrentRecord();

    //_scrollUpIfLastPage();

    FntSetFont(oldFont);
}

void Doc_linesUp(Word linesToMove)
{
    FontID    oldFont;
    Word    firstCharIndex;
    Word    lastCharIndex;

    oldFont = FntGetFont();
    FntSetFont(_docPrefs.font);

    lastCharIndex = _docPrefs.location.ch;

    while(true)
    {
        //Try to wrap back a whole page.
        firstCharIndex = lastCharIndex;
        if (firstCharIndex != 0)
            FntWordWrapReverseNLines(_decodeBuf, _apparentTextBounds.extent.x, &linesToMove, &firstCharIndex);

        //If that only got back to the beginning of the decode buffer and it wasn't the first record...
        if (firstCharIndex == 0 && _docPrefs.location.record > 1)
        {
            // Move current position to start of prev record and load
            _docPrefs.location.record--;
            _docPrefs.location.ch = 0;
            _loadCurrentRecord();

            //Adjust lastCharIndex to same char in the newly loaded buffer and try to find the firstCharIndex again.
            lastCharIndex += _decodedRecordLen;
        }
        //else we found the top of the page.
        else
        {
            _docPrefs.location.ch = firstCharIndex;
            FntSetFont(oldFont);
            return;
        }
    }
}

void Doc_scroll(int dir, enum TAP_ACTION_ENUM ta)
{
    UShort linesToScroll = 0;

    switch (ta)
    {
        case TA_PAGE:
            linesToScroll = _apparentTextBounds.extent.y / _lineHeight;
            if (appStatePtr->showPreviousLine)
                linesToScroll--;
            break;
        case TA_HALFPAGE:
            linesToScroll = (_apparentTextBounds.extent.y / _lineHeight) / 2;
            break;
        case TA_LINE:
            linesToScroll = 1;
            break;
    }

    if (dir == PAGEDIR_DOWN)
        Doc_linesDown(linesToScroll);
    else
        Doc_linesUp(linesToScroll);

#ifdef ENABLE_AUTOSCROLL
    _locationChanged = true;
#endif
}


void Doc_setBounds(RectanglePtr bounds)
{
    MemMove(&_textGadgetBounds, bounds, sizeof(*bounds));

    _setApparentTextBounds();
}

void Doc_setOrientation(OrientationType or)
{
    _docPrefs.orient = or;
    _setApparentTextBounds();
}

OrientationType Doc_getOrientation()
{
    return _docPrefs.orient;
}

RectanglePtr Doc_getGadgetBounds()
{
    return &_textGadgetBounds;
}

void Doc_setFont(FontID f)
{
    _docPrefs.font = f;
    _setLineHeight();
}

FontID Doc_getFont()
{
    return _docPrefs.font;
}

void Doc_setLineHeightAdjust(UShort i)
{
    _docPrefs.lineHeightAdjust = i;
    _setLineHeight();
}

UShort Doc_getLineHeightAdjust()
{
    return _docPrefs.lineHeightAdjust;
}

UInt Doc_getPercent()
{
    DWord    pos = 0;
    int i;

    //Add up previous records
    for (i=1; i<((int)_docPrefs.location.record); i++)
        pos+=_recLens[i-1];

    //Add in this partial record
    pos += (DWord)_docPrefs.location.ch;

    return (100 * pos) / (_fixedStoryLen-1);
}

void Doc_setPercent(UInt per)
{
    DWord pos = (per * (_fixedStoryLen-1)) / 100;
    DWord prevRecsLen = 0;
    Word i;

    //Add up record lens to find right record.
    for (i=1; prevRecsLen<=pos; i++)
        prevRecsLen += _recLens[i-1];

    _docPrefs.location.record = i-1;

    prevRecsLen -= _recLens[_docPrefs.location.record-1];
    _docPrefs.location.ch=pos-prevRecsLen;

    _loadCurrentRecord();

    // Insure that page doesn't start in mid-line.
    if (per != 0)
    {
        Doc_linesUp(1);
        Doc_linesDown(1);
    }
    _scrollUpIfLastPage();

#ifdef ENABLE_AUTOSCROLL
    _locationChanged = true;
#endif
}

Boolean Doc_inBottomHalf(int screenX, int screenY)
{
#ifdef ENABLE_ROTATION
    int y = screenY - _textGadgetBounds.topLeft.y;
    int x = screenX - _textGadgetBounds.topLeft.x;
    OrientationType a = (ORIENTATION_COUNT-_docPrefs.orient)%ORIENTATION_COUNT;
    int ymax = RotateY(_textGadgetBounds.extent.x, _textGadgetBounds.extent.y, a);
    y = RotateY(x,y,a);

    if (ymax<0)
        y += _apparentTextBounds.extent.y;

    return y > _apparentTextBounds.extent.y/2;
#else
    return screenY > (_textGadgetBounds.topLeft.y + _textGadgetBounds.extent.y/2);
#endif
}

int Doc_translatePageButton(int dir)
{
    if (_docPrefs.orient == angle270 || _docPrefs.orient == angle180)
        dir *= -1;

#ifdef ENABLE_ROTATION
    if (isSideways(_docPrefs.orient) && appStatePtr->reversePageUpDown)
        dir *= -1;
#endif
    return dir;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
static void _drawPage(RectanglePtr boundsPtr,
                        Boolean drawOnscreenPart, Boolean drawOffscreenPart)
{
    int        y = 0;
    int        charsOnRow = 0;
    int        offsetFromLocation = 0;
    char    *p;
    int        linesToShow;
    WinHandle osLineWindow = NULL;
    Word    errorWord;
    RectangleType osLineBounds;
    FontID    oldFont = FntGetFont();
    WinHandle destWindow = WinGetDrawWindow();

    if (!_dbRef)
        return;

    FntSetFont(_docPrefs.font);

    //Create osLine window to render a line to
    osLineBounds.topLeft.x = osLineBounds.topLeft.y = 0;
    osLineBounds.extent.x = boundsPtr->extent.x;
    osLineBounds.extent.y = FntBaseLine()+FntDescenderHeight();
    osLineWindow = WinCreateOffscreenWindow(osLineBounds.extent.x, osLineBounds.extent.y, genericFormat, &errorWord);
    ErrFatalDisplayIf(NULL == osLineWindow, "f");

    //Set drawing to go to the osLine window
    WinSetDrawWindow(osLineWindow);
    WinEraseRectangle(boundsPtr, 0);

    if(drawOffscreenPart)
        linesToShow = (boundsPtr->extent.y + _osExtraForAS) / _lineHeight + 1;
    else
        linesToShow = (boundsPtr->extent.y) / _lineHeight;

    y = boundsPtr->topLeft.y;
    p = & _decodeBuf[_docPrefs.location.ch];

#ifdef ENABLE_AUTOSCROLL
    if(!drawOnscreenPart)
    {
		int offscreenLines = linesToShow - (boundsPtr->extent.y) / _lineHeight;
        offscreenLines ++; //Because we really want the last onscreen line too.
        // skip past all but the last line
        while((linesToShow > offscreenLines) && (charsOnRow = FntWordWrap(p, boundsPtr->extent.x)))
        {
            y += _lineHeight;
            p += charsOnRow;
            linesToShow--;
        }
    }
#endif
    while(linesToShow && (charsOnRow = FntWordWrap(p, boundsPtr->extent.x)))
    {
        WinEraseRectangle(&osLineBounds, 0);
        TT_WinDrawChars(p, charsOnRow, 0, 0);
        //Copy the osLine in in an ORing kind of way.  This preserves lowercase extenders of previous line.
        WinCopyRectangle(osLineWindow, destWindow, &osLineBounds, boundsPtr->topLeft.x, y, scrOR);
        y += _lineHeight;
        p += charsOnRow;
        linesToShow--;
    }
//    WinDrawLine(boundsPtr->topLeft.x, boundsPtr->topLeft.y+boundsPtr->extent.y-1,
//                    boundsPtr->topLeft.x+boundsPtr->extent.x, boundsPtr->topLeft.y+boundsPtr->extent.y-1);

    WinSetDrawWindow(destWindow);
    WinDeleteWindow(osLineWindow, false);
    osLineWindow = NULL;
    FntSetFont(oldFont);
}

//#include "resources.h"
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static Boolean _onLastPage()
{
    char*    p;
    int        linesToShow;
    FontID oldFont;

    oldFont = FntGetFont();
    FntSetFont(_docPrefs.font);

    //See if the terminating null is on this page
    linesToShow = _apparentTextBounds.extent.y/_lineHeight;
    p = & _decodeBuf[_docPrefs.location.ch];
    while(linesToShow--)
        p += FntWordWrap(p, _apparentTextBounds.extent.x);

    FntSetFont(oldFont);

    return (*p == NULL);
}

static void _scrollUpIfLastPage()
{
    if (_docPrefs.location.ch == 0 && _docPrefs.location.record == 0)
        return;

    //if it was, go to end and page up.
    if (_onLastPage())
    {
        char* p = & _decodeBuf[_docPrefs.location.ch];
        while (*p)
            p++;
        _docPrefs.location.ch += (p - & _decodeBuf[_docPrefs.location.ch]);
        _loadCurrentRecord();
        Doc_scroll(PAGEDIR_UP, TA_PAGE);
    }
}

static void _setApparentTextBounds()
{
    _apparentTextBounds.topLeft.x = _apparentTextBounds.topLeft.y = 0;
    _apparentTextBounds.extent.x = _textGadgetBounds.extent.x;
    _apparentTextBounds.extent.y = _textGadgetBounds.extent.y;
#ifdef ENABLE_ROTATION
    if (isSideways(_docPrefs.orient))
    {
        UShort t = _apparentTextBounds.extent.x;
        _apparentTextBounds.extent.x = _apparentTextBounds.extent.y;
        _apparentTextBounds.extent.y = t;
    }
#endif

    _boundsChanged = true;
#ifdef ENABLE_AUTOSCROLL
    _locationChanged = true;
#endif
}

static void _setLineHeight()
{
    FontID oldFont;

    oldFont = FntGetFont();
    FntSetFont(_docPrefs.font);
    _lineHeight = FntLineHeight() - _docPrefs.lineHeightAdjust;
    FntSetFont(oldFont);

    _boundsChanged = true;
#ifdef ENABLE_AUTOSCROLL
    _osExtraForAS = _lineHeight;
    _locationChanged = true;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// If you change _docPrefs.location, call this
////////////////////////////////////////////////////////////////////////////////
static void _loadCurrentRecord()
{
    //If current record not _decodeBuf, decode it.
    if (_recordDecoded != _docPrefs.location.record)
    {
        UInt recToLoad = _docPrefs.location.record;

        _decodeLen = _decodedRecordLen = decodeRecord(_dbRef, _record0Ptr->wVersion, _decodeBuf, recToLoad, _decodeBufLen-1);
        _recordDecoded = _docPrefs.location.record;
        //If this is the last record, make the null appear to belong to this record
        if (recToLoad == _record0Ptr->wNumRecs)
        {
            _decodedRecordLen++;
        }
        //Otherwise load more records
        else
        {
            //While there are remaining records, and space to fill in the buffer, decode next records.
            while (++recToLoad <= _record0Ptr->wNumRecs && _decodeLen < _decodeBufLen-1)
                _decodeLen += decodeRecord(_dbRef, _record0Ptr->wVersion, &_decodeBuf[_decodeLen], recToLoad, (_decodeBufLen-1) - _decodeLen);
        }
        _decodeBuf[_decodeLen] = '\0';
        _postDecodeProcessing();

        //if location is not in this record, do it over
        if (_docPrefs.location.ch >= _decodedRecordLen)
            _loadCurrentRecord();
    }
    //If current location is in the next record, normalize
    else if (_docPrefs.location.ch >= _decodedRecordLen)
    {
ErrFatalDisplayIf(_docPrefs.location.record+1>_record0Ptr->wNumRecs, "sheez");
        _docPrefs.location.ch -= _decodedRecordLen;
        _docPrefs.location.record++;
        _loadCurrentRecord();
    }
}

static DWord _fixStoryLen(Word *recLens)
{
    DWord storyLen = 0;
    Word i;

    for (i = 0; i < _record0Ptr->wNumRecs; i++)
    {
/*        _docPrefs.location.ch = 0;
        _docPrefs.location.record = i+1;
        _loadCurrentRecord();
        storyLen += _decodedRecordLen;
        recLens[i] = _decodedRecordLen;
*/
        storyLen += (recLens[i] = decodedRecordLen(_dbRef, _record0Ptr->wVersion, i+1));
    }
    recLens[_record0Ptr->wNumRecs-1]++;//for the null at the end of the last record.
    storyLen++;

    return storyLen;
}

static void _postDecodeProcessing()
{
    CharPtr tooFar = &_decodeBuf[_decodeLen];
    CharPtr p = _decodeBuf;

    do
    {
        if (*p == '\r')
            *p = '\n';
    } while (++p < tooFar);

}



#ifdef ENABLE_SEARCH
void Doc_doSearch(VoidHand searchStringHandle, Boolean searchFromTop, Boolean caseSensitive, Word formId)
{
    struct DOC_LOCATION_STR posBeforeSearch;
    struct DOC_LOCATION_STR startOfSearch;
    CharPtr        searchString = MemHandleLock(searchStringHandle);
    Boolean        found = false;
    Word        stopAfterRecord = WORD_MAX;
    Boolean        giveUp = false;

    MemMove(&posBeforeSearch, &_docPrefs.location, sizeof(posBeforeSearch));

    //Either start from top or current location.
    if (searchFromTop)
    {
        startOfSearch.record = 1;
        startOfSearch.ch = 0;
        MemMove(&_docPrefs.location, &startOfSearch, sizeof(posBeforeSearch));
    }
    else
    {
        //Doc_linesDown(1);//So we don't find it again if we are sitting on it.

        int fnord = FntWordWrap(&_decodeBuf[_docPrefs.location.ch], _apparentTextBounds.extent.x);
        _docPrefs.location.ch += fnord;
    }

    while(!found && !giveUp)
    {
        Word foundPos;

        _loadCurrentRecord();

        //If we found it, move to after find.
        if (_findString(&_decodeBuf[_docPrefs.location.ch], searchString, &foundPos, caseSensitive))
        {
            _docPrefs.location.ch += foundPos;
            found = true;
        }
        //If not, move to next record.
        else
        {
            if (_docPrefs.location.record == stopAfterRecord)
                giveUp = true;
            else
            {
                if (stopAfterRecord == WORD_MAX)
                    stopAfterRecord = _docPrefs.location.record;

                _docPrefs.location.ch = 0;
                _docPrefs.location.record++;
                if (_docPrefs.location.record > _record0Ptr->wNumRecs)
                    _docPrefs.location.record = 1;
            }
        }
    }

    MemHandleUnlock(searchStringHandle);

    if (found && formId)
    {
//        _loadCurrentRecord();
//        _rewindToStartOfWord();
        _movePastWord();_loadCurrentRecord();Doc_linesUp(1);
//        _loadCurrentRecord();Doc_linesDown(1);Doc_linesUp(1);

        FrmUpdateForm(formId, 0);
    }
    else
    {
        MemMove(&_docPrefs.location, &posBeforeSearch, sizeof(posBeforeSearch));
        _loadCurrentRecord();
        _locationChanged = true;
    }

}

//_findString works a lot like FindStrInStr. Except correctly. And optionally case-sensitively.
static Boolean _findString(CharPtr haystack, CharPtr needle, WordPtr foundPos, Boolean caseSensitive)
{
    UInt    needleLen = StrLen(needle);
    UInt    haystackLen = StrLen(haystack);
    Boolean rv;
    CharPtr foundPtr;

    if (! caseSensitive)
    {
        //FindStrInStr will only find the search string if is at the beginning of a word. So, it will
        //find "foo" in "foobar" but not in "barfoo".
        //Really should just write our own FindStrInStr...

        BytePtr convTable = GetCharCaselessValue();
        CharPtr clNeedle = (CharPtr) MemHandleLock(MemHandleNew(needleLen+1));
        CharPtr clHaystack = (CharPtr) MemPtrNew(haystackLen+1); //yuck.
        Boolean toReturn;
        UInt    i;

        for(i = 0; i < needleLen; i++)
            clNeedle[i] = convTable[needle[i]];
        clNeedle[i]='\0';

        for(i = 0; i < haystackLen; i++)
            clHaystack[i] = convTable[haystack[i]];
        clHaystack[i]='\0';

        haystack = clHaystack;
        needle = clNeedle;
    }

    foundPtr = StrStr(haystack, needle);
    if (foundPtr)
    {
        *foundPos = foundPtr - haystack;
        rv = true;
    }
    else
    {
        rv = false;
    }

    if (! caseSensitive)
    {
        MemPtrFree((void*)needle);
        MemPtrFree((void*)haystack);
    }

    return rv;
}

static void _rewindToStartOfWord()
{
    WordPtr attrs = GetCharAttr();

    while (_docPrefs.location.ch>0 && ! IsSpace(attrs, _decodeBuf[_docPrefs.location.ch-1]))
    {
        _docPrefs.location.ch--;
    }
}

static void _movePastWord()
{
    WordPtr attrs = GetCharAttr();

    while (! IsSpace(attrs, _decodeBuf[_docPrefs.location.ch])
            && '\0' != _decodeBuf[_docPrefs.location.ch])
    {
        _docPrefs.location.ch++;
    }
/*
    while (IsSpace(attrs, _decodeBuf[_docPrefs.location.ch]))
    {
        _docPrefs.location.ch++;
    }
*/
}
#endif

#ifdef ENABLE_AUTOSCROLL
void Doc_prepareForPixelScrolling()
{
    RectangleType fromRect;
    WinHandle w;

    if (_pixelOffset)
        return;

    //Draw the partial line at the bottom of the screen.

    w = WinGetDrawWindow();
    WinSetDrawWindow(osPageWindow);
    WinEraseWindow();

    _drawPage(&_apparentTextBounds, true, true);

    WinSetDrawWindow(w);

    fromRect.extent.x = _apparentTextBounds.extent.x;
    fromRect.extent.y = _lineHeight;
    fromRect.topLeft.x = 0;
    fromRect.topLeft.y = _apparentTextBounds.extent.y-fromRect.extent.y;
    if (_docPrefs.orient == angle0)
    {
        WinCopyRectangle(osPageWindow, w, &fromRect,
                     _textGadgetBounds.topLeft.x + fromRect.topLeft.x,
                     _textGadgetBounds.topLeft.y + fromRect.topLeft.y,
                     scrCopy);
    } else {
        RotCopyWindow(osPageWindow,
                    _apparentTextBounds.extent.y-1-fromRect.extent.y,
                    _apparentTextBounds.extent.y-1,
                    _docPrefs.orient);
    }
}

void Doc_pixelScroll()
{
    RectangleType rect;
    WinHandle     drawWindow;
    RectangleType vacated;

    drawWindow = WinGetDrawWindow();
    WinSetDrawWindow(osPageWindow);

    // if scrolled enough to draw a line, scroll down 1 and draw the page.
    if(_pixelOffset == _lineHeight)
    {
        Doc_linesDown(1);
        WinEraseWindow();
        _drawPage(&_apparentTextBounds, true, true);
        _pixelOffset = 0;
    }

    // increment pixel offset
    _pixelOffset++;

    // Shift the spare row at the bottom of the offscreen image up.
    // todo: We don't need to shift it all, since the visible screen is scrolled below.
    rect = _apparentTextBounds;
    rect.extent.y += _osExtraForAS;
    WinScrollRectangle(&rect, up, 1, &vacated);

    WinSetDrawWindow(drawWindow);

    if (_docPrefs.orient == angle0)
    {
        RectangleType fromRect;
        WinScrollRectangle(&_textGadgetBounds, up, 1, &vacated);
        //Now fill in the gap at the bottom
        fromRect.extent.x = _apparentTextBounds.extent.x;
        fromRect.extent.y = _lineHeight;
        fromRect.topLeft.x = 0;
        fromRect.topLeft.y = _apparentTextBounds.extent.y-fromRect.extent.y;
        WinCopyRectangle(osPageWindow, drawWindow, &fromRect,
                         _textGadgetBounds.topLeft.x + fromRect.topLeft.x,
                         _textGadgetBounds.topLeft.y + fromRect.topLeft.y,
                         scrCopy);
    }
    else
    {
        RotScrollRectangleUp(&_textGadgetBounds, _docPrefs.orient);
        // RotCopyWindow might copy a few rows before the row you tell it to,
        // since it needs to start on a pyte boundary. I think.
        RotCopyWindow(osPageWindow,
                        _apparentTextBounds.extent.y-1-_lineHeight,
                        _apparentTextBounds.extent.y-1,
                        _docPrefs.orient);
    }
}

void Doc_pixelScrollClear()
{
    // if location has changed since last pixel scroll, reset scroll offset
    if(_locationChanged)
    {
        _pixelOffset = 0;
        _locationChanged = false;
    }
}
#endif
