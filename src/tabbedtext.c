/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * A doc-format database reader for the Palm Computing Platform.
 * Copyright (C) 1998-2002  by Bill Clagett (wtc@pobox.com)
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

#include <PalmOS.h>
#include <Core/System/CharAttr.h>
#include "tabbedtext.h"
#include <PalmOSGlue.h>

#define TAB_PIXELS (160/8)    //to match AportisDoc

const UInt16* attrs = NULL;

//
// Draw tabulated text
//
// Yuck.  But faster...
void Tab_WinDrawChars (Char* chars, UInt16 len, Int16 x, Int16 y)
{
    static char *tooFar;    //pointer to past the end of chars
    static char *wordStart; //pointer to first char of current
                            //word (a "word" being a tab-free chunk of text)
    static Int16 wordX;     //x where current word started
    static char c;

    tooFar = &chars[len];

    wordStart = chars;
    wordX = x;
    while (chars < tooFar)
    {
        c = *chars;
        if (!TxtGlueCharIsCntrl((UInt8)c))
            chars++;
        else
        {
            WinDrawChars(wordStart, chars-wordStart, wordX, y);
            x += FntCharsWidth(wordStart, chars-wordStart);
            if (c == '\t')
                x += (TAB_PIXELS - x%TAB_PIXELS);
            chars++;
            wordStart = chars; wordX = x;
        }
    }
    WinDrawChars(wordStart, chars-wordStart, wordX, y);
}

/*
// for debug use
static char *_ascii(Char c)
{
    static char xx[3];
    xx[0] = (c >> 4);
    xx[0] += (xx[0]>9) ? ('a'-10) : '0';
    xx[1] = (c & 0x0f);
    xx[1] += (xx[1]>9) ? ('a'-10) : '0';
    return xx;
}
*/
//
// Draw justified line
//
void Justify_WinDrawChars (Char* chars, UInt16 len, Int16 x, Int16 y, UInt16 extend)
{
    // find out an array of words with their len
    static char *start[16]; // start of the word
    static Int16 ulen[16];  // width in char
    static UInt16 wlen[16]; // width in pixel
    static UInt16 wlength;  // length of all words
    static char *p,  *old;
    static Int16 i, j, wordx;
    static UInt16 space, leave;
    static Int16 eol;
    static char bHyphen;    // Is end of word hyphenated

    eol = 0;
    bHyphen=0;

/*
    // for debug use
    for (i=0; i<len; i++) {
        WinDrawChars(_ascii(chars[i]), 2, x, y);
        x += 12;
    }

    return;
*/

    // if CR/LF at end of line, just draw the line
    p = chars + len - 1;
    switch (*p) {
        case '\n':
        case '\r':
            if (--len <= 0) return;
            WinDrawChars(chars, len, x, y);
            return;
        case ' ':
            // remove trailing spaces
            while ((*p) == ' ') { len--; p--; };
    }


    // if the char next to the last one is an alpha, then we make hyphenation
    if (( (*p) != '-') && TxtGlueCharIsAlpha((UInt8)chars[len]))
        bHyphen=1;

    old = start[0] = chars;
    i=0;
    wlength=0;

    // count and memorize words
    wordx=len;
    for (p = chars+1; --wordx; p++) {
        switch (*p) {
            case '\t':
                Tab_WinDrawChars (chars, len, x, y);
                return;
            case ' ':
                // find out multiple spaces
                if (p[-1] == ' ') continue;

                // find the end of the word and record the len
                start[i+1] = p+1;
                ulen[i]= p - old;                       // len in bytes minus the space
                wlen[i]= FntCharsWidth(old, ulen[i]);   // len in pixels minus the space
                wlength += wlen[i];                     // total length in pixel

                i++;

                // security
                if (i > 15) {
                    Tab_WinDrawChars (chars, len, x, y);
                    return;
                }

                old = p+1;
                break;
        }
    }
    ulen[i]= p - old;
    wlen[i]= FntCharsWidth(old, ulen[i]);
    wlength += wlen[i];
    i++;

    // if only one word, draw it
    if (i<2) {
        WinDrawChars(chars, len, x, y);
        return;
    }

    // If hyphenation, add the size of the hyphen
    if (bHyphen)
        wlength += TxtGlueCharWidth('-');

    // if a bug in the length
    if (wlength > extend) {
        WinDrawChars(chars, len, x, y);
        return;
    }

    // compute spacing and pixels remaining
    space = (extend - wlength)/(i-1);
    leave = (extend - wlength)%(i-1);

    // drawing justified words
    wordx = x;
    for (j=0; j<i; j++) {
        WinDrawChars(start[j], ulen[j], wordx, y);

        wordx += (wlen[j] + space);

        if (leave)  {
            leave--;    // remaining pixels to distribute
            wordx++;
        };
    }

    // if hyphenation draw the hyphen
    if (bHyphen)
        WinDrawChars("-", 1, wordx-space, y);
}
