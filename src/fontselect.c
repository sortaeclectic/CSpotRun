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

#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>

#include "resources.h"
#include "appstate.h"
#include "rotate.h"
#include "fontselect.h"
#include "doc.h"

void FS_changeFont(int index)
{
    FontID    fontID = FS_getFontByIndex(index);

    if (FS_fontIsLegal(fontID) && Doc_getFont() != fontID)
    {
        Doc_setFont(fontID);
        Doc_drawPage();
    }
}

Boolean FS_fontIsLegal(FontID f)
{
    DWord romVersion;
    Byte os;

    // Disable the bold large font if not on a PalmIII
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    os = sysGetROMVerMajor(romVersion);

    if (2 == os && f == largeBoldFont)
        return false;

    return true;
}

void FS_updateFontButtons(FormPtr formPtr)
{
    int i;

    for(i = 0; i < FONT_BUTTON_COUNT; i++)
        CtlSetValue(FrmGetObjectPtr(formPtr, FrmGetObjectIndex(formPtr, pushID_font0+i)), FS_getFontByIndex(i) == Doc_getFont());
}

FontID FS_getFontByIndex(UShort index)
{
    static selectableFonts[FONT_BUTTON_COUNT] = {stdFont, boldFont, largeFont, largeBoldFont};
    return selectableFonts[index];
}
