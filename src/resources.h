#define formID_main             1000
#define formID_controls         1001
#define    formID_globalPrefs        1002
#ifdef ENABLE_SEARCH
    #define    formID_search            1003
#endif

#define menuID_main             1200
#define menuitemID_delete        1201
#define menuitemID_about           1202
#define menuitemID_controlPrefs    1203
#define menuitemID_controls        1204
#define menuitemID_makeDefault    1205
#ifdef ENABLE_SEARCH
    #define menuitemID_search        1206
    #define menuitemID_searchAgain    1207
#endif

#define menuitemID_font0    1210
#define menuitemID_font1    1211
#define menuitemID_font2    1212
#define menuitemID_font3    1213
#define menuitemID_lineSpacing0    1220
#define menuitemID_lineSpacing1    1221
#define menuitemID_lineSpacing2    1222
#define menuitemID_percent        1230
#define menuitemID_doc            1231
#define menuitemID_rotateRight    1232
#define menuitemID_rotateLeft    1233
#define menuitemID_globalPrefs    1234

#define    listID_doc            1301
#define    listID_percent        1302

#define popupID_doc            1401
#define popupID_percent        1402

#define buttonID_ok            1500
#define buttonID_cancel        1501
#define buttonID_rotateRight 1502
#define buttonID_rotateLeft  1503
#ifdef ENABLE_ROTATION
    #define ROTATION_BUTTON_COUNT 2
#else
    #define ROTATION_BUTTON_COUNT 0
#endif

#ifdef ENABLE_SEARCH
    #define buttonID_search            1504
    #define buttonID_searchAgain    1505
    #define SEARCH_BUTTON_COUNT 2
#else
    #define SEARCH_BUTTON_COUNT 0
#endif

#define fieldID_UCGUI0            1700
#ifdef ENABLE_SEARCH
    #define fieldID_searchString    1750
#endif

#define    gadgetID_text            1800
#define LINE_SPACING_GADGET_COUNT 3
#define    gadgetID_lineSpacing0    1801
#define    gadgetID_lineSpacing1    1802
#define    gadgetID_lineSpacing2    1803

#define alertID_about            1900
#define alertID_error            1901
#define alertID_confirmDelete    1902
#define FONT_BUTTON_COUNT 4
#define pushID_font0        2000
#define pushID_font1        2001
#define pushID_font2        2002
#define pushID_font3        2003
#ifdef ENABLE_SEARCH
    #define pushID_searchDown    2005
#endif
#define pushID_tapAction0    2050

#define RUNTIME_ID_START    4000

#define checkboxID_UCGUI0                1600
#define    checkboxID_reversePageButtons    1650
#define checkboxID_showLine                1651
#ifdef ENABLE_SEARCH
    #define checkboxID_searchFromTop        1652
    #define checkboxID_caseSensitive        1653
#endif

#define stringID_controlsHelp        2100
#define stringID_globalPrefsHelp    2101
#define stringID_ucguiFonts            2102
#define stringID_ucguiRotate        2103
#define stringID_ucguiSpacing        2104
#define stringID_ucguiGotoPerc        2105
#define stringID_ucguiPickDoc        2106
#define    stringID_noDocs                2107
#ifdef ENABLE_SEARCH
#define    stringID_ucguiSearch        2108
#define    stringID_ucguiSearchAgain    2109
#endif
#define stringID_aboutBody            2110
