/*
This file is by Roy, here are some definitions that should help fix
some of the HL:Invasion annoyances.
*/

#define LoadTGA LoadTGAForRes //Original code uses LoadTGA in vgui_OrdiControl, vgui_soin, vgui_keypad and  vgui_radio, but it seems that this function doesn't exist. This one, however, does. It also seems to be functionally identical. So use it.
#define CRASHFIXPATH_INVASION_VGUI //vgui_OrdiControl.cpp tries to allocate 4 CImageLabels, but that causes vgui.so to crash. So avoid that. Using CommandButtons instead, until we can find the cause of that issue.
#define DOUBLECLICKFIXPATH_INVASION_VGUI //vgui_keypad receives double mouse key presses, at least when not in GoldSource compatible mode, making it virtually impossible to enter codes, let's treat this with a second-press timeout.
#define DONTSAVECAMERASFIX_INVASION_DLL //pSprite shouldn't be a member of m_SaveData in monster_camera, otherwise saving stops working after them being deactivated.
#define L2M3CRASHFIXPATH_INVASION_DLL //l2m3 map has a multi-manager targeting func_doors tremble_1 and tremble_2, which causes a segmentation fault CTD at least if the lib isn't compiled as GoldSource compatible. So we swap it's targets with something that doesn't exist.
#define NOATTACKFIXPATH_INVASION_VGUI //prevent attacks while VGUI is active.
