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
#include "resources.h"
#include "mainform.h"
#include "controlsform.h"
#include "bmknamefrm.h"
#include "bmkedfrm.h"
#include "bmk.h"
#include "appstate.h"
#include "ucgui.h"
#include "prefsform.h"
#include "docprefs.h"
#ifdef ENABLE_SEARCH
#include "searchform.h"
#endif
#include "decode.h"
#include "tabbedtext.h"


UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);
static void StartApp();
static void EventLoop();
static void StopApp();
static Boolean AppHandleEvent(EventType *e);
static void InitAppState();

struct APP_STATE_STR *appStatePtr;
#ifdef ENABLE_SEARCH
MemHandle   searchStringHandle;
Boolean     searchFromTop;
#endif

#define FORCE_1BIT_MODE

void savePDB (MemPtr cmdPBP);

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    if (cmd == sysAppLaunchCmdOpenDB) {
        UInt16      cardNo = ((SysAppLaunchCmdOpenDBType*)cmdPBP)->cardNo;
        LocalID   dbID = ((SysAppLaunchCmdOpenDBType*)cmdPBP)->dbID;
        UInt32     type, creator;

        // get the type/creator for this DB so we can see if it's a DOC file
        char startupDocName[dmDBNameLength];
        DmDatabaseInfo(cardNo, dbID, startupDocName, NULL, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL, &type, &creator);
        if (!(type == 'TEXt' && creator == 'REAd')) {
                FrmCustomAlert(alertID_error,
                               "You cannot open this type of database "\
                               "with CSpotRun. ", " ", " ");
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

Boolean UtilOSIsAtLeast(UInt8 reqMajor, UInt8 reqMinor)
{
    UInt32 romVersion;
    UInt8 major, minor;

    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    major = sysGetROMVerMajor(romVersion);
    minor = sysGetROMVerMinor(romVersion);

    if (major > reqMajor)
        return true;
    if (major == reqMajor && minor >= reqMinor)
        return true;
    return false;
}

static void StartApp()
{
    UInt16 prefsSize = 0;
#ifdef ENABLE_SEARCH
    Char* searchPtr = NULL;
#endif
    UInt32 newDepth = 1;
#ifdef ENABLE_BMK
    Err err;
#endif

    appStatePtr = (struct APP_STATE_STR *) MemPtrNew(sizeof(*appStatePtr));
#ifdef FORCE_1BIT_MODE
    if (UtilOSIsAtLeast(3,0))
    {
        UInt32 depth;
        WinScreenMode(winScreenModeGetSupportedDepths,
                      NULL, NULL, &depth, NULL);
        if (depth & 0x1)
            WinScreenMode(winScreenModeSet, NULL, NULL, &newDepth, NULL);
        else
            ErrFatalDisplay("No 1-bit mode!");
    }
#endif

    if (noPreferenceFound  == PrefGetAppPreferences(appId, PREF_APPSTATE,
                                                    NULL, &prefsSize, true)
        || prefsSize != sizeof(*appStatePtr))
        InitAppState();
    else
    {
        PrefGetAppPreferences(appId, PREF_APPSTATE, appStatePtr,
                              &prefsSize, true);
        if (appStatePtr->version != versionUInt16)
            InitAppState();
    }
    prefsSize = 0;

#ifdef ENABLE_SEARCH
    if (noPreferenceFound == PrefGetAppPreferences(appId, PREF_SEARCHSTRING,
                                                   NULL, &prefsSize, true))
    {
        searchStringHandle = MemHandleNew(2);
        searchPtr = MemHandleLock(searchStringHandle);
        StrCopy(searchPtr, "");
        MemHandleUnlock(searchStringHandle);
    }
    else
    {
        searchStringHandle = MemHandleNew(prefsSize);
        searchPtr = (Char*)MemHandleLock(searchStringHandle);
        PrefGetAppPreferences(appId, PREF_SEARCHSTRING, searchPtr,
                              &prefsSize, true);
        MemHandleUnlock(searchStringHandle);
    }
#endif

#ifdef ENABLE_BMK
    err = BmkStart();
    if(err)
        BmkReportError(err);
#endif

    FrmGotoForm(formID_main);

#ifdef ENABLE_HYPHEN
    // extract hyphenation tables
    LockHyphenResource();
#endif
}

static void InitAppState()
{
    MemSet(appStatePtr, sizeof(*appStatePtr), 0);

    appStatePtr->version            = versionUInt16;
    appStatePtr->UCGUIBits          = Ucgui_getDefaultUInt16();
    appStatePtr->hideControls       = 0;
    appStatePtr->reversePageUpDown  = 0;
    appStatePtr->showPreviousLine   = 1;
#ifdef ENABLE_AUTOSCROLL
    appStatePtr->autoScrollSpeed0   = 25;
    appStatePtr->autoScrollSpeed1   = 120;
    appStatePtr->autoScrollButton   = 1;
    appStatePtr->autoScrollType     = ATYPE_PIXEL;
#endif
    appStatePtr->tapAction          = TA_PAGE;

    DocPrefs_initPrefs(&appStatePtr->defaultDocPrefs, "");
}

static void StopApp()
{
#ifdef ENABLE_SEARCH
    Char* searchPtr = NULL;
#endif
    FrmCloseAllForms();

#ifdef ENABLE_HYPHEN
    // extract hyphenation tables
    UnlockHyphenResource();
#endif

#ifdef ENABLE_BMK
    BmkStop();
#endif

    PrefSetAppPreferences(appId, PREF_APPSTATE, versionUInt16,
                          appStatePtr, sizeof(*appStatePtr), true);
    MemPtrFree(appStatePtr);

    drm_unlink_library();

#ifdef ENABLE_SEARCH
    searchPtr = (Char*) MemHandleLock(searchStringHandle);
    PrefSetAppPreferences(appId, PREF_SEARCHSTRING, versionUInt16,
                          searchPtr, MemHandleSize(searchStringHandle), true);
    MemHandleUnlock(searchStringHandle);
    MemHandleFree(searchStringHandle);
#endif

#ifdef FORCE_1BIT_MODE
    if (UtilOSIsAtLeast(3,0))
        WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void EventLoop()
{
    EventType e;
    FormType *pfrm;
    UInt16 err;
#ifdef ENABLE_AUTOSCROLL
    Boolean autoScrollStopped = false;
    Int32 lastScrollTime = 0;
#endif

    do
    {
#ifdef ENABLE_AUTOSCROLL
        if(MainForm_AutoScrollEnabled())
        {
            Int32 now;
            Int32 interval;      //Desired time between scrolls.
            Int32 elapsed;       //Time since previous start of drawing
            Int32 timeToWait;    //Time to wait before nilEvent

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
                    //todo: Replace these magic numbers with
                    //macros. Hit prefsform.c's magic formulas too.

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
                        if(appStatePtr ->autoScrollSpeed1 > 230)
                            appStatePtr->autoScrollSpeed1 = 230;
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
                        appStatePtr->autoScrollSpeed1 -= 10;
                        if(appStatePtr ->autoScrollSpeed1 < 100)
                            appStatePtr->autoScrollSpeed1 = 100;
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

static Boolean AppHandleEvent(EventType *e)
{
    switch (e->eType)
    {
        case frmLoadEvent:
        {
            UInt16 formId;
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

#ifdef ENABLE_BMK
        case formID_bmkName:
            newFormEventHandler = BmkNameFormHandleEvent;
            break;

        case formID_bmkEd:
        case formID_bmkEd_ro:
            newFormEventHandler = BmkEdFormHandleEvent;
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

