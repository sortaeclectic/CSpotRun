#ifndef DOCPREFS_INCLUDED
#define DOCPREFS_INCLUDED yup

struct DOC_LOCATION_STR
{
    UInt record;
    UInt ch;
};

struct DOC_PREFS_STR
{
    char            name[dmDBNameLength];
    FontID            font;
    UShort            lineHeightAdjust;
    struct DOC_LOCATION_STR location;
    Word            orient;
    Word            spare[10-1];
};


#define DEFAULT_DOCPREFS {"dummy", stdFont, 1, {1, 0}, angle0, {}}

void DocPrefs_loadPrefs(char name[dmDBNameLength], struct DOC_PREFS_STR * prefs);
void DocPrefs_savePrefs(struct DOC_PREFS_STR * prefs);
void DocPrefs_getRecentDocName(char name[dmDBNameLength]);
void DocPrefs_cleanUpPrefs();
void DocPrefs_initPrefs(struct DOC_PREFS_STR *p, char name[dmDBNameLength]);

#endif
