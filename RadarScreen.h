#pragma once
#include "EuroScopePlugIn.h"
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>
#include "Constants.h"
#include <vector>


using namespace Gdiplus;
using namespace std;
using namespace EuroScopePlugIn;

class RadarScreen : public CRadarScreen
{
public:
	RadarScreen();
	~RadarScreen();

	toml::value Config;

	//---OnRefresh------------------------------------------------------

	void OnRefresh(HDC hDC, int Phase);

	inline virtual void OnAsrContentToBeClosed(void)
	{
		delete this;
	};

	double PI = (double)M_PI;
	double DegToRad(const double degree) { return (degree * PI / 180); };
	double RadToDeg(const double radian) { return (radian * 180 / PI); };

	CPosition Extrapolate(CPosition init, double angle, double nm)
	{
		CPosition newPos;

		double d = nm / 60 * PI / 180;
		double trk = DegToRad(angle);
		double lat0 = DegToRad(init.m_Latitude);
		double lon0 = DegToRad(init.m_Longitude);

		double lat = asin(sin(lat0) * cos(d) + cos(lat0) * sin(d) * cos(trk));
		double lon = cos(lat) == 0 ? lon0 : fmod(lon0 + asin(sin(trk) * sin(d) / cos(lat)) + PI, 2 * PI) - PI;

		newPos.m_Latitude = RadToDeg(lat);
		newPos.m_Longitude = RadToDeg(lon);

		return newPos;
	}

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


	void Log(string s)
	{
		std::ofstream file;
		file.open("C:/Users/Pierre/source/repos/CoFrance/Debug/CoFrance_custom.log", std::ofstream::out | std::ofstream::app);
		file << s << endl;
		file.close();
	}
};

