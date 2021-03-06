/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef GAME_CLIENT_UI_HUD_CHUDMENU_H
#define GAME_CLIENT_UI_HUD_CHUDMENU_H

class CHudMenu : public CHudBase
{
private:
	static const size_t MAX_MENU_STRING = 512;

public:
	bool Init() override;
	void InitHUDData() override;
	bool VidInit() override;
	void Reset()  override;
	bool Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	bool m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	bool m_fWaitingForMore;

private:
	char m_szMenuString[ MAX_MENU_STRING ] = {};
	char m_szPrelocalisedMenuString[ MAX_MENU_STRING ] = {};
};

#endif //GAME_CLIENT_UI_HUD_CHUDMENU_H