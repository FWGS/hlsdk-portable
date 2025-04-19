#pragma once

#include <vgui/VGUI2.h>

#include <IClientVGUI.h>

class CClientVGUI : public IClientVGUI {
public:
	void Initialize(CreateInterfaceFn *factories, int count) override;
	void Start() override;
	void SetParent(vgui2::VPANEL parent) override;
	int UseVGUI1() override;
	void HideScoreBoard() override;
	void HideAllVGUIMenu() override;
	void ActivateClientUI() override;
	void HideClientUI() override;
	void Shutdown() override;
};