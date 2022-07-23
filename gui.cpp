#include "../main.h"

#include "gui.h"

#include "../game/game.h"
#include "../net/netgame.h"

#include "../spawnscreen.h"
#include "../playertags.h"
#include "../dialog.h"
#include "../keyboard.h"
#include "../settings.h"
#include "../scoreboard.h"
#include "../deathmessage.h"
#include "../GButton.h"

#include "../util/patch.h"

// voice
#include "../voice/MicroIcon.h"
#include "../voice/SpeakerList.h"
#include "../voice/include/util/Render.h"

#include "interface.h"
#include "buttons.h"
#include "gamescreen.h"

#include "../debug.h"

extern CChatWindow *pChatWindow;
extern CSpawnScreen *pSpawnScreen;
extern CPlayerTags *pPlayerTags;
extern CDialogWindow *pDialogWindow;
extern CSettings *pSettings;
extern CKeyBoard *pKeyBoard;
extern CNetGame *pNetGame;
extern CScoreBoard *pScoreBoard;
extern CDeathMessage* pDeathMessage;
extern CGame *pGame;
extern CGButton *pGButton;
extern CGameScreen *pGameScreen;
extern CDebug *pDebug;

/* imgui_impl_renderware.h */
void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* draw_data);
bool ImGui_ImplRenderWare_Init();
void ImGui_ImplRenderWare_NewFrame();
void ImGui_ImplRenderWare_ShutDown();

/*
	Все координаты GUI-элементов задаются
	относительно разрешения 1920x1080
*/

#define MULT_X	1.0f / 1920
#define MULT_Y	1.0f / 1080

CGUI::CGUI()
{
    Log("Initializing GUI..");

    // setup ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplRenderWare_Init();

    // scale
    m_vecScale.x    = io.DisplaySize.x * MULT_X;
    m_vecScale.y    = io.DisplaySize.y * MULT_Y;
    // font Size
    m_fFontSize     = ScaleY( pSettings->Get().fFontSize );

    // mouse/touch
    m_bMousePressed = false;
    m_vecMousePos   = ImVec2(0, 0);

    Log("GUI | Scale factor: %f, %f Font size: %f", m_vecScale.x, m_vecScale.y, m_fFontSize);

    // ImGui::StyleColorsClassic();
    ImGui::StyleColorsDark();
    SetupDefaultStyle();

    m_bKeysStatus   = false;
    m_bTabStatus    = false;

    m_bRenderCBbg	= pSettings->Get().szChatBG;
    m_bRenderTextBg	= pSettings->Get().szTextBG;

    m_RenderSpeedID = 0;

    m_CurrentExp = 1;
    m_ToUpExp = 1;
    m_Eat = 0;

    radar = pSettings->Get().szRadar;
    timestamp = pSettings->Get().szTimeStamp;

    bShowDebugLabels = false;

    m_fuel = 0;
    bLabelBackground = pSettings->Get().bLabelBg;

    // load fonts
    char path[0xFF];
    sprintf(path, "%sSAMP/fonts/%s", g_pszStorage, pSettings->Get().szFont);
    // cp1251 ranges 
	
	// thai cr. wut edit code new ห้าม ลบ cr.
	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x04FF, // Russia
		0x0E00, 0x0E5B, // Thai
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
		0xF020, 0xF0FF, // Half-width characters
        0x2010, 0x205E, // Punctuations
        0
	};
    Log("GUI | Loading font: %s", pSettings->Get().szFont);
    m_pFont = io.Fonts->AddFontFromFileTTF(path, m_fFontSize, nullptr, ranges);
    Log("GUI | ImFont pointer = 0x%X", m_pFont);

    Log("GUI | Loading font: gtaweap3.ttf");
	m_pFontGTAWeap = LoadFont("gtaweap3.ttf", 0);
	Log("GUI | ImFont pointer = 0x%X", m_pFontGTAWeap);

    // voice (комментировать)
	for(const auto& deviceInitCallback : Render::deviceInitCallbacks)
    {
        if(deviceInitCallback != nullptr) 
			deviceInitCallback();
    }
}

CGUI::~CGUI()
{
    // voice 
	for(const auto& deviceFreeCallback : Render::deviceFreeCallbacks)
	{
		if(deviceFreeCallback != nullptr) 
			deviceFreeCallback();
	}
    ImGui_ImplRenderWare_ShutDown();
    ImGui::DestroyContext();
    SaveMenuSettings();
}

ImFont* CGUI::LoadFont(char *font, float fontsize)
{
	ImGuiIO &io = ImGui::GetIO();

	// load fonts
	char path[0xFF];
	sprintf(path, "%sSAMP/fonts/%s", g_pszStorage, font);
	
	// ranges
	
	// thai cr. wut edit code new ห้าม ลบ cr.
	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x04FF, // Russia
		0x0E00, 0x0E5B, // Thai
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
		0xF020, 0xF0FF, // Half-width characters
        0x2010, 0x205E, // Punctuations
        0
	};
	
	ImFont* pFont = io.Fonts->AddFontFromFileTTF(path, m_fFontSize, nullptr, ranges);
	return pFont;
}

void CGUI::RenderTextDeathMessage(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float font_size, ImFont *font, bool bOutlineUseTextColor)
{
	int iOffset = bOutlineUseTextColor ? 1 : pSettings->Get().iFontOutline;
	if(bOutline)
	{
		// left
		posCur.x -= iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font == nullptr ? GetFont() : font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.x += iOffset;
		// right
		posCur.x += iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font == nullptr ? GetFont() : font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.x -= iOffset;
		// above
		posCur.y -= iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font == nullptr ? GetFont() : font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.y += iOffset;
		// below
		posCur.y += iOffset;
		ImGui::GetBackgroundDrawList()->AddText(font == nullptr ? GetFont() : font, font_size == 0.0f ? GetFontSize() : font_size, posCur, bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.y -= iOffset;
	}

	ImGui::GetBackgroundDrawList()->AddText(font == nullptr ? GetFont() : font, font_size == 0.0f ? GetFontSize() : font_size, posCur, col, text_begin, text_end);
}


void CGUI::SetupDefaultStyle() {

    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    style->WindowPadding = ImVec2(15, 15);
    style->WindowRounding = 0.0f;
    style->FramePadding = ImVec2(5, 5);
    style->FrameRounding = 15.0f;
    style->ItemSpacing = ImVec2(12, 8);
    style->ItemInnerSpacing = ImVec2(8, 6);
    style->IndentSpacing = 25.0f;
    style->ScrollbarSize = ScaleY(40.0f);
    style->ScrollbarRounding = 0.0f;
    style->GrabMinSize = 5.0f;
    style->GrabRounding = 1.0f;

    style->WindowBorderSize = 0.0f;
    style->ChildBorderSize  = 0.1f;
    style->PopupBorderSize  = 0.1f;
    style->FrameBorderSize  = 2.0f;

    colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 0.70f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

}

void CGUI::Render()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplRenderWare_NewFrame();
    ImGui::NewFrame();

    // RenderVersion();
    // RenderPosition();
    RenderFPS();

    // voice (комментировать)
	if(pNetGame)
	{
		if(pDialogWindow->m_bIsActive || pScoreBoard->m_bToggle)
		{
			SpeakerList::Hide();
			MicroIcon::Hide();
		}
		else 
		{
			if(MicroIcon::hasShowed)
			{
				SpeakerList::Show();
				MicroIcon::Show();
			}
		}

		for(const auto& renderCallback : Render::renderCallbacks)
		{
			if(renderCallback != nullptr) 
				renderCallback();
		}
	}
    
    if(pNetGame)
    {   
        pNetGame->GetPlayerBubblePool()->Draw();
        pNetGame->GetLabelPool()->Draw();
        pNetGame->GetTextDrawPool()->Draw();
    }

    if (pGameScreen) 
    {
		#if defined Flame
			pGameScreen->GetInterface()->RenderSpeedometer();
			pGameScreen->GetInterface()->RenderHud();
		#endif
		
        pGameScreen->GetInterface()->RenderMenu();
        pGameScreen->GetButtons()->Render();
        pGButton->RenderButton();
    }
        
    if(pPlayerTags)     pPlayerTags->Render();
    if(pChatWindow)     pChatWindow->Render();
    if(pDialogWindow)   pDialogWindow->Render();
    if(pSpawnScreen)    pSpawnScreen->Render();
    if(pKeyBoard)       pKeyBoard->Render();
    if(pScoreBoard)     pScoreBoard->Draw();
    if(pDeathMessage)   pDeathMessage->Render();
    
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());

    if(m_bNeedClearMousePos)
    {
        io.MousePos = ImVec2(-1, -1);
        m_bNeedClearMousePos = false;
    }
}

bool CGUI::OnTouchEvent(int type, bool multi, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();

    if(pNetGame) {
        if (pChatWindow && !pChatWindow->OnTouchEvent(type, multi, x, y))
            return false;

        if (pKeyBoard && !pKeyBoard->OnTouchEvent(type, multi, x, y))
            return false;

        pNetGame->GetTextDrawPool()->OnTouchEvent(type, multi, x, y);
    }

    switch(type)
    {
        case TOUCH_PUSH:
            io.MousePos = ImVec2(x, y);
            io.MouseDown[0] = true;
            break;

        case TOUCH_POP:
            io.MouseDown[0] = false;
            m_bNeedClearMousePos = true;
            break;

        case TOUCH_MOVE:
            io.MousePos = ImVec2(x, y);

            ScrollDialog(x, y);

            m_iLastPosY = y;
            break;
    }

    return true;
}

void CGUI::RenderFPS()
{
	if(!pSettings->Get().iFPSCounter) return;
	
	CPatch::CallFunction<void>(g_libGTASA + 0x39A0C4 + 1);
	char buff[50];
	float count = CPatch::CallFunction<float>(g_libGTASA + 0x39A054 + 1);
	sprintf(buff, "FPS: %.1f", count);
	
	float PositionFPS;
	ImVec2 posCur;
	PositionFPS = m_vecScale.y * (float)(1080.0 - m_fFontSize);
    posCur.x = m_vecScale.x * 20.0;
    posCur.y = PositionFPS;
	
	RenderText(posCur, ImColor(255, 255, 255), 1, buff);
}

void CGUI::RenderVersion()
{
    ImGuiIO& io = ImGui::GetIO();
    char buffer[128];

    // --> Render Version --> --> --> --> -->
    ImVec2 pos = ImVec2(10, 10);
    RenderText(pos, ImColor(255, 255, 255, 255), true, "Build 1.0", nullptr, 20);
}

void CGUI::RenderPosition()
{
    ImGuiIO& io = ImGui::GetIO();
    MATRIX4X4 matFromPlayer;

    CPlayerPed *pLocalPlayerPed = pGame->FindPlayerPed();
    pLocalPlayerPed->GetMatrix(&matFromPlayer);

    ImVec2 _ImVec2 = ImVec2(ScaleX(10), io.DisplaySize.y - ImGui::GetFontSize() * 0.85);

    char text[128];
    sprintf(text, "\t\tPosition > X: %.4f - Y: %.4f - Z: %.4f", matFromPlayer.pos.X, matFromPlayer.pos.Y, matFromPlayer.pos.Z);

    // RenderText(_ImVec2, ImColor(255, 255, 255, 255), false, text, nullptr, ImGui::GetFontSize() * 0.85);
}

void CGUI::RenderRakNetStatistics()
{

}

void CGUI::SetupKeyboardStyle() {

    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    style->WindowBorderSize = 0.0f;
    style->ChildBorderSize  = 0.0f;
    style->PopupBorderSize  = 0.0f;
    style->FrameBorderSize  = 0.0f;

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0x00, 0x00, 0x00, 0x00).Value);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0x00, 0x00, 0x00, 0x00).Value);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0x00, 0x00, 0x00, 0x00).Value);

}

void CGUI::RenderText(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float font_size)
{
    int iOffset = pSettings->Get().iFontOutline;

    if (bOutline)
    {
        auto oAlpha = (col & 0xFF000000) >> 24;
        auto oColor = ImColor(IM_COL32(0, 0, 0, oAlpha));

        // left
        posCur.x -= iOffset;
        ImGui::GetBackgroundDrawList()->AddText(nullptr, font_size, posCur, oColor, text_begin, text_end);
        posCur.x += iOffset;
        // right
        posCur.x += iOffset;
        ImGui::GetBackgroundDrawList()->AddText(nullptr, font_size, posCur, oColor, text_begin, text_end);
        posCur.x -= iOffset;
        // above
        posCur.y -= iOffset;
        ImGui::GetBackgroundDrawList()->AddText(nullptr, font_size, posCur, oColor, text_begin, text_end);
        posCur.y += iOffset;
        // below
        posCur.y += iOffset;
        ImGui::GetBackgroundDrawList()->AddText(nullptr, font_size, posCur, oColor, text_begin, text_end);
        posCur.y -= iOffset;
    }

    ImGui::GetBackgroundDrawList()->AddText(nullptr, font_size, posCur, col, text_begin, text_end);
}

void CGUI::RenderOverlayText(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end)
{
    int iOffset = pSettings->Get().iFontOutline;

    if(bOutline)
    {
        posCur.x -= iOffset;
        ImGui::GetOverlayDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
        posCur.x += iOffset;
        // right
        posCur.x += iOffset;
        ImGui::GetOverlayDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
        posCur.x -= iOffset;
        // above
        posCur.y -= iOffset;
        ImGui::GetOverlayDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
        posCur.y += iOffset;
        // below
        posCur.y += iOffset;
        ImGui::GetOverlayDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
        posCur.y -= iOffset;
    }

    ImGui::GetOverlayDrawList()->AddText(posCur, col, text_begin, text_end);
}

void CGUI::ScrollDialog(float x, float y)
{
    if (m_imWindow != nullptr)
    {
        // --> Scroll Window --> --> --> --> -->
        if (m_iLastPosY > y)
            ImGui::SetWindowScrollY(m_imWindow, m_imWindow->Scroll.y + ImGui::GetFontSize() / 2);

        if (m_iLastPosY < y)
            ImGui::SetWindowScrollY(m_imWindow, m_imWindow->Scroll.y - ImGui::GetFontSize() / 2);
    }
}

void CGUI::RenderTextWithSize(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float font_size)
{
    int iOffset = pSettings->Get().iFontOutline;

    if (bOutline)
    {
        // left
        posCur.x -= iOffset;
        ImGui::GetBackgroundDrawList()->AddText(m_pFont, font_size, posCur, col, text_begin, text_end);
        posCur.x += iOffset;
        // right
        posCur.x += iOffset;
        ImGui::GetBackgroundDrawList()->AddText(m_pFont, font_size, posCur, col, text_begin, text_end);
        posCur.x -= iOffset;
        // above
        posCur.y -= iOffset;
        ImGui::GetBackgroundDrawList()->AddText(m_pFont, font_size, posCur, col, text_begin, text_end);
        posCur.y += iOffset;
        // below
        posCur.y += iOffset;
        ImGui::GetBackgroundDrawList()->AddText(m_pFont, font_size, posCur, col, text_begin, text_end);
        posCur.y -= iOffset;
    }

    ImGui::GetBackgroundDrawList()->AddText(m_pFont, font_size, posCur, col, text_begin, text_end);
}