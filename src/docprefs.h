/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef DOCPREFS_INCLUDED
#define DOCPREFS_INCLUDED yup

struct DOC_LOCATION_STR
{
    UInt16 record;
    UInt16 ch;
};

struct DOC_PREFS_STR
{
    char            name[dmDBNameLength];
    FontID            font;
    UInt16            lineHeightAdjust;
    struct DOC_LOCATION_STR location;
    UInt16            orient;
    UInt16          justify;
    UInt16          hyphen;
    UInt16            spare[10-3];
};


#define DEFAULT_DOCPREFS {"dummy", stdFont, 1, {1, 0}, angle0, 0, 0, {}}

void DocPrefs_loadPrefs(char name[dmDBNameLength], struct DOC_PREFS_STR * prefs);
void DocPrefs_savePrefs(struct DOC_PREFS_STR * prefs);
void DocPrefs_getRecentDocName(char name[dmDBNameLength]);
void DocPrefs_cleanUpPrefs();
void DocPrefs_initPrefs(struct DOC_PREFS_STR *p, char name[dmDBNameLength]);
void DocPrefs_setStartupDocName(char startupDocName[dmDBNameLength]);
#endif
