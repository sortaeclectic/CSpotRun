Boolean MainFormHandleEvent(EventType *e);
void MainForm_UCGUIChanged();

#ifdef ENABLE_AUTOSCROLL
Boolean MainForm_AutoScrollEnabled();
void MainForm_UpdateAutoScroll();
void MainForm_ToggleAutoScroll();
#endif
