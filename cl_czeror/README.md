# Client and menu of the mod

The client (cs16-client) and the menu (mainui_cpp) are taken as they are upstream. Everything
of the mod that goes on top of them is here and is applied by the build (.github/workflows/build.yml):

- `src/` - new files of the client, copied to `cl_dll/` (torch, fiber optic camera)
- `cs16-client.patch` - changes of the existing files of the client (HUD, events, grass, tools)
- `mainui/czds_mainui.patch` - world map and the CZ:DS switch of the menu (the pull request to FWGS/mainui_cpp)
- `mainui/czds_mainui_cs16.patch` - menu layout of the game only (not for the pull request)
- `mainui/czds_mainui_locks.patch` - unlocked missions of the world map
- `mainui/mainui_background.patch` - main menu background

Everything is switched on only for the game folder `czeror`.
