/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * A doc-format database reader for the Palm Computing Platform.
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

#include <PalmOS.h>
#include <Core/System/CharAttr.h>
#include "tabbedtext.h"

#define TAB_PIXELS (160/8)    //to match AportisDoc

const UInt16* attrs = NULL;

static UInt16 hyphenate(unsigned char *word, UInt16 room, UInt16 hlevel, UInt16 hn);

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

    if (!attrs)
        attrs = GetCharAttr();

    tooFar = &chars[len];

    wordStart = chars;
    wordX = x;
    while (chars < tooFar)
    {
        c = *chars;
        if (!IsCntrl(attrs, c))
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
    if (( (*p) != '-') && TxtCharIsAlpha(chars[len]))
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
        wlength += TxtCharWidth('-');

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
        WinDrawChar('-', wordx-space, y);
}

/*********************************************************************
 *  Hyphenation part
 */
#ifdef ENABLE_HYPHEN

//
//  Make a word wrap with hyphenation
//

static Boolean _noHyphen;   // if the hyphen database available

static DmOpenRef _hyphenDbRef;


UInt16 Hyphen_FntWordWrap (Char* chars, UInt16 extend)
{
    static UInt16 len,lenx;   // len int bytes
    static Int16 remainx;  // pixels remaining
    static char *next;  // begin of next word
    static char *p;
    static UInt16 next_len;// len in bytes of the next word
    static UInt16 next_lenx;// len in pixels of the hiphenation

    lenx = len = FntWordWrap(chars, extend);

    if (_noHyphen) return len;

    switch (chars[len-1]) {
        case '\r':
        case '\n':
            return len; // end of line, don't look for hyphenation
        case ' ':
            lenx--;     // Remove trailing space
    }


    // compute string len in pixels
    remainx = extend - FntCharsWidth(chars, lenx);

    // convert remaining pixels to remaining chars
    remainx = remainx/TxtCharWidth('a');

    // don't try hiphenation if less than 4 chars
    if (remainx<4) return len;

    // begin of the next word, return if end of string
    next = chars + len ;
    if (*next == 0) return len;

    // len of the next word
    next_len=0;
    for (p=next; (*p) && (*p) != ' '; p++)  next_len++;

    // compute the hyphen
    next_len = hyphenate(next, remainx, 5, next_len);
    if (!next_len) return len;

    // return total number of bytes jumped
    return len + next_len - 1;
}

/*
 * Function hyphenate() takes as arguments a word to be split up
 * by hyphenation, the room available to fit the first part into, and
 * the hyphenation level.  If successful in finding a hyphenation
 * point that will permit a small enough first part, the function
 * returns the length of the first part, including one space for the
 * hyphen that will presumably be added.  Otherwise, the original
 * length of the word is returned.
 * Hyphenation level >= 6 blocks hyphenation after hyphen, and
 * level >= 7 blocks hyphenation when there are non-alphabetic
 * characters surrounded by alphabetic ones (such as in email
 * addresses).
 *
 * The tables were taken from a dump of TeX (initex) after
 * it finished digesting the file hyphen.tex, and consequently this
 * data is derivative from the work of Donald E. Knuth and Frank
 * M. Liang.  Almost all the code for the hyphenation function was
 * taken from TeX, from sections 923-924 of TeX: The Program, by
 * Donald E. Knuth, 1986, Addison Wesley.  See section 919-920 of
 * that book for other references.  The original algorithm is
 * intended to apply only to spans of alphabetic characters, but
 * here it is applied more generally.  If it malfunctions, then,
 * that is likely my fault.
 *
 * This program code is in the public domain.
 * Greg Lee, 4/10/92.
 */

#ifdef SAVE_HYPHEN_PDB
#include "hyph_eng.h"
    extern unsigned short *trie_link;
#else
    unsigned char *hyf_distance, *hyf_num, *hyf_next;
    unsigned char *trie_char, *trie_op;
    unsigned short *trie_link;
#endif

#define DEL (unsigned char)127

//
// Extract hyphen tables lookup
//
void LockHyphenResource ()
{
    UInt16      cardNo=0;
    LocalID lID;
    DmSearchStateType    searchState;

#ifdef SAVE_HYPHEN_PDB
    savePDB ();
#else

    lID = DmFindDatabase (cardNo, "CSpotRun_hyphen");
    if (!lID) {
        _noHyphen = true;
        return;
    }

    _noHyphen=false;

    _hyphenDbRef = DmOpenDatabase (cardNo, lID, dmModeReadOnly);

    hyf_distance = (unsigned char *) MemHandleLock(DmGetRecord(_hyphenDbRef, 0));
    hyf_num = (unsigned char *) MemHandleLock(DmGetRecord(_hyphenDbRef, 1));
    hyf_next = (unsigned char *) MemHandleLock(DmGetRecord(_hyphenDbRef, 2));
    trie_char = (unsigned char *) MemHandleLock(DmGetRecord(_hyphenDbRef, 3));
    trie_link = (unsigned short *) MemHandleLock(DmGetRecord(_hyphenDbRef, 4));
    trie_op = (unsigned char *) MemHandleLock(DmGetRecord(_hyphenDbRef, 5));


#endif
}

//
// Release hyphen tables lookup
//
void UnlockHyphenResource ()
{
    if (_noHyphen) return;

#ifndef SAVE_HYPHEN_PDB
    _noHyphen=false;

    MemPtrUnlock(hyf_distance);
    DmReleaseRecord(_hyphenDbRef, 0, false);

    MemPtrUnlock(hyf_num);
    DmReleaseRecord(_hyphenDbRef, 1, false);

    MemPtrUnlock(hyf_next);
    DmReleaseRecord(_hyphenDbRef, 2, false);

    MemPtrUnlock(trie_char);
    DmReleaseRecord(_hyphenDbRef, 3, false);

    MemPtrUnlock(trie_link);
    DmReleaseRecord(_hyphenDbRef, 4, false);

    MemPtrUnlock(trie_op);
    DmReleaseRecord(_hyphenDbRef, 5, false);

    DmCloseDatabase (_hyphenDbRef);
#endif
}

#ifdef SAVE_HYPHEN_PDB
//
// Save the hyphenation lookup tables
//
void savePDB (MemPtr cmdPBP)
{
    UInt16      cardNo;
    DmOpenRef   hyph;
    LocalID lID;
    MemHandle mh;
    unsigned char *b;
    unsigned short *c;
    DmSearchStateType    searchState;
    UInt16 i;

    DmGetNextDatabaseByTypeCreator(true, &searchState, 0, 0, false, &cardNo, &lID);

/*
    lID = DmFindDatabase (cardNo, "CSpotRun_hyphen");
    hyph = DmOpenDatabase (cardNo, lID, dmModeReadOnly);
    ErrFatalDisplayIf(!hyph, "trouve pas");

    mh = DmGetRecord (hyph, 0);
    ErrFatalDisplayIf(!mh, "pas de record");

    b = (unsigned char *) MemHandleLock(mh);
    for (i=0; i<sizeof(hyf_distance); i++)
        ErrFatalDisplayIf(b[i] != hyf_distance[i], "YA PA BON");
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, 0, false);

    mh = DmGetRecord (hyph, 1);
    ErrFatalDisplayIf(!mh, "pas de record 1");

    b = (unsigned char *) MemHandleLock(mh);
    for (i=0; i<sizeof(hyf_num); i++)
        ErrFatalDisplayIf(b[i] != hyf_num[i], "YA PA BON hyf_num");
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, 1, false);

    mh = DmGetRecord (hyph, 2);
    ErrFatalDisplayIf(!mh, "pas de record 2");

    b = (unsigned char *) MemHandleLock(mh);
    for (i=0; i<sizeof(hyf_next); i++)
        ErrFatalDisplayIf(b[i] != hyf_next[i], "YA PA BON hyf_next");
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, 2, false);

    mh = DmGetRecord (hyph, 3);
    ErrFatalDisplayIf(!mh, "pas de record 3");

    b = (unsigned char *) MemHandleLock(mh);
    for (i=0; i<sizeof(trie_char); i++)
        ErrFatalDisplayIf(b[i] != trie_char[i], "YA PA BON trie_char");
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, 3, false);

    mh = DmGetRecord (hyph, 4);
    ErrFatalDisplayIf(!mh, "pas de record 4");

    c = (unsigned short *) MemHandleLock(mh);
    for (i=0; i<sizeof(trie_link); i++)
        ErrFatalDisplayIf(c[i] != trie_link[i], "YA PA BON trie_link");
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, 4, false);

    mh = DmGetRecord (hyph, 5);
    ErrFatalDisplayIf(!mh, "pas de record 3");

    b = (unsigned char *) MemHandleLock(mh);
    for (i=0; i<sizeof(trie_op); i++)
        ErrFatalDisplayIf(b[i] != trie_op[i], "YA PA BON trie_op");
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, 5, false);
*/

    DmCreateDatabase (cardNo, "CSpotRun_hyphen", 'CSBh', 'HYPh', false);
    lID = DmFindDatabase (cardNo, "CSpotRun_hyphen");
    hyph=DmOpenDatabase (cardNo, lID, dmModeWrite);

    i=0;
    mh = DmNewRecord (hyph, &i, sizeof (hyf_distance));
    b = (unsigned char *) MemHandleLock(mh);
    DmWrite (b, 0, hyf_distance, sizeof(hyf_distance));
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, i, true);

    i++;
    mh = DmNewRecord (hyph, &i, sizeof (hyf_num));
    b = (unsigned char *) MemHandleLock(mh);
    DmWrite (b, 0, hyf_num, sizeof(hyf_num));
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, i, true);

    i++;
    mh = DmNewRecord (hyph, &i, sizeof (hyf_next));
    b = (unsigned char *) MemHandleLock(mh);
    DmWrite (b, 0, hyf_next, sizeof(hyf_next));
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, i, true);

    i++;
    mh = DmNewRecord (hyph, &i, sizeof (trie_char));
    b = (unsigned char *) MemHandleLock(mh);
    DmWrite (b, 0, trie_char, sizeof(trie_char));
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, i, true);

    i++;
    mh = DmNewRecord (hyph, &i, sizeof (trie_link));
    b = (unsigned char *) MemHandleLock(mh);
    DmWrite (b, 0, trie_link, sizeof(trie_link));
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, i, true);

    i++;
    mh = DmNewRecord (hyph, &i, sizeof (trie_op));
    b = (unsigned char *) MemHandleLock(mh);
    DmWrite (b, 0, trie_op, sizeof(trie_op));
    MemHandleUnlock(mh);
    DmReleaseRecord(hyph, i, true);

    DmCloseDatabase (hyph);
}
#endif

# define tolower(c) \
    ({ unsigned char __x = (c); TxtCharIsUpper(__x) ? (__x - 'A' + 'a') : __x;})

//
// Make the real hyphenation
//
UInt16 hyphenate(word, room, hlevel, hn)
unsigned char *word;
UInt16 room, hlevel, hn;
{
    static unsigned char hc[32], hyf[32];
    static char embflag = 0;
    static UInt16 i, j, z, l, v, ret;

    if (hn < 5) return(0);
    if (hn > sizeof(hc)) return(0);

    // clear buffer
    MemSet (hyf, sizeof(hyf), 0);

    hc[0] = hc[hn+1] = (unsigned char)DEL;
    hc[hn+2] = (unsigned char)256;

    for (j = 0; j<hn; j++)
        hc[j+1] = (TxtCharIsAlpha(word[j]))? tolower(word[j]) - 1 : (unsigned char)DEL;

    for (j = 0; j <= hn - 2; j++) {
        z = hc[j];
        l = j;
        while (hc[l] == trie_char[z]) {
            if (v = trie_op[z]) {
                do {
                    i = l - hyf_distance[v];
                    if (hyf_num[v] > hyf[i]) hyf[i] = hyf_num[v];
                    v = hyf_next[v];
                } while (v);
            }
            l++;
            z = trie_link[z] + hc[l];
        }
    }

    hyf[1] = 0; hyf[hn-1] = 0; hyf[hn] = 0; hyf[hn-2] = 0;
    for (j = 0; j<hn; j++) {
        if (!TxtCharIsAlpha(word[j])) {
            hyf[j+1] = 0;
            hyf[j+2] = 0;
            hyf[j] = 0;
            if (j > 0) hyf[j-1] = 0;
            if (j > 1) hyf[j-2] = 0;
            if (embflag == 1) embflag++;
        }
        /* hlevel 7 or greater prevents split when non-alpha
         * is in the midst of alpha characters */
        else if (hlevel >= 7) {
            if (!embflag) embflag++;
            else if (embflag == 2) return(hn);
        }
        /* hlevel 6 or greater prevents split after '-' */
        if (hlevel <= 5 && word[j] == '-') hyf[j+1] = 1;
    }

    ret = 0;
    for (j = hn-3 ; j > 1; j--) {
        z = (hc[j]+1 == '-')? j : j+1;
        if ((hyf[j] % 2) && (z <= room)) {
            ret = z;
            break;
        }
    }
    return(ret);
}
#endif