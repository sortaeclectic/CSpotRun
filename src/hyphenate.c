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
#include <PalmOSGlue.h>
#include "myCharAttr.h"
#include "hyphenate.h"

#ifdef ENABLE_HYPHEN

static UInt16 hyphenate();

//
//  Make a word wrap with hyphenation
//

static Boolean _noHyphen;   // if the hyphen database available

static DmOpenRef _hyphenDbRef;


/*
 *  Compute len of a displayed line with hyphen
 */
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
    // take care of leading tab
    if (*chars == '\t') {
        remainx = extend - FntCharsWidth(chars+1, lenx-1) - 160/8;
    }
    else  
        remainx = extend - FntCharsWidth(chars, lenx);

    // convert remaining pixels to remaining chars
    remainx = remainx/TxtGlueCharWidth('a');

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

unsigned char *hyf_distance, *hyf_num, *hyf_next;
unsigned char *trie_char, *trie_op;
unsigned short *trie_link;

#define DEL (unsigned char)127

//
// Extract hyphen tables lookup
//
void LockHyphenResource ()
{
    UInt16      cardNo=0;
    LocalID lID;
    DmSearchStateType    searchState;

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
}

//
// Release hyphen tables lookup
//
void UnlockHyphenResource ()
{
    if (_noHyphen) return;

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
}

# define tolower(c) ({ unsigned char __x = (c); TxtGlueCharIsUpper((UInt8)__x) ? (__x - 'A' + 'a') : __x;})

//
// Make the real hyphenation
//
static UInt16 hyphenate(word, room, hlevel, hn)
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
        hc[j+1] = (TxtCharIsAlpha((UInt8)word[j]))? tolower(word[j]) - 1 : (unsigned char)DEL;

    for (j = 0; j <= hn - 2; j++) {
        z = hc[j];
        l = j;
        while (hc[l] == trie_char[z]) {
            v = trie_op[z];
            if (v) {
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
        if (!TxtGlueCharIsAlpha((UInt8)word[j])) {
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
