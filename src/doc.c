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
#include "myCharAttr.h"
#include <PalmOSGlue.h>
#include "appstate.h"
#include "rotate.h"
#include "doc.h"
#include "resources.h"
#include "mainform.h"
#include "decode.h"
#include "docprefs.h"
#include "tabbedtext.h"
#include "hyphenate.h"
#include "bmk.h"

#define min(a,b) ((a)<(b)?(a):(b))

#define WORD_MAX 0xFFFF /* USHRT_MAX is coming from a Linux header,
             * not a Palm one.*/

static void         _loadCurrentRecord();
static void         _setLineHeight();
static void         _scrollUpIfLastPage();

#ifdef SUPER0
static UInt32       _fixStoryLen(UInt16 *recLens, Boolean force);
#else
static UInt32       _fixStoryLen(UInt16 *recLens);
#endif

static void         _postDecodeProcessing();
static void         _drawPage(RectanglePtr boundsPtr,
                              Boolean drawOnscreenPart,
                              Boolean drawOffscreenPart);
static void         _setApparentTextBounds();
static Boolean      _findString(Char* haystack, Char* needle,
                                UInt16* foundPos, Boolean caseSensitive);
static void         _movePastWord();
static Boolean      _onLastPage();

/* RECORD0_STR describes the format of record 0 in a doc database.
 * Don't use fields labelled "munged", as aren't exactly what the
 * name would make you expect. */
struct RECORD0_STR
{
    UInt16 wVersion; /* 1=plain text, 2=compressed */
    UInt16 wSpare;  /* ? */
    UInt32 munged_dwStoryLen; /* in bytes, when decompressed.
                               * Appears to be wrong in some DOCs. */
    UInt16 munged_wNumRecs;  /* text records only;
                              * equals tDocHeader.wNumRecs-1.
                              * Use this, because bookmark records follow
                              * beyond these. Appears to be too big in some
                              * DOCs. */
    UInt16 wRecSize;         /* usually 0x1000 */
#ifdef SUPER0
    UInt32  dwCurrentPosition;
    UInt16  wRecordsSize[1];    // len of each decoded record
#else
    UInt32 dwSpare2;         /* ? */
#endif
};

typedef struct Doc {
    /* Record 0 of the document database */
    MemHandle           record0Handle;
    struct RECORD0_STR* record0Ptr;

    /* decodeBuf is the hunk of memory we decompress into */
    MemHandle           decodeBufHandle;
    Char*               decodeBuf;

    /* These describe the state of decodeBuf */
    UInt16 decodeBufLen;     /* Size of buffer allocated */
    UInt16 decodeLen;        /* Characters decoded */
    UInt16 recordDecoded;    /* The first record held in "decodeBuf" */
    UInt16 decodedRecordLen; /* Characters in record #recordDecoded
                              * which were decoded */

    UInt32     fixedStoryLen;
    MemHandle  recLensHandle;
    UInt16    *recLens;

    /* _dbRef and wNumRecs are referenced from the bookmarks subsystem */
    DmOpenRef dbRef;
    UInt16    numRecs; /* see record0's wNumRecs */
    UInt16    dbMode;
} Doc;

// Drawing table
#define MAX_DISPLAY_LINE 32
static Char*        _draw_buf[MAX_DISPLAY_LINE];            // Beginin of each line on screen
static UInt16       _draw_buf_len;          // number of lines in buffer
static Boolean      _bLastPage;             // Am I displaying the last page ?

Doc gDoc;

RectangleType       _textGadgetBounds;
RectangleType       _apparentTextBounds;
UInt16              _lineHeight = 0;

#ifdef ENABLE_AUTOSCROLL
static UInt16       _pixelOffset = 0;
static Boolean      _locationChanged = true;
static UInt16       _osExtraForAS; /* How many extra rows are added
                                    * to the osPageWindow */
#endif
static Boolean      _boundsChanged = true;
static WinHandle    osPageWindow = NULL;

struct DOC_PREFS_STR _docPrefs = DEFAULT_DOCPREFS;

UInt16 Doc_getDbMode() {return gDoc.dbMode;}
DmOpenRef Doc_getDbRef() {return gDoc.dbRef;}
UInt16 Doc_getNumRecs() {return gDoc.numRecs;}

void Doc_makeSettingsDefault()
{
    MemMove(&appStatePtr->defaultDocPrefs, &_docPrefs, sizeof(_docPrefs));
}

static WinHandle myWinCreateOffscreenWindow(Coord width, Coord height,
                                            UInt16 *error) 
{
    UInt32 version;
    Err err = 0;
    WindowFormatType format;

    err = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version);
    if (version >= 4)
        format = nativeFormat;
    else 
        format = screenFormat;

    return WinCreateOffscreenWindow(width, height,
                                    format, error);
}

void Doc_open(UInt16 cardNo, LocalID dbID, char name[dmDBNameLength])
{
    UInt16 dbAttrs = 0;

    ErrFatalDisplayIf(gDoc.dbRef != NULL, "g");

    MemSet(&gDoc, sizeof(gDoc), 0);

    // check if the db is read-only
    DmDatabaseInfo(cardNo, dbID, NULL, &dbAttrs, NULL, NULL,
                   NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    gDoc.dbMode = dmModeReadWrite;
    if(dbAttrs & dmHdrAttrReadOnly)
        gDoc.dbMode = dmModeReadOnly;

    //Open Db
    gDoc.dbRef = DmOpenDatabase(cardNo, dbID, gDoc.dbMode);
    ErrFatalDisplayIf(!gDoc.dbRef, "a");

    //lock record0
    gDoc.record0Handle = DmGetRecord(gDoc.dbRef, 0);
    ErrFatalDisplayIf(!gDoc.record0Handle, "b");

    gDoc.record0Ptr = (struct RECORD0_STR *) MemHandleLock(gDoc.record0Handle);
    ErrFatalDisplayIf(!gDoc.record0Ptr, "c"); 

    //allocate decode buffer
    gDoc.decodeBufLen
        = gDoc.record0Ptr->wRecSize + 640; // needs to hold a record+1 page+\0.
    gDoc.decodeBufHandle = MemHandleNew(gDoc.decodeBufLen);
    ErrFatalDisplayIf(!gDoc.decodeBufHandle, "d");
    gDoc.decodeBuf = MemHandleLock(gDoc.decodeBufHandle);
    ErrFatalDisplayIf(!gDoc.decodeBuf, "e");

    //I saw a document with wNumRecs was too big. Kill me now.
    gDoc.numRecs
        = min(gDoc.record0Ptr->munged_wNumRecs, DmNumRecords(gDoc.dbRef) - 1);

    //allocate record len array
    gDoc.recLensHandle = MemHandleNew(gDoc.numRecs*sizeof(*gDoc.recLens));
    ErrFatalDisplayIf(!gDoc.recLensHandle, "q");

    gDoc.recLens = MemHandleLock(gDoc.recLensHandle);
    ErrFatalDisplayIf(!gDoc.recLens, "r");

#ifdef SUPER0
    gDoc.fixedStoryLen = _fixStoryLen(gDoc.recLens, false);
#else
    gDoc.fixedStoryLen = _fixStoryLen(gDoc.recLens);
#endif
    //load prefs for this document
    DocPrefs_loadPrefs(name, &_docPrefs);

    //if location is bad, go to beginning.
    if (_docPrefs.location.record > gDoc.numRecs
        || _docPrefs.location.ch >= gDoc.recLens[_docPrefs.location.record-1])
    {
        _docPrefs.location.record = _docPrefs.location.ch = 0;
    }

    _setLineHeight(); //line height is dependent on the docPrefs.
    _setApparentTextBounds(); //rotation is from docPrefs.
    _loadCurrentRecord();
}

void Doc_close()
{
    //If a document is open...
    if (gDoc.dbRef != NULL)
    {
#ifdef ENABLE_BMK
        /* close bookmarks first */
        BmkCloseDoc();
#endif

        DocPrefs_savePrefs(&_docPrefs);

        //Free decode buffer
        MemHandleUnlock(gDoc.decodeBufHandle); //gDoc.decodeBuf = NULL;
        MemHandleFree(gDoc.decodeBufHandle); gDoc.decodeBufHandle = NULL;
        gDoc.decodeBufLen = 0;

        //unlock record0
        MemHandleUnlock(gDoc.record0Handle); gDoc.record0Ptr = NULL;
        gDoc.record0Handle = NULL;

        DmReleaseRecord(gDoc.dbRef, 0, false);

        MemHandleUnlock(gDoc.recLensHandle); gDoc.recLens = NULL;
        MemHandleFree(gDoc.recLensHandle); gDoc.recLensHandle = NULL;

        //Close Db
        DmCloseDatabase(gDoc.dbRef); gDoc.dbRef = NULL;
    }

    if (osPageWindow)
    {
        WinDeleteWindow(osPageWindow, false);
        osPageWindow = NULL;
    }
}

void Doc_drawPage()
{
    UInt16        errorUInt16;
    WinHandle   screenWindow = NULL;

    screenWindow = WinGetDrawWindow();

#ifdef ENABLE_AUTOSCROLL
    Doc_pixelScrollClear(false);
#endif
    
    if(_boundsChanged == true)
    {
        if(osPageWindow)
            WinDeleteWindow(osPageWindow, false);
#ifdef ENABLE_AUTOSCROLL
        osPageWindow = myWinCreateOffscreenWindow(_apparentTextBounds.extent.x,
                        _apparentTextBounds.extent.y + _osExtraForAS,
                                                &errorUInt16);
#else
        osPageWindow = myWinCreateOffscreenWindow(_apparentTextBounds.extent.x,
                                                _apparentTextBounds.extent.y,
                                                &errorUInt16);
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
        WinCopyRectangle(osPageWindow, screenWindow, 
                         &_apparentTextBounds,
                         _textGadgetBounds.topLeft.x,
                         _textGadgetBounds.topLeft.y,
                        winPaint);
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

//
// Scroll down n linesToMove
//  Quick=true : use table of displayed lines
//  Quick=false : browse the text line per line
//
Boolean Doc_linesDown(UInt16 linesToMove, Boolean quick)
{
    char*    p;
    FontID oldFont;

    if (quick) {
        if (_bLastPage) return false;
        p = _draw_buf[linesToMove];
    } else {
        if (_onLastPage())
            return false;

        oldFont = FntGetFont();
        FntSetFont(_docPrefs.font);

        p = & gDoc.decodeBuf[_docPrefs.location.ch];

        // advance p through linesToShow lines of worth of characters
        while(linesToMove--)
            p += (_docPrefs.justify && _docPrefs.hyphen) 
                ? Hyphen_FntWordWrap(p, _apparentTextBounds.extent.x) 
                : FntWordWrap (p, _apparentTextBounds.extent.x);

        //_scrollUpIfLastPage();

        FntSetFont(oldFont);
    }

    // Advance the current location by how far we advanced p
    _docPrefs.location.ch += (p - & gDoc.decodeBuf[_docPrefs.location.ch]);
    _loadCurrentRecord();
  
    return true;
}

//
// Scroll up n linesToMove
//
void Doc_linesUp(UInt16 linesToMove)
{
    FontID    oldFont;
    UInt16    firstCharIndex;
    UInt16    lastCharIndex;
    UInt16  linesRemain;
    
    oldFont = FntGetFont();
    FntSetFont(_docPrefs.font);

    lastCharIndex = _docPrefs.location.ch;

    while(true)
    {
        linesRemain = linesToMove;
        
        //Try to wrap back a whole page.
        firstCharIndex = lastCharIndex;
        if (firstCharIndex != 0)
            FntWordWrapReverseNLines(gDoc.decodeBuf,
                                     _apparentTextBounds.extent.x,
                                     &linesRemain, &firstCharIndex);

        linesToMove -= linesRemain;
        
        //If that only got back to the beginning of the decode buffer
        //and it wasn't the first record...
        if (firstCharIndex == 0 && _docPrefs.location.record > 1)
        {
            // Move current position to start of prev record and load
            _docPrefs.location.record--;
            _docPrefs.location.ch = 0;
            _loadCurrentRecord();

            //Adjust lastCharIndex to same char in the newly loaded buffer
            //and try to find the firstCharIndex again.
            lastCharIndex += gDoc.decodedRecordLen;
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

//
// Scroll the document
//
void Doc_scroll(int dir, enum TAP_ACTION_ENUM ta)
{
    UInt16 linesToScroll = 0;

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
        Doc_linesDown(linesToScroll, true);
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

void Doc_setLineHeightAdjust(UInt16 i)
{
    _docPrefs.lineHeightAdjust = i;
    _setLineHeight();
}

UInt16 Doc_getLineHeightAdjust()
{
    return _docPrefs.lineHeightAdjust;
}

//
// Change justification state of this Doc
//
void Doc_invertJustify()
{
    _docPrefs.justify = !_docPrefs.justify;
}

//
// Return justification state
//
UInt16 Doc_getJustify()
{
    return _docPrefs.justify;
}

//
// Change Hyphenation state of this Doc
//
void Doc_invertHyphen()
{
    _docPrefs.hyphen = !_docPrefs.hyphen;
}

UInt32 Doc_getPosition()
{
    UInt32    pos = 0;
    int i;

    //Add up previous records
    for (i=1; i<((int)_docPrefs.location.record); i++)
        pos+=gDoc.recLens[i-1];

    //Add in this partial record
    pos += (UInt32)_docPrefs.location.ch;

    return pos;
}

UInt16 Doc_getPercent()
{
    UInt32    pos = Doc_getPosition();
    return (100 * pos) / (gDoc.fixedStoryLen-1);
}

void Doc_setPosition(UInt32 pos)
{
    UInt32 prevRecsLen = 0;
    UInt16 i;

    //Add up record lens to find right record.
    for (i=1; prevRecsLen<=pos; i++)
        prevRecsLen += gDoc.recLens[i-1];

    _docPrefs.location.record = i-1;

    prevRecsLen -= gDoc.recLens[_docPrefs.location.record-1];
    _docPrefs.location.ch=pos-prevRecsLen;

    _loadCurrentRecord();

    // Insure that page doesn't start in mid-line.
    if (pos != 0)
    {
        Doc_linesUp(1);
        Doc_linesDown(1, false);
    }
    _scrollUpIfLastPage();

#ifdef ENABLE_AUTOSCROLL
    _locationChanged = true;
#endif
}

void Doc_setPercent(UInt16 per)
{
    UInt32 pos = (per * (gDoc.fixedStoryLen-1)) / 100;
    Doc_setPosition(pos);
}

Boolean Doc_inBottomHalf(int screenX, int screenY)
{
#ifdef ENABLE_ROTATION
    int y = screenY - _textGadgetBounds.topLeft.y;
    int x = screenX - _textGadgetBounds.topLeft.x;
    OrientationType a = (ORIENTATION_COUNT-_docPrefs.orient)%ORIENTATION_COUNT;
    int ymax = RotateY(_textGadgetBounds.extent.x,
                       _textGadgetBounds.extent.y, a);
    y = RotateY(x,y,a);

    if (ymax<0)
        y += _apparentTextBounds.extent.y;

    return y > _apparentTextBounds.extent.y/2;
#else
    return screenY > (_textGadgetBounds.topLeft.y 
                      + _textGadgetBounds.extent.y/2);
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

static void _drawPage(RectanglePtr boundsPtr,
                      Boolean drawOnscreenPart,
                      Boolean drawOffscreenPart)
{
    int       y;
    Boolean     leadingSpace;
    int       charsOnRow = 0;
    char*     p;
    int       linesToShow;
    WinHandle osLineWindow = NULL;
    UInt16      errorUInt16;
    RectangleType osLineBounds;
    FontID    oldFont = FntGetFont();
    WinHandle destWindow = WinGetDrawWindow();

    if (!gDoc.dbRef)
        return;

    FntSetFont(_docPrefs.font);

    //Create osLine window to render a line to
    osLineBounds.topLeft.x = osLineBounds.topLeft.y = 0;
    osLineBounds.extent.x = boundsPtr->extent.x;
    osLineBounds.extent.y = FntBaseLine()+FntDescenderHeight();
    osLineWindow = myWinCreateOffscreenWindow(osLineBounds.extent.x,
                                            osLineBounds.extent.y,
                                            &errorUInt16);
    ErrFatalDisplayIf(NULL == osLineWindow, "f");

    //Set drawing to go to the osLine window
    WinSetDrawWindow(osLineWindow);
#ifdef ENABLE_AUTOSCROLL
    {
        RectangleType biggerBounds = *boundsPtr;
        biggerBounds.extent.y += _osExtraForAS;
        WinEraseRectangle(&biggerBounds, 0);
    }
#else
    WinEraseRectangle(boundsPtr, 0);
#endif

    if(drawOffscreenPart)
#ifdef ENABLE_AUTOSCROLL
        linesToShow = (boundsPtr->extent.y + _osExtraForAS) / _lineHeight + 1;
#else    
        linesToShow = (boundsPtr->extent.y) / _lineHeight + 1;
#endif    
    else
        linesToShow = (boundsPtr->extent.y) / _lineHeight;


#ifdef ENABLE_AUTOSCROLL
    y = boundsPtr->topLeft.y - _pixelOffset;
#else
    y = boundsPtr->topLeft.y;
#endif
    
    p = & gDoc.decodeBuf[_docPrefs.location.ch];
    _draw_buf_len=0;

#ifdef ENABLE_AUTOSCROLL
    if(!drawOnscreenPart)
    {
        int offscreenLines = linesToShow - (boundsPtr->extent.y) / _lineHeight;
        offscreenLines += 2 ; //Because we want the last onscreen line too.

        // skip past onscreen lines
        _draw_buf_len = linesToShow - offscreenLines;
        if (_draw_buf_len > 0)
        {
            int i;
                
            // scroll the _draw_buff buffer
            for (i=0; i < MAX_DISPLAY_LINE-1; i++)
                _draw_buf[i] = _draw_buf[i + 1];
            
            y += _lineHeight * _draw_buf_len;
            p = _draw_buf[_draw_buf_len];
            linesToShow -= _draw_buf_len;

        }
    }
#endif

    // draw each line and memorize line in _draw_buf

    while(linesToShow)
    {
        leadingSpace = false;

        // if the line start with a space, replace it by an - to correctly compute
        // the len of the line
        //  -> FntWordWrap seems to affect only  a 1 pixel len to a leading space
        
        if (*p == ' ') {
            *p = '-';
            leadingSpace = true;
        }
            
        // find the end of the line.
        charsOnRow = (_docPrefs.justify && _docPrefs.hyphen) ?
                    Hyphen_FntWordWrap(p, boundsPtr->extent.x) :
                    FntWordWrap (p, boundsPtr->extent.x);

        if (leadingSpace) *p = ' ';
        
        if (!charsOnRow) break;

        WinEraseRectangle(&osLineBounds, 0);

        if (_docPrefs.justify)
            Justify_WinDrawChars(p, charsOnRow, 0, 0, boundsPtr->extent.x);
        else
            Tab_WinDrawChars(p, charsOnRow, 0, 0);

        _draw_buf[_draw_buf_len++] = p; // memorize line position
        ErrFatalDisplayIf ((_draw_buf_len>MAX_DISPLAY_LINE), "Too many lines");

        //Copy the osLine in in an ORing kind of way.
        //This preserves lowercase extenders of previous line.

        WinCopyRectangle(osLineWindow, destWindow, &osLineBounds,
                         boundsPtr->topLeft.x, y, winOverlay);

        y += _lineHeight;
        p += charsOnRow;
        linesToShow--;
    }

    // Am I at the end of the doc ?
    _bLastPage = (*p == 0);

    // Memorise last line
    _draw_buf[_draw_buf_len++] = p; // status of last line + 1

    //    WinDrawLine(boundsPtr->topLeft.x, boundsPtr->topLeft.y+boundsPtr->extent.y-1,
    //                boundsPtr->topLeft.x+boundsPtr->extent.x,
    //                boundsPtr->topLeft.y+boundsPtr->extent.y-1);

    WinSetDrawWindow(destWindow);
    WinDeleteWindow(osLineWindow, false);
    osLineWindow = NULL;
    FntSetFont(oldFont);
}

static Boolean _onLastPage()
{
    char*    p;
    int        linesToShow;
    FontID oldFont;

    oldFont = FntGetFont();
    FntSetFont(_docPrefs.font);

    //See if the terminating null is on this page
    linesToShow = _apparentTextBounds.extent.y/_lineHeight;
    p = & gDoc.decodeBuf[_docPrefs.location.ch];
    while(linesToShow--)
        p += (_docPrefs.justify && _docPrefs.hyphen)
            ? Hyphen_FntWordWrap(p, _apparentTextBounds.extent.x)
            : FntWordWrap (p, _apparentTextBounds.extent.x);

    FntSetFont(oldFont);

    _bLastPage = (*p == 0);
    return _bLastPage;
}

static void _scrollUpIfLastPage()
{
    if (_docPrefs.location.ch == 0 && _docPrefs.location.record == 0)
        return;

    //if it was, go to end and page up.
    if (_onLastPage())
    {
        char* p = & gDoc.decodeBuf[_docPrefs.location.ch];
        while (*p)
            p++;
        _docPrefs.location.ch += (p - & gDoc.decodeBuf[_docPrefs.location.ch]);
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
        UInt16 t = _apparentTextBounds.extent.x;
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

// If you change _docPrefs.location, call this
static void _loadCurrentRecord()
{
    //If current record not gDoc.decodeBuf, decode it.
    if (gDoc.recordDecoded != _docPrefs.location.record)
    {
        UInt16 recToLoad = _docPrefs.location.record;

        gDoc.decodeLen = gDoc.decodedRecordLen
            = decodeRecord(gDoc.dbRef, gDoc.record0Ptr->wVersion,
                           gDoc.decodeBuf,
                           recToLoad, gDoc.decodeBufLen-1);

#ifdef SUPER0
        // Compare this decoded len to the one stored in the RECORD0
        // If we find a bug, recompute the table to fix it directly in the RECORD0
        if (gDoc.decodeLen != gDoc.recLens[recToLoad - 1]) {
            // Warning the last record is recorded with the \0 added in memory !!
            if (recToLoad != gDoc.numRecs) {
                gDoc.fixedStoryLen = _fixStoryLen(gDoc.recLens, true);
            } else if (gDoc.decodeLen != (gDoc.recLens[recToLoad - 1]-1))
                gDoc.fixedStoryLen = _fixStoryLen(gDoc.recLens, true);
        }
#endif
        
        gDoc.recordDecoded = _docPrefs.location.record;

        /* If this is the last record, make the null appear to belong
         * to this record */
        if (recToLoad == gDoc.numRecs)
        {
            gDoc.decodedRecordLen++;
        }
        //Otherwise load more records
        else
        {
            /* While there are remaining records, and space to fill
             * in buffer, decode next records. */
            while (++recToLoad <= gDoc.numRecs
                   && gDoc.decodeLen < gDoc.decodeBufLen-1)
                gDoc.decodeLen
                    += decodeRecord(gDoc.dbRef, gDoc.record0Ptr->wVersion,
                                    &gDoc.decodeBuf[gDoc.decodeLen],
                                    recToLoad,
                                    (gDoc.decodeBufLen-1) - gDoc.decodeLen);
        }
        gDoc.decodeBuf[gDoc.decodeLen] = '\0';
        _postDecodeProcessing();

        //if location is not in this record, do it over
        if (_docPrefs.location.ch >= gDoc.decodedRecordLen)
            _loadCurrentRecord();
    }
    //If current location is in the next record, normalize
    else if (_docPrefs.location.ch >= gDoc.decodedRecordLen)
    {
        ErrFatalDisplayIf(_docPrefs.location.record+1>gDoc.numRecs, "sheez");
        _docPrefs.location.ch -= gDoc.decodedRecordLen;
        _docPrefs.location.record++;
        _loadCurrentRecord();
    }
}

//
// Compute the total space of the dos
//  Fill the recLens table with the len of each record
//  Extract this from the record0 if available
//  force =true to force rebuild of the table
#ifdef SUPER0
static UInt32 _fixStoryLen(UInt16 *recLens, Boolean force)
{
    UInt32 storyLen = 0;
    UInt16 i, *a, *b;
    // look if the wRecordsSize if filled and complete
    if ((!force)
        && (MemPtrSize (gDoc.record0Ptr) >= sizeof(struct RECORD0_STR))
        && (gDoc.record0Ptr->wRecordsSize[0])
        )
    {
        a = recLens;
        b = &gDoc.record0Ptr->wRecordsSize[0];
        for (i = 0; i < gDoc.numRecs; i++) {
            *(a++) = *b;
            storyLen += *(b++);
        }
    }
    else {        
        for (i = 0; i < gDoc.numRecs; i++)
        {
            recLens[i]
                = decodedRecordLen(gDoc.dbRef, gDoc.record0Ptr->wVersion, i+1);
            storyLen += recLens[i];
        }

        // Resize record0 to hold the table if the file is read/write
        
        if (gDoc.dbMode == dmModeReadWrite) {
            MemHandle h;
            
            MemHandleUnlock(gDoc.record0Handle);
            
            h = DmResizeRecord(gDoc.dbRef, 0,
                               (UInt32)(sizeof (struct RECORD0_STR)
                                        + gDoc.numRecs*sizeof(UInt16)));
            
            if (h != NULL) { 
                gDoc.record0Handle = h;
            }

            gDoc.record0Ptr
                = (struct RECORD0_STR *)MemHandleLock(gDoc.record0Handle);

            if (h != NULL) {
                DmWrite (gDoc.record0Ptr,
                         sizeof (struct RECORD0_STR) - sizeof(UInt16),
                         recLens,
                         sizeof(UInt16)*gDoc.numRecs);
            }

            ErrFatalDisplayIf(!gDoc.record0Ptr, "c");
        }
    }
    recLens[gDoc.numRecs - 1]++;//for the null at the end of the last record.
    
    return ++storyLen;
}
#else
static UInt32 _fixStoryLen(UInt16 *recLens)
{
    UInt32 storyLen = 0;
    UInt16 i;

    for (i = 0; i < gDoc.numRecs; i++)
    {
        recLens[i] = decodedRecordLen(gDoc.dbRef,
                                      gDoc.record0Ptr->wVersion, i+1);
        storyLen += recLens[i];
    }
    recLens[gDoc.numRecs-1]++;//for the null at the end of the last record.
    storyLen++;

    return storyLen;
}
#endif

static void _postDecodeProcessing()
{
    Char* tooFar = &gDoc.decodeBuf[gDoc.decodeLen];
    Char* p = gDoc.decodeBuf;

    do
    {
        if (*p == '\r')
            *p = '\n';
    } while (++p < tooFar);
}



#ifdef ENABLE_SEARCH
void Doc_doSearch(MemHandle searchStringHandle, Boolean searchFromTop,
                  Boolean caseSensitive, UInt16 formId)
{
    struct DOC_LOCATION_STR posBeforeSearch;
    struct DOC_LOCATION_STR startOfSearch;
    Char* searchString = MemHandleLock(searchStringHandle);
    Boolean found = false;
    UInt16 stopAfterRecord = WORD_MAX;
    Boolean giveUp = false;

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
        int fnord = FntWordWrap(&gDoc.decodeBuf[_docPrefs.location.ch],
                                _apparentTextBounds.extent.x);
        _docPrefs.location.ch += fnord;
    }

    while(!found && !giveUp)
    {
        UInt16 foundPos;

        _loadCurrentRecord();

        //If we found it, move to after find.
        if (_findString(&gDoc.decodeBuf[_docPrefs.location.ch], searchString,
                        &foundPos, caseSensitive))
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
                if (_docPrefs.location.record > gDoc.numRecs)
                    _docPrefs.location.record = 1;
            }
        }
    }

    MemHandleUnlock(searchStringHandle);

    if (found && formId)
    {
        _movePastWord();
        _loadCurrentRecord();
        Doc_linesUp(1);

        FrmUpdateForm(formId, 0);
    }
    else
    {
        MemMove(&_docPrefs.location,
                &posBeforeSearch, sizeof(posBeforeSearch));
        _loadCurrentRecord();
        _locationChanged = true;
    }

}

//_findString works a lot like FindStrInStr. Except correctly.
//And optionally case-sensitively.
static Boolean _findString(Char* haystack, Char* needle,
                           UInt16* foundPos,
                           Boolean caseSensitive)
{
    UInt16    needleLen = StrLen(needle);
    UInt16    haystackLen = StrLen(haystack);
    Boolean rv;
    Char* foundPtr;

    if (! caseSensitive)
    {
        //FindStrInStr will only find the search string if is at
        //the beginning of a word. So, it will
        //find "foo" in "foobar" but not in "barfoo".
        //Really should just write our own FindStrInStr...

        const UInt8* convTable = GetCharCaselessValue();
        Char* clNeedle = (Char*) MemHandleLock(MemHandleNew(needleLen+1));
        Char* clHaystack = (Char*) MemPtrNew(haystackLen+1); //yuck.
        Boolean toReturn;
        UInt16    i;

        for(i = 0; i < needleLen; i++)
            clNeedle[i] = convTable[(int) needle[i]];
        clNeedle[i]='\0';

        for(i = 0; i < haystackLen; i++)
            clHaystack[i] = convTable[(int)haystack[i]];
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

static void _movePastWord()
{
    while (! TxtGlueCharIsSpace((UInt8)gDoc.decodeBuf[_docPrefs.location.ch])
            && '\0' != gDoc.decodeBuf[_docPrefs.location.ch])
    {
        _docPrefs.location.ch++;
    }
}
#endif

#ifdef ENABLE_AUTOSCROLL
void Doc_prepareForPixelScrolling()
{
    RectangleType fromRect;
    WinHandle w;

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

#ifdef ENABLE_ROTATION    
    if (_docPrefs.orient == angle0)
    {
        WinCopyRectangle(osPageWindow, w, &fromRect,
                     _textGadgetBounds.topLeft.x + fromRect.topLeft.x,
                     _textGadgetBounds.topLeft.y + fromRect.topLeft.y,
                     winPaint);
    } else {
        RotCopyWindow(osPageWindow,
                    _apparentTextBounds.extent.y-1-fromRect.extent.y,
                    _apparentTextBounds.extent.y-1,
                    _docPrefs.orient);
    }
#else
        WinCopyRectangle(osPageWindow, w, &fromRect,
                     _textGadgetBounds.topLeft.x + fromRect.topLeft.x,
                     _textGadgetBounds.topLeft.y + fromRect.topLeft.y,
                     winPaint);
#endif
}

void Doc_pixelScroll()
{
    RectangleType rect;
    WinHandle     drawWindow;
    RectangleType vacated;
    Boolean       endOfDocument;

    drawWindow = WinGetDrawWindow();

#ifdef ENABLE_ROTATION
    if (_docPrefs.orient == angle0)
        WinScrollRectangle(&_textGadgetBounds, winUp, 1, &vacated);
    else
        RotScrollRectangleUp(&_textGadgetBounds, _docPrefs.orient);
#else
        WinScrollRectangle(&_textGadgetBounds, winUp, 1, &vacated);
#endif
    
    WinSetDrawWindow(osPageWindow);

    // if scrolled enough to draw a line, scroll down 1 and draw the page.
    if(_pixelOffset == _lineHeight)
    {
        UInt16 i = _docPrefs.location.record;
        
        if (! Doc_linesDown(1, true))
        {
            WinSetDrawWindow(drawWindow);
            return;
        }

        WinEraseWindow();
        _pixelOffset = 0;

        // If I changed of record I need to redraw the full screen to rebuild the _draw_buf cache
        if (i != _docPrefs.location.record)
            _drawPage(&_apparentTextBounds, true, true);
        else
            _drawPage(&_apparentTextBounds, false, true);
    }


    // increment pixel offset
    _pixelOffset++;

    // Shift the spare row at the bottom of the offscreen image up.
    // todo: We don't need to shift it all, since the visible screen
    // is scrolled below.
    rect = _apparentTextBounds;
    rect.extent.y += _osExtraForAS;
    WinScrollRectangle(&rect, winUp, 1, &vacated);

    WinSetDrawWindow(drawWindow);

#ifdef ENABLE_ROTATION
    if (_docPrefs.orient == angle0)
    {
        RectangleType fromRect;
        //Now fill in the gap at the bottom
        fromRect.extent.x = _apparentTextBounds.extent.x;
        fromRect.extent.y = _lineHeight;
        fromRect.topLeft.x = 0;
        fromRect.topLeft.y = _apparentTextBounds.extent.y-fromRect.extent.y;
        WinCopyRectangle(osPageWindow, drawWindow, &fromRect,
                         _textGadgetBounds.topLeft.x + fromRect.topLeft.x,
                         _textGadgetBounds.topLeft.y + fromRect.topLeft.y,
                         winPaint);
    }
    else
    {
        // RotCopyWindow might copy a few rows before the row you tell it to,
        // since it needs to start on a pyte boundary. I think.
        RotCopyWindow(osPageWindow,
                        _apparentTextBounds.extent.y-1-_lineHeight,
                        _apparentTextBounds.extent.y-1,
                        _docPrefs.orient);
    }
#else
    {
        RectangleType fromRect;
        //Now fill in the gap at the bottom
        fromRect.extent.x = _apparentTextBounds.extent.x;
        fromRect.extent.y = _lineHeight;
        fromRect.topLeft.x = 0;
        fromRect.topLeft.y = _apparentTextBounds.extent.y-fromRect.extent.y;
        WinCopyRectangle(osPageWindow, drawWindow, &fromRect,
                         _textGadgetBounds.topLeft.x + fromRect.topLeft.x,
                         _textGadgetBounds.topLeft.y + fromRect.topLeft.y,
                         winPaint);
    }
#endif    
}

void Doc_pixelScrollClear(Boolean forceReset)
{
    // if location has changed since last pixel scroll, reset scroll offset
    if(_locationChanged || forceReset)
    {
        _pixelOffset = 0;
        _locationChanged = false;
    }
}
#endif
