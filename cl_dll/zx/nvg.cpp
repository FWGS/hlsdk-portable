#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"

DECLARE_MESSAGE(m_NVG, NVGActivate )
DECLARE_MESSAGE(m_NVG, NVG )

int CHudNVG::Init(void)
{
    HOOK_MESSAGE( NVGActivate );
    HOOK_MESSAGE( NVG );

    m_iNVG = 0;
    m_iOn = 0;
    m_flBattery = 0;

    m_iFlags = 0;
    gHUD.AddHudElem(this);

    srand( (unsigned)time( NULL ) );

    return 1;
}

int CHudNVG::VidInit(void)
{
    // this is the NVG effect sprite
    m_hFlicker = LoadSprite("sprites/nvg.spr");

    // get the battery sprites
    int HUD_battery_full = gHUD.GetSpriteIndex( "nvg_battery_full" );
    int HUD_battery_empty = gHUD.GetSpriteIndex( "nvg_battery_empty" );

    m_hBatteryFull = gHUD.GetSprite(HUD_battery_full);
    m_hBatteryEmpty = gHUD.GetSprite(HUD_battery_empty);
    m_prcBatteryFull = &gHUD.GetSpriteRect(HUD_battery_full);
    m_prcBatteryEmpty = &gHUD.GetSpriteRect(HUD_battery_empty);

    m_iWidth = m_prcBatteryFull->right - m_prcBatteryFull->left;

    // calculate the y offset (using the flashlight sprite)
    int HUD_flashlight = gHUD.GetSpriteIndex( "flash_empty" );
    wrect_t *prcFL;

    prcFL = &gHUD.GetSpriteRect( HUD_flashlight );
    m_iBatteryY = (prcFL->bottom - prcFL->top)*2;

    return 1;
};

int CHudNVG::Draw(float fTime)
{
    // draw the battery
    if (m_iNVG) {
        int r, g, b, x, y, a, offset;
        wrect_t rc;

        // updates the battery state, calculate colors
        if (m_iOn) {
            m_flBattery -= gHUD.m_flTimeDelta * NVG_DRAIN_PER_SECOND;

            if (m_flBattery < 0)
                m_flBattery = 0;
            a = 255;
        } else {
            m_flBattery += gHUD.m_flTimeDelta * NVG_RECHARGE_PER_SECOND;
            if (m_flBattery > 100)
                m_flBattery = 100;

            a = MIN_ALPHA;
        }

        if (m_flBattery < 20)
            UnpackRGB(r,g,b, RGB_REDISH);
        else
            UnpackRGB(r,g,b, RGB_YELLOWISH);

        ScaleColors(r, g, b, a);

        // calculate x/y offsets, empty width
        x = ScreenWidth - m_iWidth - m_iWidth / 2;
        y = m_iBatteryY;

        offset = (m_flBattery * m_iWidth) / 100;

        // draw the empty part first...
        if (offset < m_iWidth) {
            SPR_Set(m_hBatteryEmpty, r, g, b);

            rc = *m_prcBatteryEmpty;
            rc.right -= offset;
            SPR_DrawAdditive( 0, x, y, &rc);
        }

        // and then the full part
        if (offset > 0) {
            SPR_Set(m_hBatteryFull, r, g, b);

            rc = *m_prcBatteryFull;
            rc.left += (m_iWidth - offset);
            SPR_DrawAdditive( 0, x + m_iWidth - offset, y, &rc);
        }
    }

    if (m_iOn)
    {
        int x, y, w, h;
        int frame;

        SPR_Set(m_hFlicker, 10, 100, 10 );

        // play at 15fps
        frame = (int)(fTime * 15) % SPR_Frames(m_hFlicker);

        w = SPR_Width(m_hFlicker,0);
        h = SPR_Height(m_hFlicker,0);
        
        for(y = -(rand() % h); y < ScreenHeight; y += h) {
            for(x = -(rand() % w); x < ScreenWidth; x += w) {
                SPR_DrawAdditive( frame,  x, y, NULL );
            }
        }
    }

    return 1;
}

int CHudNVG::MsgFunc_NVG(const char *pszName,  int iSize, void *pbuf)
{
    BEGIN_READ( pbuf, iSize );

    // update NVG data
    m_iNVG = READ_BYTE();
    m_flBattery = (float)READ_BYTE();

    if (m_iNVG)
        m_iFlags |= HUD_ACTIVE;
    else
        m_iFlags &= ~HUD_ACTIVE;

    m_iOn = 0;

    return 1;
}

int CHudNVG::MsgFunc_NVGActivate(const char *pszName,  int iSize, void *pbuf)
{
    BEGIN_READ( pbuf, iSize );

    // update NVG data
    m_iOn = READ_BYTE();
    m_flBattery = (float)READ_BYTE();

    return 1;
}


