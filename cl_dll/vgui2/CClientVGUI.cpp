#include <tier0/dbg.h>

#include <tier1/interface.h>
#include <tier1/tier1.h>

#include <tier2/tier2.h>

#include <vgui_controls/Controls.h>

#include "CClientVGUI.h"

namespace vgui2 {
	vgui2::HScheme VGui_GetDefaultScheme() {
		return 0;
	}
}

EXPOSE_SINGLE_INTERFACE(CClientVGUI, IClientVGUI, ICLIENTVGUI_NAME);

void CClientVGUI::Initialize(CreateInterfaceFn *factories, int count) {
	ConnectTier1Libraries(factories, count);
	ConnectTier2Libraries(factories, count);
	
	if (!vgui2::VGui_InitInterfacesList("CLIENT", factories, count)) {
		Error("Failed to intialize VGUI2!\n");
		return;
	}
}

void CClientVGUI::Start() {

}

void CClientVGUI::SetParent(vgui2::VPANEL parent) {

}

// TODO: Setting to false breaks the mouse cursor
int CClientVGUI::UseVGUI1() {
	return true;
}

void CClientVGUI::HideScoreBoard() {

}

void CClientVGUI::HideAllVGUIMenu() {

}

void CClientVGUI::ActivateClientUI() {

}

void CClientVGUI::HideClientUI() {

}

void CClientVGUI::Shutdown() {

}