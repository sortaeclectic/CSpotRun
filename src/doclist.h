void    DocList_populateList(ListPtr l);
void    DocList_freeList();

UInt    DocList_getCardNo(int i);
LocalID DocList_getID(int i);
CharPtr    DocList_getTitle(int i);
Int        DocList_getIndex(char name[dmDBNameLength]);
UInt    DocList_getDocCount();
