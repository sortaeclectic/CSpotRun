/*
 * CSpotRun: A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2000  by Bill Clagett (wtc@pobox.com)
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
#include <UI/CharAttr.h>
#include "tabbedtext.h"

#define TAB_PIXELS (160/8)    //to match AportisDoc

WordPtr attrs = NULL;

// Yuck.  But faster...
void TT_WinDrawChars (CharPtr chars, Word len, SWord x, SWord y)
{
    static char *tooFar;    //pointer to past the end of chars
    static char *wordStart; //pointer to first char of current word (a "word" being a tab-free chunk of text)
    static SWord wordX;     //x where current word started
    static char c;

    if (!attrs)
        attrs = GetCharAttr();

    tooFar = &chars[len];

    wordStart = chars;
    wordX = x;
    while (chars < tooFar)
    {
        c = *chars;
        if (IsPrint(attrs, c))
            chars++;
        else
        {
            WinDrawChars(wordStart, chars-wordStart, wordX, y);
            x += FntCharsWidth(wordStart, chars-wordStart);
            if(c == '\t')
                x += (TAB_PIXELS - x%TAB_PIXELS);
            chars++;
            wordStart = chars; wordX = x;
        }
    }
    WinDrawChars(wordStart, chars-wordStart, wordX, y);
}
