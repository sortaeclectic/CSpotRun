#define IS_FONTSELECT_MENUID(id)        ((id) >= menuitemID_font0 && (id) < (menuitemID_font0 + FONT_BUTTON_COUNT))
#define FONTSELECT_MENUID_TO_INDEX(id)    ((id) - menuitemID_font0)

#define IS_FONTSELECT_PUSHID(id)        ((id) >= pushID_font0 && (id) < (pushID_font0 + FONT_BUTTON_COUNT))
#define FONTSELECT_PUSHID_TO_INDEX(id)    ((id) - pushID_font0)

Boolean FS_fontIsLegal(FontID f);
void    FS_changeFont(int index);
void    FS_updateFontButtons(FormPtr formPtr);
FontID  FS_getFontByIndex(UInt16 index);

