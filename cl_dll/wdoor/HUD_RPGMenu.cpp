#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_RPGMenu, WRPGMenu)

int CHudRPGMenu::Init(void)
{
	m_menu_on = 0;
	m_menu_select = 0;
	m_menu_select_alpha = 0;

	m_actor1 = 0;
	m_hp1 = 0;
	m_maxhp1 = 0;
	m_numhp1 = 0;
	m_level1 = 0;
	m_exp1 = 0;
	m_maxexp1 = 0;
	m_status1 = 0;

	m_actor2 = 0;
	m_hp2 = 0;
	m_maxhp2 = 0;
	m_numhp2 = 0;
	m_level2 = 0;
	m_exp2 = 0;
	m_maxexp2 = 0;
	m_status2 = 0;

	m_actor3 = 0;
	m_hp3 = 0;
	m_maxhp3 = 0;
	m_numhp3 = 0;
	m_level3 = 0;
	m_exp3 = 0;
	m_maxexp3 = 0;
	m_status3 = 0;

	m_actor4 = 0;
	m_hp4 = 0;
	m_maxhp4 = 0;
	m_numhp4 = 0;
	m_level4 = 0;
	m_exp4 = 0;
	m_maxexp4 = 0;
	m_status4 = 0;

	m_actor5 = 0;
	m_hp5 = 0;
	m_maxhp5 = 0;
	m_numhp5 = 0;
	m_level5 = 0;
	m_exp5 = 0;
	m_maxexp5 = 0;
	m_status5 = 0;

	m_item1 = 0;
	m_item2 = 0;
	m_item3 = 0;
	m_item4 = 0;
	m_item5 = 0;
	m_item6 = 0;
	m_item7 = 0;
	m_item8 = 0;
	m_item9 = 0;
	m_item10 = 0;
	m_item11 = 0;
	m_item12 = 0;
	m_item_s = 0;
	m_item_e = -1;
	m_skill0 = 0;
	m_skill1 = 0;
	m_skill2 = 0;
	m_skill3 = 0;
	m_skill4 = 0;
	m_skill5 = 0;
	m_skill6 = 0;
	m_skill7 = 0;
	m_skill8 = 0;
	m_skill9 = 0;
	HOOK_MESSAGE(WRPGMenu);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
};

int CHudRPGMenu::VidInit(void)
{
	int HUD_flash_d = gHUD.GetSpriteIndex( "health_check_new1" );
	m_spr_actor1_hp = gHUD.GetSprite(HUD_flash_d);
	m_prc1_hp = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc1_exp = &gHUD.GetSpriteRect(HUD_flash_d);

	m_spr_actor2_hp = gHUD.GetSprite(HUD_flash_d);
	m_prc2_hp = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc2_exp = &gHUD.GetSpriteRect(HUD_flash_d);

	m_spr_actor3_hp = gHUD.GetSprite(HUD_flash_d);
	m_prc3_hp = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc3_exp = &gHUD.GetSpriteRect(HUD_flash_d);

	m_spr_actor4_hp = gHUD.GetSprite(HUD_flash_d);
	m_prc4_hp = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc4_exp = &gHUD.GetSpriteRect(HUD_flash_d);

	m_spr_actor5_hp = gHUD.GetSprite(HUD_flash_d);
	m_prc5_hp = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc5_exp = &gHUD.GetSpriteRect(HUD_flash_d);

	return 1;
};

int CHudRPGMenu:: MsgFunc_WRPGMenu(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_menu_on = READ_BYTE();
	m_menu_select = READ_BYTE();
	m_menu_select_alpha = READ_BYTE();
//	m_menu_origin = READ_BYTE();

	m_actor1 = READ_BYTE();
	m_hp1 = READ_BYTE();
	m_maxhp1 = READ_LONG();
	m_numhp1 = READ_LONG();
	m_level1 = READ_BYTE();
	m_exp1	 = READ_BYTE();
	m_maxexp1 = READ_LONG();
	m_status1 = READ_BYTE();

	m_actor2 = READ_BYTE();
	m_hp2 = READ_BYTE();
	m_maxhp2 = READ_LONG();
	m_numhp2 = READ_LONG();
	m_level2 = READ_BYTE();
	m_exp2	 = READ_BYTE();
	m_maxexp2 = READ_LONG();
	m_status2 = READ_BYTE();

	m_actor3 = READ_BYTE();
	m_hp3 = READ_BYTE();
	m_maxhp3 = READ_LONG();
	m_numhp3 = READ_LONG();
	m_level3 = READ_BYTE();
	m_exp3	 = READ_BYTE();
	m_maxexp3 = READ_LONG();
	m_status3 = READ_BYTE();

	m_actor4 = READ_BYTE();
	m_hp4 = READ_BYTE();
	m_maxhp4 = READ_LONG();
	m_numhp4 = READ_LONG();
	m_level4 = READ_BYTE();
	m_exp4	 = READ_BYTE();
	m_maxexp4 = READ_LONG();
	m_status4 = READ_BYTE();

	m_actor5 = READ_BYTE();
	m_hp5 = READ_BYTE();
	m_maxhp5 = READ_LONG();
	m_numhp5 = READ_LONG();
	m_level5 = READ_BYTE();
	m_exp5	 = READ_BYTE();
	m_maxexp5 = READ_LONG();
	m_status5 = READ_BYTE();

	m_item1 = READ_BYTE();
	m_item2 = READ_BYTE();
	m_item3 = READ_BYTE();
	m_item4 = READ_BYTE();
	m_item5 = READ_BYTE();
	m_item6 = READ_BYTE();
	m_item7 = READ_BYTE();
	m_item8 = READ_BYTE();
	m_item9 = READ_BYTE();
	m_item10 = READ_BYTE();
	m_item11 = READ_BYTE();
	m_item12 = READ_BYTE();
	m_item_s = READ_BYTE();
	m_item_e = READ_BYTE();
	m_skill0 = READ_BYTE();
	m_skill1 = READ_BYTE();
	m_skill2 = READ_BYTE();
	m_skill3 = READ_BYTE();
	m_skill4 = READ_BYTE();
	m_skill5 = READ_BYTE();
	m_skill6 = READ_LONG();
	m_skill7 = READ_BYTE();
	m_skill8 = READ_BYTE();
	m_skill9 = READ_BYTE();
	m_skill10 = READ_BYTE();
	m_skill11 = READ_BYTE();
	m_skill12 = READ_BYTE();
	return 1;
}

char *actorchar;
void GetActor(int actor)
{
	if(actor == 1)
	{
		actorchar = "sprites/wdoor_menu_head1.spr";
	}
	else if(actor == 2)
	{
		actorchar = "sprites/wdoor_menu_head2.spr";
	}
	else if(actor == 3)
	{
		actorchar = "sprites/wdoor_menu_head3.spr";
	}
	else if(actor == 4)
	{
		actorchar = "sprites/wdoor_menu_head4.spr";
	}
	else if(actor == 5)
	{
		actorchar = "sprites/wdoor_menu_head5.spr";
	}
	else if(actor == 6)
	{
		actorchar = "sprites/wdoor_menu_head6.spr";
	}
	else if(actor == 7)
	{
		actorchar = "sprites/wdoor_menu_head7.spr";
	}
	else if(actor == 8)
	{
		actorchar = "sprites/wdoor_menu_head8.spr";
	}
	else if(actor == 9)
	{
		actorchar = "sprites/wdoor_menu_head9.spr";
	}
	else if(actor == 10)
	{
		actorchar = "sprites/wdoor_menu_head10.spr";
	}
	else if(actor == 11)
	{
		actorchar = "sprites/wdoor_menu_head11.spr";
	}
	else if(actor == 12)
	{
		actorchar = "sprites/wdoor_menu_head12.spr";
	}
	else if(actor == 13)
	{
		actorchar = "sprites/wdoor_menu_head13.spr";
	}
	else if(actor == 14)
	{
		actorchar = "sprites/wdoor_menu_head14.spr";
	}
	else if(actor == 15)
	{
		actorchar = "sprites/wdoor_menu_head15.spr";
	}
	else if(actor == 16)
	{
		actorchar = "sprites/wdoor_menu_head16.spr";
	}
	else if(actor == 17)
	{
		actorchar = "sprites/wdoor_menu_head17.spr";
	}
	else if(actor == 18)
	{
		actorchar = "sprites/wdoor_menu_head18.spr";
	}
	else if(actor == 19)
	{
		actorchar = "sprites/wdoor_menu_head19.spr";
	}
	else if(actor == 20)
	{
		actorchar = "sprites/wdoor_menu_head20.spr";
	}
	else
	{
		actorchar = "sprites/wdoor_menu_head0.spr";
	}
}

int icons;
void GetItem_Icon(int item)
{
	if(item == 1 || item == 2 || item == 14 || item == 23)
	{
		icons = 11;
	}
	else if(item >= 3 && item <= 8)
	{
		icons = 4;
	}
	else if(item >= 9 && item <= 12)
	{
		icons = 9;
	}
	else if(item == 13)
	{
		icons = 6;
	}
	else if(item == 15)
	{
		icons = 2;
	}
	else if(item == 16)
	{
		icons = 7;
	}
	else if(item == 17)
	{
		icons = 7;
	}
	else if(item == 18)
	{
		icons = 29;
	}
	else if(item == 19)
	{
		icons = 8;
	}
	else if(item == 20)
	{
		icons = 15;
	}
	else if(item == 21)
	{
		icons = 16;
	}
	else if(item == 22)
	{
		icons = 14;
	}
	else
	{
		icons = 32;
	}
}

void GetSkill_Icon(int skill)
{
	if(skill == 2 || skill == 21 || skill == 24 || skill == 39 || skill == 40 || skill == 43 || skill == 44 || skill == 57 || skill == 60 || skill == 66)
	{
		icons = 17;
	}
	else if(skill == 3 || skill == 16 || skill == 31 || skill == 47 || skill == 54 || skill == 58 || skill == 81)
	{
		icons = 7;
	}
	else if(skill == 4 || skill == 46)
	{
		icons = 24;
	}
	else if(skill == 5 || skill == 61 || skill == 76)
	{
		icons = 3;
	}
	else if(skill == 6 || skill == 37 || skill == 48 || skill == 63)
	{
		icons = 13;
	}
	else if(skill == 7 || skill == 65)
	{
		icons = 26;
	}
	else if(skill == 8 || skill == 19 || skill == 50 || skill == 52 || skill == 56 || skill == 69 || skill == 70 || skill == 72 || skill == 62 || skill == 22)
	{
		icons = 10;
	}
	else if(skill == 41 || skill == 55 || skill == 59 || skill == 17 || skill == 82)
	{
		icons = 12;
	}
	else if(skill == 10 || skill == 77)
	{
		icons = 20;
	}
	else if(skill == 11 || skill == 14 || skill == 78)
	{
		icons = 22;
	}
	else if(skill == 12 || skill == 20 || skill == 33 || skill == 67)
	{
		icons = 29;
	}
	else if(skill == 13)
	{
		icons = 25;
	}
	else if(skill == 18 || skill == 26 || skill == 28 || skill == 30 || skill == 35 || skill == 36)
	{
		icons = 18;
	}
	else if(skill == 23 || skill == 73)
	{
		icons = 28;
	}
	else if(skill == 25 || skill == 27 || skill == 29 || skill == 80)
	{
		icons = 19;
	}
	else if(skill == 32 || skill == 45 || skill == 64)
	{
		icons = 23;
	}
	else if(skill == 34 || skill == 51 || skill == 68 || skill == 71)
	{
		icons = 15;
	}
	else if(skill == 38)
	{
		icons = 30;
	}
	else if(skill == 49)
	{
		icons = 27;
	}
	else if(skill == 53)
	{
		icons = 21;
	}
	else if(skill == 9 || skill == 74 || skill == 75 || skill == 79 || skill == 1 || skill == 42)
	{
		icons = 1;
	}
	else
	{
		icons = 32;
	}
}

char *itemchar;
int item_des;
void GetItem(int item)
{
	if(item == 1)
	{
		itemchar = "sprites/wdoor_menu_item1.spr";
		item_des = 1;
	}
	else if(item == 2)
	{
		itemchar = "sprites/wdoor_menu_item2.spr";
		item_des = 2;
	}
	else if(item == 3)
	{
		itemchar = "sprites/wdoor_menu_item3.spr";
		item_des = 3;
	}
	else if(item == 4)
	{
		itemchar = "sprites/wdoor_menu_item4.spr";
		item_des = 3;
	}
	else if(item == 5)
	{
		itemchar = "sprites/wdoor_menu_item5.spr";
		item_des = 3;
	}
	else if(item == 6)
	{
		itemchar = "sprites/wdoor_menu_item6.spr";
		item_des = 3;
	}
	else if(item == 7)
	{
		itemchar = "sprites/wdoor_menu_item7.spr";
		item_des = 3;
	}
	else if(item == 8){
		itemchar = "sprites/wdoor_menu_item8.spr";
		item_des = 3;
	}
	else if(item == 9)
	{
		itemchar = "sprites/wdoor_menu_item9.spr";
		item_des = 4;
	}
	else if(item == 10)
	{
		itemchar = "sprites/wdoor_menu_item10.spr";
		item_des = 5;
	}
	else if(item == 11)
	{
		itemchar = "sprites/wdoor_menu_item11.spr";
		item_des = 6;
	}
	else if(item == 12)
	{
		itemchar = "sprites/wdoor_menu_item12.spr";
		item_des = 7;
	}
	else if(item == 13)
	{
		itemchar = "sprites/wdoor_menu_item13.spr";
		item_des = 8;
	}
	else if(item == 14)
	{
		itemchar = "sprites/wdoor_menu_item14.spr";
		item_des = 9;
	}
	else if(item == 15)
	{
		itemchar = "sprites/wdoor_menu_item15.spr";
		item_des = 10;
	}
	else if(item == 16)
	{
		itemchar = "sprites/wdoor_menu_item16.spr";
		item_des = 11;
	}
	else if(item == 17)
	{
		itemchar = "sprites/wdoor_menu_item17.spr";
		item_des = 12;
	}
	else if(item == 18)
	{
		itemchar = "sprites/wdoor_menu_item18.spr";
		item_des = 13;
	}
	else if(item == 19)
	{
		itemchar = "sprites/wdoor_menu_item19.spr";
		item_des = 14;
	}
	else if(item == 20)
	{
		itemchar = "sprites/wdoor_menu_item20.spr";
		item_des = 15;
	}
	else if(item == 21)
	{
		itemchar = "sprites/wdoor_menu_item21.spr";
		item_des = 16;
	}
	else if(item == 22)
	{
		itemchar = "sprites/wdoor_menu_item22.spr";
		item_des = 17;
	}
	else if(item == 23)
	{
		itemchar = "sprites/wdoor_menu_item23.spr";
		item_des = 18;
	}
	else
	{
		itemchar = "sprites/wdoor_rpg_menu_short2.spr";
		item_des = 0;
	}
}

int CHudRPGMenu::Draw(float flTime)
{
	if (m_menu_on == 0)
		return 1;
	
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL )  )
		return 1;
	
	int r, g, b, x, x2, x3,y, y2,y3,a;
	int x4,y4,x5,y5,x6,xc,xe,y6,rs,skill_des;
	int r2,g2,b2;

	if (m_menu_on == 1 || m_menu_on == 4 || m_menu_on == 6)
	{
		m_spr_menu1 = LoadSprite("sprites/wdoor_rpg_menu_short_e.spr");
		
		r = 255;
		g = 255;
		b = 255;
		a = 255;

	//	x = -600;
	//	x2 = x + (600 * 0.02 * m_menu_origin);
		x2 = 0;
		if(m_actor1 == 0)
		{
			x2 = -200;
		}

		y = (ScreenHeight - 600) * 0.5;
		if(y < 0)
		{
			y = 0;
		}
		SPR_Set(m_spr_menu1, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		m_spr_menu2 = LoadSprite("sprites/wdoor_rpg_menu_long.spr");
		x2 += 200;
		SPR_Set(m_spr_menu2, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);
		
		GetActor(m_actor1);
		m_spr_actor1 = LoadSprite(actorchar);
		//m_spr_actor1 = LoadSprite("sprites/wdoor_menu_head1.spr");
		SPR_Set(m_spr_actor1, r, g, b );
		y3 = y + 10;
		x3 = x2 + 16;
		SPR_DrawHoles( 0,  x3, y3, NULL);

		if(m_actor1 == 1)
		{
			//HP
			x4 = x3 + 105;
			y4 = y3 + 10;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive( m_actor1,  x4, y4, NULL);

			x4 = x3 + 102;
			y4 = y3 + 45;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4, y4, NULL);

			x4 = x3 + 100;
			y4 = y3 + 80;
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive( 1,  x4, y4, NULL);

			x4 = x3 + 256;
			y4 = y3 + 10;
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive( 2,  x4, y4, NULL);
			gHUD.DrawHudNumber(x4+48, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_level1, 255, 255, 255);

			x4 = x3 + 150;
			y4 = y3 + 50;
			m_spr_actor1_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor1_maxhp, 96, 96, 96 );
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_hp1 > 0)
			{
				m_prc1_hp->right = m_hp1;
				if(m_hp1 <= 40)
				{
					SPR_Set(m_spr_actor1_hp, 255, 32, 32 );
				}
				else if(m_hp1 <= 100)
				{
					SPR_Set(m_spr_actor1_hp, 255, 160, 32 );
				}
				else
				{
					SPR_Set(m_spr_actor1_hp, 32, 255, 32 );
				}
				SPR_DrawAdditive( 0,  x4, y4, m_prc1_hp);
			}

			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_numhp1, 255, 255, 255);
			m_spr_hspr = LoadSprite("sprites/l.spr");
			SPR_Set(m_spr_hspr, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4+104, y4, NULL);
			gHUD.DrawHudNumberLarge(x4+96, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxhp1, 255, 255, 255);

			m_spr_actor1_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor1_maxhp, 96, 96, 96 );
			y4 = y3 + 85;
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_exp1 > 0){
			m_prc1_exp->right = m_exp1;
			SPR_Set(m_spr_actor1_hp, 255, 255, 32 );
			SPR_DrawAdditive( 0,  x4, y4, m_prc1_exp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxexp1, 255, 255, 255);
		}

		if(m_actor2 != 0)
		{
			GetActor(m_actor2);
			m_spr_actor2 = LoadSprite(actorchar);
			
			SPR_Set(m_spr_actor2, r, g, b );
			y3 = y + 128;
			x3 = x2 + 16;
			SPR_DrawHoles( 0,  x3, y3, NULL);

			//HP
			x4 = x3 + 105;
			y4 = y3 + 10;
			m_spr_actor2_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor2_status, 255, 255, 255 );
			SPR_DrawAdditive( m_actor2,  x4, y4, NULL);

			x4 = x3 + 102;
			y4 = y3 + 45;
			m_spr_actor2_status = LoadSprite("sprites/wdoor_menutext.spr");
			SPR_Set(m_spr_actor2_status, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4, y4, NULL);

			x4 = x3 + 100;
			y4 = y3 + 80;
			SPR_Set(m_spr_actor2_status, 255, 255, 255 );
			SPR_DrawAdditive( 1,  x4, y4, NULL);

			x4 = x3 + 256;
			y4 = y3 + 10;
			SPR_Set(m_spr_actor2_status, 255, 255, 255 );
			SPR_DrawAdditive( 2,  x4, y4, NULL);
			gHUD.DrawHudNumber(x4+48, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_level2, 255, 255, 255);

			x4 = x3 + 150;
			y4 = y3 + 50;
			m_spr_actor2_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor2_maxhp, 96, 96, 96 );
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_hp2 > 0)
			{
				m_prc2_hp->right = m_hp2;
				if(m_hp2 <= 40)
				{
					SPR_Set(m_spr_actor2_hp, 255, 32, 32 );
				}
				else if(m_hp2 <= 100)
				{
					SPR_Set(m_spr_actor2_hp, 255, 160, 32 );
				}
				else
				{
					SPR_Set(m_spr_actor2_hp, 32, 255, 32 );
				}
				SPR_DrawAdditive( 0,  x4, y4, m_prc2_hp);
			}

			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_numhp2, 255, 255, 255);
			m_spr_hspr = LoadSprite("sprites/l.spr");
			SPR_Set(m_spr_hspr, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4+104, y4, NULL);
			gHUD.DrawHudNumberLarge(x4+96, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxhp2, 255, 255, 255);

			m_spr_actor2_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor2_maxhp, 96, 96, 96 );
			y4 = y3 + 85;
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_exp2 > 0)
			{
				m_prc2_exp->right = m_exp2;
				SPR_Set(m_spr_actor2_hp, 255, 255, 32 );
				SPR_DrawAdditive( 0,  x4, y4, m_prc2_exp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxexp2, 255, 255, 255);

		}
	
		if(m_actor3 != 0)
		{
			GetActor(m_actor3);
			m_spr_actor3 = LoadSprite(actorchar);
			
			SPR_Set(m_spr_actor3, r, g, b );
			y3 = y + 246;
			x3 = x2 + 16;
			SPR_DrawHoles( 0,  x3, y3, NULL);

			//HP
			x4 = x3 + 105;
			y4 = y3 + 10;//����
			m_spr_actor3_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor3_status, 255, 255, 255 );
			SPR_DrawAdditive( m_actor3,  x4, y4, NULL);

			x4 = x3 + 102;
			y4 = y3 + 45;
			m_spr_actor3_status = LoadSprite("sprites/wdoor_menutext.spr");
			SPR_Set(m_spr_actor3_status, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4, y4, NULL);

			x4 = x3 + 100;
			y4 = y3 + 80;
			SPR_Set(m_spr_actor3_status, 255, 255, 255 );
			SPR_DrawAdditive( 1,  x4, y4, NULL);

			x4 = x3 + 256;
			y4 = y3 + 10;
			SPR_Set(m_spr_actor3_status, 255, 255, 255 );
			SPR_DrawAdditive( 2,  x4, y4, NULL);
			gHUD.DrawHudNumber(x4+48, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_level3, 255, 255, 255);

			x4 = x3 + 150;
			y4 = y3 + 50;
			m_spr_actor3_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor3_maxhp, 96, 96, 96 );
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_hp3 > 0)
			{
				m_prc3_hp->right = m_hp3;
				if(m_hp3 <= 40)
				{
					SPR_Set(m_spr_actor3_hp, 255, 32, 32 );
				}
				else if(m_hp3 <= 100)
				{
					SPR_Set(m_spr_actor3_hp, 255, 160, 32 );
				}
				else
				{
					SPR_Set(m_spr_actor3_hp, 32, 255, 32 );
				}
				SPR_DrawAdditive( 0,  x4, y4, m_prc3_hp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_numhp3, 255, 255, 255);
			m_spr_hspr = LoadSprite("sprites/l.spr");
			SPR_Set(m_spr_hspr, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4+104, y4, NULL);
			gHUD.DrawHudNumberLarge(x4+96, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxhp3, 255, 255, 255);

			m_spr_actor3_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor3_maxhp, 96, 96, 96 );
			y4 = y3 + 85;
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_exp3 > 0)
			{
				m_prc3_exp->right = m_exp3;
				SPR_Set(m_spr_actor3_hp, 255, 255, 32 );
				SPR_DrawAdditive( 0,  x4, y4, m_prc3_exp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxexp3, 255, 255, 255);
		}

		if(m_actor4 != 0)
		{
			GetActor(m_actor4);
			m_spr_actor4 = LoadSprite(actorchar);
			
			SPR_Set(m_spr_actor4, r, g, b );
			y3 = y + 364;
			x3 = x2 + 16;
			SPR_DrawHoles( 0,  x3, y3, NULL);

			//HP
			x4 = x3 + 105;
			y4 = y3 + 10;
			m_spr_actor4_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor4_status, 255, 255, 255 );
			SPR_DrawAdditive( m_actor4,  x4, y4, NULL);

			x4 = x3 + 102;
			y4 = y3 + 45;
			m_spr_actor4_status = LoadSprite("sprites/wdoor_menutext.spr");
			SPR_Set(m_spr_actor4_status, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4, y4, NULL);

			x4 = x3 + 100;
			y4 = y3 + 80;
			SPR_Set(m_spr_actor4_status, 255, 255, 255 );
			SPR_DrawAdditive( 1,  x4, y4, NULL);

			x4 = x3 + 256;
			y4 = y3 + 10;
			SPR_Set(m_spr_actor4_status, 255, 255, 255 );
			SPR_DrawAdditive( 2,  x4, y4, NULL);
			gHUD.DrawHudNumber(x4+48, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_level4, 255, 255, 255);

			x4 = x3 + 150;
			y4 = y3 + 50;
			m_spr_actor4_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor4_maxhp, 96, 96, 96 );
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_hp4 > 0)
			{
				m_prc4_hp->right = m_hp4;
				if(m_hp4 <= 40)
				{
					SPR_Set(m_spr_actor4_hp, 255, 32, 32 );
				}
				else if(m_hp4 <= 100)
				{
					SPR_Set(m_spr_actor4_hp, 255, 160, 32 );
				}
				else
				{
					SPR_Set(m_spr_actor4_hp, 32, 255, 32 );
				}
				SPR_DrawAdditive( 0,  x4, y4, m_prc4_hp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_numhp4, 255, 255, 255);
			m_spr_hspr = LoadSprite("sprites/l.spr");
			SPR_Set(m_spr_hspr, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4+104, y4, NULL);
			gHUD.DrawHudNumberLarge(x4+96, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxhp4, 255, 255, 255);

			m_spr_actor4_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor4_maxhp, 96, 96, 96 );
			y4 = y3 + 85;
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_exp4 > 0)
			{
				m_prc4_exp->right = m_exp4;
				SPR_Set(m_spr_actor4_hp, 255, 255, 32 );
				SPR_DrawAdditive( 0,  x4, y4, m_prc4_exp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxexp4, 255, 255, 255);
		}

		if(m_actor5 != 0)
		{
			GetActor(m_actor5);
			m_spr_actor5 = LoadSprite(actorchar);
			
			SPR_Set(m_spr_actor5, r, g, b );
			y3 = y + 482;
			x3 = x2 + 16;
			SPR_DrawHoles( 0,  x3, y3, NULL);

			//HP
			x4 = x3 + 105;
			y4 = y3 + 10;
			m_spr_actor5_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor5_status, 255, 255, 255 );
			SPR_DrawAdditive( m_actor5,  x4, y4, NULL);

			x4 = x3 + 102;
			y4 = y3 + 45;
			m_spr_actor5_status = LoadSprite("sprites/wdoor_menutext.spr");
			SPR_Set(m_spr_actor5_status, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4, y4, NULL);

			x4 = x3 + 100;
			y4 = y3 + 80;
			SPR_Set(m_spr_actor5_status, 255, 255, 255 );
			SPR_DrawAdditive( 1,  x4, y4, NULL);

			x4 = x3 + 256;
			y4 = y3 + 10;
			SPR_Set(m_spr_actor5_status, 255, 255, 255 );
			SPR_DrawAdditive( 2,  x4, y4, NULL);
			gHUD.DrawHudNumber(x4+48, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_level5, 255, 255, 255);

			x4 = x3 + 150;
			y4 = y3 + 50;
			m_spr_actor5_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor5_maxhp, 96, 96, 96 );
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_hp5 > 0)
			{
				m_prc5_hp->right = m_hp5;
				if(m_hp5 <= 40)
				{
					SPR_Set(m_spr_actor5_hp, 255, 32, 32 );
				}
				else if(m_hp5 <= 100)
				{
					SPR_Set(m_spr_actor5_hp, 255, 160, 32 );
				}
				else
				{
					SPR_Set(m_spr_actor5_hp, 32, 255, 32 );
				}
				SPR_DrawAdditive( 0,  x4, y4, m_prc5_hp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_numhp5, 255, 255, 255);
			m_spr_hspr = LoadSprite("sprites/l.spr");
			SPR_Set(m_spr_hspr, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x4+104, y4, NULL);
			gHUD.DrawHudNumberLarge(x4+96, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxhp5, 255, 255, 255);

			m_spr_actor5_maxhp = LoadSprite("sprites/healthcheck_m.spr");
			SPR_Set(m_spr_actor5_maxhp, 96, 96, 96 );
			y4 = y3 + 85;
			SPR_DrawHoles( 0,  x4, y4, NULL);

			if(m_exp5 > 0)
			{
				m_prc5_exp->right = m_exp5;
				SPR_Set(m_spr_actor5_hp, 255, 255, 32 );
				SPR_DrawAdditive( 0,  x4, y4, m_prc5_exp);
			}
			gHUD.DrawHudNumberLarge(x4, y4, DHN_3DIGITS | DHN_DRAWZERO, m_maxexp5, 255, 255, 255);

		}

		if(m_menu_on == 4)
		{
			m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select2.spr");
			x2 += 16;
			y2 = y + 10 + (118 * m_menu_select);
			SPR_Set(m_spr_menu_select, 192, 192, 64 );
			SPR_DrawAdditive( 0,  x2, y2, NULL);

			m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select1.spr");
			x2 -= 216;
			y2 = y + 64;
			SPR_Set(m_spr_menu_select, 255, 255, 0 );
			SPR_DrawHoles( 0,  x2, y2, NULL);
		}
		else if(m_menu_on == 6)
		{
			m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select2.spr");
			x2 += 16;
			y2 = y + 10 + (118 * m_menu_select);
			SPR_Set(m_spr_menu_select, 192, 192, 64 );
			SPR_DrawAdditive( 0,  x2, y2, NULL);

			m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select1.spr");
			x2 -= 216;
			y2 = y + 128;
			SPR_Set(m_spr_menu_select, 255, 255, 0 );
			SPR_DrawHoles( 0,  x2, y2, NULL);
		}
		else
		{
			m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select1.spr");
			x2 -= 200;
			y2 = y + (64 * m_menu_select);
			SPR_Set(m_spr_menu_select, 255, 255, 0 );
			SPR_DrawHoles( 0,  x2, y2, NULL);
		}

	}
	else if (m_menu_on == 2 || m_menu_on == 3)
	{
		if(m_menu_on == 3)
		{
			rs = m_menu_select;
			m_menu_select = m_item_s;
		}

		if(m_menu_select == 0)
		{
			GetItem(m_item1);
		}
		else if(m_menu_select == 1)
		{
			GetItem(m_item2);
		}
		else if(m_menu_select == 2)
		{
			GetItem(m_item3);
		}
		else if(m_menu_select == 3)
		{
			GetItem(m_item4);
		}
		else if(m_menu_select == 4)
		{
			GetItem(m_item5);
		}
		else if(m_menu_select == 5)
		{
			GetItem(m_item6);
		}
		else if(m_menu_select == 6)
		{
			GetItem(m_item7);
		}
		else if(m_menu_select == 7)
		{
			GetItem(m_item8);
		}
		else if(m_menu_select == 8)
		{
			GetItem(m_item9);
		}
		else if(m_menu_select == 9)
		{
			GetItem(m_item10);
		}
		else if(m_menu_select == 10)
		{
			GetItem(m_item11);
		}
		else if(m_menu_select == 11)
		{
			GetItem(m_item12);
		}

		m_spr_menu1 = LoadSprite(itemchar);
		r = 255;
		g = 255;
		b = 255;
		a = 255;

		x = -600;
		x2 = 0;

		y = (ScreenHeight - 600) * 0.5;
		if(y < 0)
		{
			y = 0;
		}

		x5 = x2;
		y5 = y;

		SPR_Set(m_spr_menu1, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		m_spr_menu2 = LoadSprite("sprites/wdoor_rpg_menu_long.spr");
		x2 += 200;
		SPR_Set(m_spr_menu2, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		m_spr_menu_icons = LoadSprite("sprites/wdoor_menutext_icons.spr");
		m_spr_menu_icons_drk = LoadSprite("sprites/wdoor_menutext_icons_drk.spr");
		m_spr_menu_itemtext = LoadSprite("sprites/wdoor_menutext_items_e.spr");

		xe = x2;
		xc = x2+24;
		x2 += 56;//20+32+12
		y += 25;

		m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select3.spr");
		x6 = x2 - 48;
		y2 = y + (m_menu_select * 36);
		SPR_Set(m_spr_menu_select, 255, 255, 0 );
		SPR_DrawHoles( 0,  x6, y2, NULL);

		if(m_item1 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item1,  x2, y, NULL);

			GetItem_Icon(m_item1);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 0)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item1 == 1 || m_item1 == 2 || m_item1 == 15 || m_item1 == 16 || m_item1 == 18 || m_item1 == 19 || m_item1 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item2 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item2,  x2, y, NULL);

			GetItem_Icon(m_item2);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 1)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item2 == 1 || m_item2 == 2 || m_item2 == 15 || m_item2 == 16 || m_item2 == 18 || m_item2 == 19 || m_item2 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item3 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item3,  x2, y, NULL);

			GetItem_Icon(m_item3);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 2){
			SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item3 == 1 || m_item3 == 2 || m_item3 == 15 || m_item3 == 16 || m_item3 == 18 || m_item3 == 19 || m_item3 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item4 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item4,  x2, y, NULL);

			GetItem_Icon(m_item4);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 3)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item4 == 1 || m_item4 == 2 || m_item4 == 15 || m_item4 == 16 || m_item4 == 18 || m_item4 == 19 || m_item4 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item5 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item5,  x2, y, NULL);

			GetItem_Icon(m_item5);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 4)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item5 == 1 || m_item5 == 2 || m_item5 == 15 || m_item5 == 16 || m_item5 == 18 || m_item5 == 19 || m_item5 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item6 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item6,  x2, y, NULL);

			GetItem_Icon(m_item6);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 5)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item6 == 1 || m_item6 == 2 || m_item6 == 15 || m_item6 == 16 || m_item6 == 18 || m_item6 == 19 || m_item6 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item7 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item7,  x2, y, NULL);

			GetItem_Icon(m_item7);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 6)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item7 == 1 || m_item7 == 2 || m_item7 == 15 || m_item7 == 16 || m_item7 == 18 || m_item7 == 19 || m_item7 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item8 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item8,  x2, y, NULL);

			GetItem_Icon(m_item8);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 7){
			SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item8 == 1 || m_item8 == 2 || m_item8 == 15 || m_item8 == 16 || m_item8 == 18 || m_item8 == 19 || m_item8 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item9 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item9,  x2, y, NULL);

			GetItem_Icon(m_item9);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 8){
			SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item9 == 1 || m_item9 == 2 || m_item9 == 15 || m_item9 == 16 || m_item9 == 18 || m_item9 == 19 || m_item9 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item10 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item10,  x2, y, NULL);

			GetItem_Icon(m_item10);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 9)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item10 == 1 || m_item10 == 2 || m_item10 == 15 || m_item10 == 16 || m_item10 == 18 || m_item10 == 19 || m_item10 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item11 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item11,  x2, y, NULL);

			GetItem_Icon(m_item11);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 10)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item11 == 1 || m_item11 == 2 || m_item11 == 15 || m_item11 == 16 || m_item11 == 18 || m_item11 == 19 || m_item11 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}
		y += 36;
		if(m_item12 != 0)
		{
			SPR_Set(m_spr_menu_itemtext, r, g, b );
			SPR_DrawAdditive( m_item12,  x2, y, NULL);

			GetItem_Icon(m_item12);
			SPR_Set(m_spr_menu_icons_drk, r, g, b );
			SPR_DrawHoles( icons,  xc, y, NULL);
			SPR_Set(m_spr_menu_icons, r, g, b );
			SPR_DrawAdditive( icons,  xc, y, NULL);

			if(m_item_e == 11)
			{
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}

			if(m_item12 == 1 || m_item12 == 2 || m_item12 == 15 || m_item12 == 16 || m_item12 == 18 || m_item12 == 19 || m_item12 == 20)
			{
				SPR_Set(m_spr_menu_icons, 255, 255, 0 );
				SPR_DrawAdditive( 31,  xe, y, NULL);
			}
		}

		if (m_menu_on == 3)
		{
			m_spr_menu5 = LoadSprite("sprites/wdoor_rpg_menu_short3_e.spr");
			
			SPR_Set(m_spr_menu5, r, g, b );
			y5 += 250;
			SPR_DrawHoles( 0,  x5, y5, NULL);

			m_spr_menu_select2 = LoadSprite("sprites/wdoor_rpg_menu_select1.spr");
			SPR_Set(m_spr_menu_select2, r, g, b );
			y5 += rs * 64;
			SPR_DrawHoles( 0,  x5, y5, NULL);
		}

		m_spr_menu3 = LoadSprite("sprites/wdoor_rpg_menu_long2.spr");
		x2 = 0;
		y = (ScreenHeight - 600) * 0.5 + 472;
		SPR_Set(m_spr_menu3, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		if(item_des != 0)
		{
			m_spr_menu4 = LoadSprite("sprites/wdoor_menutext_items_de_e.spr");
			
			y += 4;
			SPR_Set(m_spr_menu4, r, g, b );
			SPR_DrawAdditive( item_des - 1,  x2, y, NULL);
		}
	}
	else if (m_menu_on == 5)
	{
		m_spr_menu1 = LoadSprite("sprites/wdoor_rpg_menu_short2.spr");
		r = 255;
		g = 255;
		b = 255;
		a = 255;

		x = -600;
		x2 = 0;

		y = (ScreenHeight - 600) * 0.5;
		if(y < 0)
		{
			y = 0;
		}

		x5 = x2;
		y5 = y;

		SPR_Set(m_spr_menu1, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		GetActor(m_skill0);
		m_spr_actor1 = LoadSprite(actorchar);

		SPR_Set(m_spr_actor1, r, g, b );
		y3 = y + 20;
		x3 = x2 + 48;
		SPR_DrawHoles( 0,  x3, y3, NULL);

		x4 = x3 + 5;
		y4 = y3 + 112;
		m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive(m_skill0,  x4, y4, NULL);

		m_spr_menu2 = LoadSprite("sprites/wdoor_rpg_menu_long.spr");
		x2 += 200;
		SPR_Set(m_spr_menu2, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		m_spr_menu_icons = LoadSprite("sprites/wdoor_menutext_icons.spr");
		m_spr_menu_icons_drk = LoadSprite("sprites/wdoor_menutext_icons_drk.spr");
		m_spr_menu_itemtext = LoadSprite("sprites/wdoor_menutext_skills_e.spr");
		
		xe = x2;
		xc = x2+24;
		x6 = x2 + 56;
		y6 = y + 25;

		GetSkill_Icon(m_skill1);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill1,  x6, y6, NULL);

		GetSkill_Icon(m_skill2);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+36, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+36, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill2,  x6, y6+36, NULL);

		GetSkill_Icon(m_skill3);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+72, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+72, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill3,  x6, y6+72, NULL);

		GetSkill_Icon(m_skill4);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+108, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+108, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill4,  x6, y6+108, NULL);

		GetSkill_Icon(m_skill5);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+144, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+144, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill5,  x6, y6+144, NULL);

		GetSkill_Icon(m_skill6);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+180, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+180, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill6,  x6, y6+180, NULL);

		GetSkill_Icon(m_skill7);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+216, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+216, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill7,  x6, y6+216, NULL);

		GetSkill_Icon(m_skill8);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+252, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+252, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill8,  x6, y6+252, NULL);

		GetSkill_Icon(m_skill9);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+288, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+288, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill9,  x6, y6+288, NULL);

		GetSkill_Icon(m_skill10);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+324, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+324, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill10,  x6, y6+324, NULL);

		GetSkill_Icon(m_skill11);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+360, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+360, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill11,  x6, y6+360, NULL);

		GetSkill_Icon(m_skill12);
		SPR_Set(m_spr_menu_icons_drk, r, g, b );
		SPR_DrawHoles( icons,  xc, y6+396, NULL);
		SPR_Set(m_spr_menu_icons, r, g, b );
		SPR_DrawAdditive( icons,  xc, y6+396, NULL);
		SPR_Set(m_spr_menu_itemtext, r, g, b );
		SPR_DrawAdditive( m_skill12,  x6, y6+396, NULL);

		if(m_menu_select == 0)
		{
			skill_des = m_skill1;
		}
		else if(m_menu_select == 1)
		{
			skill_des = m_skill2;
		}
		else if(m_menu_select == 2)
		{
			skill_des = m_skill3;
		}
		else if(m_menu_select == 3)
		{
			skill_des = m_skill4;
		}
		else if(m_menu_select == 4)
		{
			skill_des = m_skill5;
		}
		else if(m_menu_select == 5)
		{
			skill_des = m_skill6;
		}
		else if(m_menu_select == 6)
		{
			skill_des = m_skill7;
		}
		else if(m_menu_select == 7)
		{
			skill_des = m_skill8;
		}
		else if(m_menu_select == 8)
		{
			skill_des = m_skill9;
		}
		else if(m_menu_select == 9)
		{
			skill_des = m_skill10;
		}
		else if(m_menu_select == 10)
		{
			skill_des = m_skill11;
		}
		else if(m_menu_select == 11)
		{
			skill_des = m_skill12;
		}

		m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select3.spr");
		x2 -= 10;
		y2 = y + 24 + (m_menu_select * 36);
		SPR_Set(m_spr_menu_select, 255, 255, 0 );
		SPR_DrawHoles( 0,  x2 + 20, y2, NULL);

		m_spr_menu3 = LoadSprite("sprites/wdoor_rpg_menu_long2.spr");
		x2 = 0;
		y = (ScreenHeight - 600) * 0.5 + 472;
		SPR_Set(m_spr_menu3, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		m_spr_menu4 = LoadSprite("sprites/wdoor_menutext_skills_de_e.spr");
		
		y += 4;
		SPR_Set(m_spr_menu4, r, g, b );
		SPR_DrawAdditive( skill_des,  x2, y, NULL);
	}
	else if (m_menu_on == 7)
	{
		m_spr_menu1 = LoadSprite("sprites/wdoor_rpg_menu_long3.spr");
		r = 255;
		g = 255;
		b = 255;
		a = 255;

		r2 = 255;
		g2 = 255;
		b2 = 255;

		x2 = 0;
		y = (ScreenHeight - 600) * 0.5;
		if(y < 0)
		{
			y = 0;
		}

		SPR_Set(m_spr_menu1, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		x3 = x2 + 32;
		y3 = y + 16;
		y5 = y3;

		if(m_skill1 != 0)
		{
			if(m_skill1 > 100)
			{
				m_skill1 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill1);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill1,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill2 != 0)
		{
			if(m_skill2 > 100)
			{
				m_skill2 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill2);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill2,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill3 != 0)
		{
			if(m_skill3 > 100)
			{
				m_skill3 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill3);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill3,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill4 != 0)
		{
			if(m_skill4 > 100)
			{
				m_skill4 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill4);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill4,  x3, y4, NULL);
		}
		x3 -= 360;
		y3 += 180;
		if(m_skill5 != 0)
		{
			if(m_skill5 > 100)
			{
				m_skill5 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill5);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill5,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill6 != 0)
		{
			if(m_skill6 > 100)
			{
				m_skill6 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill6);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill6,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill7 != 0)
		{
			if(m_skill7 > 100)
			{
				m_skill7 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill7);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill7,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill8 != 0)
		{
			if(m_skill8 > 100)
			{
				m_skill8 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill8);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill8,  x3, y4, NULL);
		}
		x3 -= 360;
		y3 += 160;
		if(m_skill9 != 0)
		{
			if(m_skill9 > 100)
			{
				m_skill9 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill9);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill9,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill10 != 0)
		{
			if(m_skill10 > 100)
			{
				m_skill10 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill10);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill10,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill11 != 0)
		{
			if(m_skill11 > 100)
			{
				m_skill11 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill11);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill11,  x3, y4, NULL);
		}
		x3 += 120;
		if(m_skill12 != 0)
		{
			if(m_skill12 > 100)
			{
				m_skill12 -= 100;
				g2 = 0;
				b2 = 0;
			}
			else
			{
				g2 = 255;
				b2 = 255;
			}
			GetActor(m_skill12);
			m_spr_actor1 = LoadSprite(actorchar);
			SPR_Set(m_spr_actor1, r2, g2, b2 );
			SPR_DrawHoles( 0,  x3, y3, NULL);
			y4 = y3 + 112;
			m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_name.spr");
			SPR_Set(m_spr_actor1_status, 255, 255, 255 );
			SPR_DrawAdditive(m_skill12,  x3, y4, NULL);
		}
		
		m_spr_menu_select = LoadSprite("sprites/wdoor_rpg_menu_select4.spr");
		
		if(m_menu_select >= 9)
		{
			x3 = 32 + (m_menu_select - 8) * 120 - 120;
			y4 = y5+340;
		}
		else if(m_menu_select >= 5)
		{
			x3 = 32 + (m_menu_select - 4) * 120 - 120;
			y4 = y5+180;
		}
		else if(m_menu_select >= 1)
			{
			x3 = 32 + m_menu_select * 120 - 120;
			y4 = y5;
		}
		SPR_Set(m_spr_menu_select, 255, 255, 0 );
		SPR_DrawAdditive( 0,  x3, y4, NULL);

		if(m_skill0 >= 1)
		{
			if(m_skill0 >= 9)
			{
				x3 = 32 + (m_skill0 - 8) * 120 - 120;
				y4 = y5+340;
			}
			else if(m_skill0 >= 5)
			{
				x3 = 32 + (m_skill0 - 4) * 120 - 120;
				y4 = y5+180;
			}
			else if(m_skill0 >= 1)
			{
				x3 = 32 + m_skill0 * 120 - 120;
				y4 = y5;
			}
			SPR_Set(m_spr_menu_select, 255, 255, 255 );
			SPR_DrawAdditive( 0,  x3, y4, NULL);
		}
	}
	else if (m_menu_on == 8)
	{
		m_spr_menu1 = LoadSprite("sprites/wdoor_rpg_menu_long.spr");
		r = 255;
		g = 255;
		b = 255;
		a = 255;

		r2 = 255;
		g2 = 255;
		b2 = 255;

		x2 = 0;
		y = (ScreenHeight - 600) * 0.5;
		if(y < 0)
		{
			y = 0;
		}

		SPR_Set(m_spr_menu1, r, g, b );
		SPR_DrawHoles( 0,  x2, y, NULL);

		x4 = x2 + 32;
		y4 = y + 32;
		m_spr_actor1_status = LoadSprite("sprites/wdoor_menutext_game_e.spr");
	
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive( 0,  x4, y4, NULL);

		gHUD.DrawHudNumber(x4+64, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill1, 255, 255, 255);//ʱ
		gHUD.DrawHudNumber(x4+134, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill2, 255, 255, 255);//��
		gHUD.DrawHudNumber(x4+204, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill3, 255, 255, 255);//��

		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		y4 += 64;
		SPR_DrawAdditive( 1,  x4, y4, NULL);
		if(m_skill4 == 3)
		{
			SPR_Set(m_spr_actor1_status, 255, 0, 0 );
			SPR_DrawAdditive( 4,  x4 + 128, y4, NULL);
			SPR_DrawAdditive( 4,  x4 + 128, y4, NULL);
		}
		else if(m_skill4 == 2)
		{
			SPR_Set(m_spr_actor1_status, 255, 255, 0 );
			SPR_DrawAdditive( 3,  x4 + 128, y4, NULL);
		}
		else
		{
			SPR_Set(m_spr_actor1_status, 0, 255, 0 );
			SPR_DrawAdditive( 2,  x4 + 128, y4, NULL);
		}

		y4 += 64;
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive( 5,  x4, y4, NULL);
		gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill5, 255, 255, 255);
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive( 9,  x4+160, y4, NULL);

		y4 += 64;
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive( 8,  x4, y4, NULL);
		if(m_skill8 <= 20)
		{
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, 0, 255, 0, 0);
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, 0, 255, 0, 0);
		}
		else if(m_skill8 >= 100)
		{
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, 100, 32, 255, 32);
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, 100, 32, 255, 32);
		}
		else if(m_skill8 >= 80)
		{
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill8, 255, 255, 0);
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill8, 255, 255, 0);
		}
		else
		{
			gHUD.DrawHudNumber(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill8, 255, 255, 255);
		}
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive( 9,  x4+160, y4, NULL);

		y4 += 64;
		SPR_DrawAdditive( 6,  x4, y4, NULL);
		gHUD.DrawHudNumberLarge(x4+96, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill6, 255, 255, 255);

		y4 += 64;
		SPR_Set(m_spr_actor1_status, 255, 255, 255 );
		SPR_DrawAdditive( 7,  x4, y4, NULL);
		if(m_skill7 >= 18)
		{
			gHUD.DrawHudNumber(x4+128, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill7, 255, 255, 0);
			gHUD.DrawHudNumber(x4+128, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill7, 255, 255, 0);
		}
		else
		{
			gHUD.DrawHudNumber(x4+128, y4+4, DHN_3DIGITS | DHN_DRAWZERO, m_skill7, 255, 255, 255);
		}
	}

	return 1;
}