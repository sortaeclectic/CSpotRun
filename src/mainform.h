/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
Boolean MainFormHandleEvent(EventType *e);
void MainForm_UCGUIChanged();

#ifdef ENABLE_AUTOSCROLL
Boolean MainForm_AutoScrollEnabled();
void MainForm_UpdateAutoScroll();
void MainForm_ToggleAutoScroll();
#endif
