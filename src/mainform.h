/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "segments.h"

Boolean MainFormHandleEvent(EventType *e) MAINFORM_SEGMENT;
void MainForm_UCGUIChanged() MAINFORM_SEGMENT;

#ifdef ENABLE_AUTOSCROLL
Boolean MainForm_AutoScrollEnabled() MAINFORM_SEGMENT;
void MainForm_UpdateAutoScroll() MAINFORM_SEGMENT;
void MainForm_ToggleAutoScroll() MAINFORM_SEGMENT;
#endif
