/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998,1999  by Bill Clagett (wtc@pobox.com)
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
#include <KeyMgr.h>
#include <SysEvtMgr.h>
#include "app.h"
#include "resources.h"
#include "mainform.h"
#include "controlsform.h"
#include "appstate.h"
#include "ucgui.h"
#include "prefsform.h"
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
VoidHand searchStringHandle;
Boolean    searchFromTop;
#endif


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        StartApp();
        EventLoop();
        StopApp();
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
static void StartApp()
{
    VoidHand appStateHandle = MemHandleNew(sizeof(*appStatePtr));
    Word prefsSize = 0;
#ifdef ENABLE_SEARCH
    CharPtr searchPtr = NULL;
#endif
    appStatePtr = MemHandleLock(appStateHandle);
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
    appStatePtr->version = versionWord;
    appStatePtr->UCGUIBits = Ucgui_getDefaultWord();
    appStatePtr->hideControls = 0;
    appStatePtr->reversePageUpDown = 0;
    appStatePtr->showPreviousLine = 1;
#ifdef ENABLE_AUTOSCROLL
    appStatePtr->autoScrollSpeed = 60;
#endif
    appStatePtr->tapAction = TA_PAGE;
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
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

static void EventLoop()
{
    EventType e;
    FormType *pfrm;
    Word err;
    Long timeout;
    do
    {
#ifdef ENABLE_AUTOSCROLL
        if(MainForm_AutoScrollEnabled())
            EvtGetEvent(&e, appStatePtr->autoScrollSpeed);
        else
            EvtGetEvent(&e, evtWaitForever);

        // use Hard button 2 to toggle AutoScroll
        if((e.eType == keyDownEvent)&&(e.data.keyDown.chr == hard2Chr)&&
           (!(e.data.keyDown.modifiers & poweredOnKeyMask)))
        {
            MainForm_ToggleAutoScroll();
            continue;
        }

        if(MainForm_AutoScrollEnabled())
        {
            if(e.eType == nilEvent)
            {
                MainForm_UpdateAutoScroll();
            }
            else if(e.eType == penDownEvent)
            {
                MainForm_ToggleAutoScroll();
                continue;
            }
        }
#else
        EvtGetEvent(&e, evtWaitForever);
#endif

        if (!SysHandleEvent(&e))
            if (!MenuHandleEvent(MenuGetActiveMenu(), &e, &err))
                if (!AppHandleEvent(&e))
                    FrmDispatchEvent(&e);

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

