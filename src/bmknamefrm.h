/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef _BMKNAMEFRM_H_
#define _BMKNAMEFRM_H_

#include "segments.h"

#ifdef ENABLE_BMK
extern char bmkName[];

Boolean BmkNameFormHandleEvent(EventType *e) BMK_SEGMENT;
#endif

#endif


