/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "segments.h"

#ifndef ROTATE_INCLUDED
#define ROTATE_INCLUDED

enum ORIENTATION_ENUM
{
    angle0,
    angle90,
    angle180,
    angle270,
    ORIENTATION_COUNT
} ;
typedef enum ORIENTATION_ENUM OrientationType;

#ifdef ENABLE_ROTATION

#define isSideways(r) ((r)==angle90 || (r) == angle270)

#ifdef ENABLE_AUTOSCROLL
void RotCopyWindow(WinHandle fromWindowH, int startRow, int stopRow, OrientationType a) ROTATE_SEGMENT;
void RotScrollRectangleUp(RectangleType *rect, OrientationType o) ROTATE_SEGMENT;
#else
void RotCopyWindow(WinHandle fromWindowH, int ox, int oy, OrientationType a) ROTATE_SEGMENT;
#endif /* ENABLE_AUTOSCROLL */

int RotateY(int x, int y, OrientationType a) ROTATE_SEGMENT;
Boolean RotCanDoRotation();

#endif /* ENABLE_ROTATION */
#endif
