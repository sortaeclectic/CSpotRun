
#ifndef _BMK_H_
#define _BMK_H_

#ifdef ENABLE_BMK

#include <Common.h>
#include <SystemMgr.h>
#include <UI/List.h>


#define BMK_NAME_SIZE	15


/*
 * actions to perform when special items in the
 * bookmarks list are chosen
 */
#define A_NEW		1
#define A_EDIT		2


#define bmkErrorClass		(appErrorClass | 0x0100)

#define bmkErrDocNotOpened	(bmkErrorClass | 0x0001)


#define bmkListRedrawEvt        0x6001


/* actions for BmkMove() */
#define MOVE_UP			1
#define MOVE_DOWN		2

/* actions for BmkSort() */
#define SORT_POS		1
#define SORT_NAME		2


/*
 * bookmark record structure as documented at
 * http://www.pyrite.org/doc_format.php
 */
typedef struct bmk_rec {
	char	name[16];
	DWord	pos;
} bmk_rec_t;


Err	BmkStart(void);
void	BmkStop(void);
Err	BmkCloseDoc(void);
Err	BmkPopulateList(ListPtr l, int shortcuts, int resize);
int	BmkGetAction(int selection);
void	BmkGoTo(int selection, int shortcuts);
Err	BmkAdd(char *name, int);
Err	BmkRename(int sel, char *name);
Err	BmkDelete(int selection);
Err	BmkDeleteAll(void);
Err	BmkSort(int);
Err	BmkMove(int action, int sel);

void	BmkReportError(Err);

#endif

#endif

