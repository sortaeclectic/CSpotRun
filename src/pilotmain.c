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

#pragma pack(2)

#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>
#include <UI/ScrDriverNew.h>
#include <KeyMgr.h>
#include <SysEvtMgr.h>
#include "app.h"
#include "resources.h"
#include "mainform.h"
#include "controlsform.h"
#include "appstate.h"
#include "ucgui.h"
#include "prefsform.h"
#include "docprefs.h"
#ifdef ENABLE_SEARCH
#include "searchform.h"
#endif

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags);
static void StartApp();
static void EventLoop();
static void StopApp();
static Boolean AppHandleEvent(EventType *e);
static void InitAppState();

struct APP_STATE_STR *appStatePtr;
#ifdef ENABLE_SEARCH
VoidHand    searchStringHandle;
Boolean     searchFromTop;
#endif

#ifndef SysAppLaunchCmdOpenDBType
typedef struct {
    Word cardNo;
    LocalID dbID;
} SysAppLaunchCmdOpenDBType;

#define sysAppLaunchCmdOpenDB 52
#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdOpenDB) {
        Word      cardNo = ((SysAppLaunchCmdOpenDBType*)cmdPBP)->cardNo;
        LocalID   dbID = ((SysAppLaunchCmdOpenDBType*)cmdPBP)->dbID;
        ULong     type, creator;

        // get the type/creator for this DB so we can see if it's a DOC file
        char startupDocName[dmDBNameLength];
        DmDatabaseInfo(cardNo, dbID, startupDocName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &type, &creator);
        if (!(type == 'TEXt' && creator == 'REAd')) {
                FrmCustomAlert(alertID_error, "You cannot open this type of database with CSpotRun. ", " ", " ");
                return 1;
        }
        DocPrefs_setStartupDocName(startupDocName);
        cmd = sysAppLaunchCmdNormalLaunch;
    }

    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        StartApp();
        EventLoop();
        StopApp();
    }

    return 0;
}

Boolean UtilOSIsAtLeast(Byte reqMajor, Byte reqMinor)
{
    DWord romVersion;
    Byte major, minor;

    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    major = sysGetROMVerMajor(romVersion);
    minor = sysGetROMVerMinor(romVersion);

    if (major > reqMajor)
        return true;
    if (major == reqMajor && minor >= reqMinor)
        return true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
static void StartApp()
{
    Word prefsSize = 0;
#ifdef ENABLE_SEARCH
    CharPtr searchPtr = NULL;
#endif
    DWord newDepth = 1;

    appStatePtr = (struct APP_STATE_STR *) MemPtrNew(sizeof(*appStatePtr));
    if (UtilOSIsAtLeast(3,0))
    {
        DWord depth;
        ScrDisplayMode(scrDisplayModeGetSupportedDepths, NULL, NULL, &depth, NULL);
        if (depth & 0x1)
            ScrDisplayMode(scrDisplayModeSet, NULL, NULL, &newDepth, NULL);
        else
            ErrFatalDisplay("No 1-bit mode!");
    }

    if (noPreferenceFound == PrefGetAppPreferences(appId, PREF_APPSTATE, NULL, &prefsSize, true)
        || prefsSize != sizeof(*appStatePtr))
        InitAppState();
    else
    {
        PrefGetAppPreferences(appId, PREF_APPSTATE, appStatePtr, &prefsSize, true);
        if (appStatePtr->version != versionWord)
            InitAppState();
    }
    prefsSize = 0;

#ifdef ENABLE_SEARCH
    if (noPreferenceFound == PrefGetAppPreferences(appId, PREF_SEARCHSTRING, NULL, &prefsSize, true))
    {
        searchStringHandle = MemHandleNew(2);
        searchPtr = MemHandleLock(searchStringHandle);
        StrCopy(searchPtr, "");
        MemHandleUnlock(searchStringHandle);
    }
    else
    {
        searchStringHandle = MemHandleNew(prefsSize);
        searchPtr = (CharPtr)MemHandleLock(searchStringHandle);
        PrefGetAppPreferences(appId, PREF_SEARCHSTRING, searchPtr, &prefsSize, true);
        MemHandleUnlock(searchStringHandle);
    }
#endif

    FrmGotoForm(formID_main);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void InitAppState()
{
    MemSet(appStatePtr, sizeof(*appStatePtr), 0);

    appStatePtr->version            = versionWord;
    appStatePtr->UCGUIBits          = Ucgui_getDefaultWord();
    appStatePtr->hideControls       = 0;
    appStatePtr->reversePageUpDown  = 0;
    appStatePtr->showPreviousLine   = 1;
#ifdef ENABLE_AUTOSCROLL
    appStatePtr->autoScrollSpeed0   = 20;
    appStatePtr->autoScrollSpeed1   = 60;
    appStatePtr->autoScrollButton   = 1;
    appStatePtr->autoScrollType     = ATYPE_PIXEL;
#endif
    appStatePtr->tapAction          = TA_PAGE;

    DocPrefs_initPrefs(&appStatePtr->defaultDocPrefs, "");
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void StopApp()
{
#ifdef ENABLE_SEARCH
    CharPtr searchPtr = NULL;
#endif
    FrmCloseAllForms();

    PrefSetAppPreferences(appId, PREF_APPSTATE, versionWord, appStatePtr, sizeof(*appStatePtr), true);
    MemPtrFree(appStatePtr);

#ifdef ENABLE_SEARCH
    searchPtr = (CharPtr) MemHandleLock(searchStringHandle);
    PrefSetAppPreferences(appId, PREF_SEARCHSTRING, versionWord, searchPtr, MemHandleSize(searchStringHandle), true);
    MemHandleUnlock(searchStringHandle);
    MemHandleFree(searchStringHandle);
#endif
    if (UtilOSIsAtLeast(3,0))
        ScrDisplayMode(scrDisplayModeSetToDefaults, NULL, NULL, NULL, NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void EventLoop()
{
    EventType e;
    FormType *pfrm;
    Word err;
#ifdef ENABLE_AUTOSCROLL
    Boolean autoScrollStopped = false;
    Long lastScrollTime = 0;
#endif

    do
    {
#ifdef ENABLE_AUTOSCROLL
        if(MainForm_AutoScrollEnabled())
        {
            Long now;
            Long interval;      //Desired time between scrolls.
            Long elapsed;       //Time since previous start of drawing
            Long timeToWait;    //Time to wait before nilEvent

            //todo: Time between frames is autoScrollSpeed+timeittakestodraw
            //Should subtract the time since last scroll from the timeout here
            //so that it will scroll at the same speed on future superfast Palm
            //devices.
            if(appStatePtr->autoScrollType == ATYPE_PIXEL)
                interval = appStatePtr->autoScrollSpeed0;
            else
                interval = appStatePtr->autoScrollSpeed1;

            now = TimGetTicks();
            if (now > lastScrollTime)
                elapsed = now - lastScrollTime;
            else
                elapsed = 0;
            if (elapsed < interval)
                timeToWait = interval - elapsed;
            else
                timeToWait = 0;

            //For diagnosing stutters...
            //if (timeToWait == 0)
            //    SndPlaySystemSound(sndClick);

            EvtGetEvent(&e, timeToWait);
        }
        else
            EvtGetEvent(&e, evtWaitForever);

        // use Hard button 2 to toggle AutoScroll
        if((e.eType == keyDownEvent)&&(e.data.keyDown.chr == hard2Chr)
           &&(!(e.data.keyDown.modifiers & poweredOnKeyMask))
           &&(appStatePtr->autoScrollButton == 1))
        {
            MainForm_ToggleAutoScroll();
            continue;
        }

        if(MainForm_AutoScrollEnabled())
        {
            if(e.eType == nilEvent)
            {
                lastScrollTime = TimGetTicks();
                MainForm_UpdateAutoScroll();
            }
            //todo: move most of this event handling to mainform.c?
            else if(e.eType == penDownEvent)
            {
                autoScrollStopped = true;
            }
            else if(e.eType == keyDownEvent)
            {
                // use Hard button 2 to toggle AutoScroll
                if((e.data.keyDown.chr == pageDownChr))
                {
                    // lower autoscroll speed
                    if(appStatePtr->autoScrollType == ATYPE_PIXEL)
                    {
                        appStatePtr->autoScrollSpeed0 += 5;
                        if(appStatePtr ->autoScrollSpeed0 > 70)
                            appStatePtr->autoScrollSpeed0 = 70;
                    }
                    else
                    {
                        appStatePtr->autoScrollSpeed1 += 10;
                        if(appStatePtr ->autoScrollSpeed1 > 150)
                            appStatePtr->autoScrollSpeed1 = 150;
                    }

                    continue;
                }
                else if((e.data.keyDown.chr == pageUpChr))
                {
                    // raise autoscroll speed.
                    if(appStatePtr->autoScrollType == ATYPE_PIXEL)
                    {
                        appStatePtr->autoScrollSpeed0 -= 5;
                        if(appStatePtr ->autoScrollSpeed0 < 5)
                            appStatePtr->autoScrollSpeed0 = 5;
                    }
                    else
                    {
                        appStatePtr->autoScrollSpeed1 -= 20;
                        if(appStatePtr ->autoScrollSpeed1 < 20)
                            appStatePtr->autoScrollSpeed1 = 20;
                    }

                    continue;
                }
            }
        }
#else
        EvtGetEvent(&e, evtWaitForever);
#endif

        if (!SysHandleEvent(&e))
            if (!MenuHandleEvent(MenuGetActiveMenu(), &e, &err))
                if (!AppHandleEvent(&e))
                    FrmDispatchEvent(&e);

#ifdef ENABLE_AUTOSCROLL
        if(autoScrollStopped)
        {
            //Check again here, we might be toggling it back on.
            if (MainForm_AutoScrollEnabled())
                MainForm_ToggleAutoScroll();
            autoScrollStopped = false;
        }
#endif
    } while(e.eType != appStopEvent);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static Boolean AppHandleEvent(EventType *e)
{
    switch (e->eType)
    {
        case frmLoadEvent:
        {
            Word formId;
            FormPtr frm;
            FormEventHandlerPtr newFormEventHandler = NULL;

            formId = e->data.frmLoad.formID;
            switch(formId)
            {
                case formID_main:
                    newFormEventHandler = MainFormHandleEvent;
                    break;
                case formID_controls:
                    newFormEventHandler = ControlsFormHandleEvent;
                    break;
                case formID_globalPrefs:
                    newFormEventHandler = PrefsFormHandleEvent;
                    break;
#ifdef ENABLE_SEARCH
                case formID_search:
                    newFormEventHandler = SearchFormHandleEvent;
                    break;
#endif
            }
            if (newFormEventHandler)
            {
                frm = FrmInitForm(formId);
                FrmSetActiveForm(frm);
                FrmSetEventHandler(frm, newFormEventHandler);
                return true;
            }
        }
    }
    return false;
}

