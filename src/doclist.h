void    DocList_populateList(ListPtr l);
void    DocList_freeList();

UInt16  DocList_getCardNo(int i);
LocalID DocList_getID(int i);
Char*   DocList_getTitle(int i);
Int16   DocList_getIndex(char name[dmDBNameLength]);
UInt16  DocList_getDocCount();
