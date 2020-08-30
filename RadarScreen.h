#pragma once
#include "EuroScopePlugIn.h"
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>
#include "Constants.h"
#include <vector>
#include <algorithm>
#include "CoFrancePlugIn.h"
#include "VERA.h"


using namespace Gdiplus;
using namespace std;
using namespace EuroScopePlugIn;

class RadarScreen : public CRadarScreen
{
public:
	RadarScreen(CoFrancePlugIn* CoFrancepluginInstance);
	~RadarScreen();

	CoFrancePlugIn* CoFrancepluginInstance;
	int Filter_Upper = 99999;
	int Filter_Lower = 0;
	bool EnableTagDrawings = true;
	bool EnableFilters = false;

	bool EnableVV = false;
	int VV_Minutes = 2;

	bool ApproachMode = false;

	string FirstSepToolCallsign = "";
	map<string, string> ActiveSepTools;
	map<string, CRect> ToAddAcSymbolScreenObject;

	int AC_SYMBOL = 800;

	int BUTTON_FILTRES = 950;
	int BUTTON_FILTRES_LOWER = 951;
	int BUTTON_FILTRES_UPPER = 952;
	int BUTTON_RAD = 953;
	int BUTTON_VV = 954;
	int BUTTON_VV_TIME = 955;
	int BUTTON_APPROACH = 956;

	int FUNCTION_SET_VV_TIME = 785;
	int FUNCTION_SET_LOWER_FILTER = 787;
	int FUNCTION_SET_HIGHER_FILTER = 788;

	POINT MousePt;

	//---OnRefresh------------------------------------------------------

	void OnRefresh(HDC hDC, int Phase);

	inline void OnAsrContentToBeClosed()
	{
		delete this;
	};

	void DrawFixedSizedText(Graphics* g, CPosition TextPosition, int Size, string text, Color c) {
		
		POINT pt = ConvertCoordFromPositionToPixel(TextPosition);

		int maxLenghtNm = Size;

		CPosition fontPosFirst;
		CPosition fontPosLast;
		fontPosFirst.LoadFromStrings("E002.09.16.748", "N050.42.02.815");
		fontPosLast = Extrapolate(fontPosFirst, 90, maxLenghtNm);

		POINT fontPosFirstPt = ConvertCoordFromPositionToPixel(fontPosFirst);
		POINT fontPosLastPt = ConvertCoordFromPositionToPixel(fontPosLast);

		int width = fontPosLastPt.x - fontPosFirstPt.x;
		int FontSize = width / 3;

		Font* LevelFontF = new Gdiplus::Font(L"vCTR Text", Gdiplus::REAL(FontSize), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		g->DrawString(wstring(text.begin(), text.end()).c_str(), wcslen(wstring(text.begin(), text.end()).c_str()), LevelFontF, PointF(Gdiplus::REAL(pt.x), Gdiplus::REAL(pt.y)), &SolidBrush(c));

	}

	void OnOverScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area);

	void OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button);

	void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);

	CRect DrawMenuBarButton(CDC* dc, POINT TopLeft, string Text, bool Pressed) {
		COLORREF DarkBlueMenu = StaticColours::DarkBlueMenu.ToCOLORREF();
		COLORREF LightBlueMenu = StaticColours::LightBlueMenu.ToCOLORREF();
		CBrush ButtonBackground(DarkBlueMenu);
		CBrush ButtonPressedBackground(LightBlueMenu);
		CPen YellowPen(PS_SOLID, 1, StaticColours::YellowHighlight.ToCOLORREF());

		// We need to calculate the size of the button according to the text fitting
		CSize TextSize = dc->GetTextExtent(Text.c_str());

		int Width = TextSize.cx + ButtonPaddingSides * 2;
		int Height = TextSize.cy + ButtonPaddingTop * 2;

		CRect Button(TopLeft.x, TopLeft.y, TopLeft.x + Width, TopLeft.y + Height);

		if (!Pressed)
			dc->FillSolidRect(Button, DarkBlueMenu);
		else
			dc->FillSolidRect(Button, LightBlueMenu);

		dc->Draw3dRect(TopLeft.x, TopLeft.y, Width, Height, StaticColours::MenuButtonTop.ToCOLORREF(), StaticColours::MenuButtonBottom.ToCOLORREF());

		if (IsInRect(MousePt, Button)) {
			dc->SelectStockObject(NULL_BRUSH);
			dc->SelectObject(&YellowPen);
			dc->Rectangle(Button);
		}

		// Text Draw
		if (!Pressed)
			dc->SetTextColor(StaticColours::GreyTextMenu.ToCOLORREF());
		else
			dc->SetTextColor(StaticColours::DarkBlueMenu.ToCOLORREF());

		dc->TextOutA(TopLeft.x + ButtonPaddingSides, TopLeft.y + ButtonPaddingTop, Text.c_str());

		return Button;
	};

	void FillInAltitudeList(CPlugIn* Plugin, int FunctionId, int Current) {
		Plugin->AddPopupListElement(" 999 ", "", FunctionId, Current == 99999);
		for (int i = 410; i >= 0; i -= 10)
			Plugin->AddPopupListElement(string(string(" ") + to_string(i) + string(" ")).c_str(), "", FunctionId, Current / 100 == i);
	};

	void Log(string s)
	{
		std::ofstream file;
		file.open("C:/Users/Pierre/source/repos/CoFrance/Debug/CoFrance_custom.log", std::ofstream::out | std::ofstream::app);
		file << s << endl;
		file.close();
	}
};

