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
void RotCopyWindow(WinHandle fromWindowH, int ox, int oy, int ow, int oh, OrientationType a);
#else
void RotCopyWindow(WinHandle fromWindowH, int ox, int oy, OrientationType a);
#endif

int RotateY(int x, int y, OrientationType a);

#endif
#endif
