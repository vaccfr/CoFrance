#pragma once
#include "EuroScopePlugIn.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <sstream>

#define MY_PLUGIN_NAME "CoFrance"
#define MY_PLUGIN_VERSION "0.1"
#define MY_PLUGIN_DEVELOPER "Pierre Ferran"
#define MY_PLUGIN_COPYRIGHT "MIT License"

#define CONFIG_ONLINE_URL_BASE "https://new.vatfrance.org"
#define CONFIG_ONLINE_URL_PATH "/api/cfr/config"

using namespace std;
using namespace EuroScopePlugIn;

namespace CoFranceTags {
	const int GS = 1;
	const int VZ_INDICATOR = 2;
    const int COPXN_POINT_REDUCED = 3;
    const int COPXN_ALT_REDUCED = 4;
    const int COPXN_ALT_FULL = 5;
    const int DUMMY = 6;
    const int DUMMY_TAGGED = 7;
    const int SCRATCHPAD_INDIC = 8;
    const int CONFLICT_GROUP_DETAIL = 9;
    const int RFL = 10;
    const int CONFLICT_GROUP_TAGGED = 11;

    const int FUNCTION_CONFLICT_POPUP = 500;
    const int FUNCTION_HANDLE_CONFLICT_GROUP = 501;
}

namespace CoFranceCharacters {
    const string Moon = "±";
    const string Star = "§";
    const string Losange = "÷";
    const string Pen = "µ";
}

namespace StaticColours {
    const Gdiplus::Color YellowHighlight(255, 255, 0);

    const Gdiplus::Color DarkBlueMenu(0, 2, 48);
    const Gdiplus::Color LightBlueMenu(23, 156, 155);
    const Gdiplus::Color MenuButtonTop(165, 165, 165);
    const Gdiplus::Color MenuButtonBottom(74, 74, 74);
    const Gdiplus::Color GreyTextMenu(162, 162, 162);

    const Gdiplus::Color SectorActive(4, 7, 14);
    const Gdiplus::Color SectorInactive(22, 22, 22);
}

const static int ButtonPaddingSides = 5;
const static int ButtonPaddingTop = 2;

static int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

inline static bool IsInRect(POINT pt, CRect rect) {
    if (pt.x >= rect.left + 1 && pt.x <= rect.right - 1 && pt.y >= rect.top + 1 && pt.y <= rect.bottom - 1)
        return true;
    return false;
}

static int GetCoordinationTagColour(int CoordinationState) {
    switch (CoordinationState) {
    case EuroScopePlugIn::COORDINATION_STATE_REQUESTED_BY_ME:
        return EuroScopePlugIn::TAG_COLOR_ONGOING_REQUEST_FROM_ME;
        break;
    case EuroScopePlugIn::COORDINATION_STATE_REQUESTED_BY_OTHER:
        return EuroScopePlugIn::TAG_COLOR_ONGOING_REQUEST_TO_ME;
        break;
    case EuroScopePlugIn::COORDINATION_STATE_ACCEPTED:
        return EuroScopePlugIn::TAG_COLOR_ONGOING_REQUEST_ACCEPTED;
        break;
    case EuroScopePlugIn::COORDINATION_STATE_MANUAL_ACCEPTED:
        return EuroScopePlugIn::TAG_COLOR_ONGOING_REQUEST_ACCEPTED;
        break;
    case EuroScopePlugIn::COORDINATION_STATE_REFUSED:
        return EuroScopePlugIn::TAG_COLOR_ONGOING_REQUEST_REFUSED;
        break;
    default:
        return EuroScopePlugIn::TAG_COLOR_DEFAULT;
    }
}

static Gdiplus::Color vectorToGdiplusColour(std::vector<int> input) {
    if (input.size() == 3) {
        return Gdiplus::Color(input.at(0), input.at(1), input.at(2));
    }

    if (input.size() == 4) {
        return Gdiplus::Color(input.at(0), input.at(1), input.at(2), input.at(3));
    }
    
    return Gdiplus::Color(255, 0, 0);
}

static double DegToRad(const double degree) { return (degree * M_PI / 180.0); };
static double RadToDeg(const double radian) { return (radian * 180.0 / M_PI); };

static CPosition Extrapolate(CPosition init, double angle, double nm)
{
    CPosition newPos;

    double d = nm / 60 * M_PI / 180;
    double trk = DegToRad(angle);
    double lat0 = DegToRad(init.m_Latitude);
    double lon0 = DegToRad(init.m_Longitude);

    double lat = asin(sin(lat0) * cos(d) + cos(lat0) * sin(d) * cos(trk));
    double lon = cos(lat) == 0 ? lon0 : fmod(lon0 + asin(sin(trk) * sin(d) / cos(lat)) + M_PI, 2 * M_PI) - M_PI;

    newPos.m_Latitude = RadToDeg(lat);
    newPos.m_Longitude = RadToDeg(lon);

    return newPos;
}

static Gdiplus::Point rotatePoint(Gdiplus::Point center, float angle, Gdiplus::Point p)
{
    float s = sin(angle);
    float c = cos(angle);

    // translate point back to origin:
    p.X -= center.X;
    p.Y -= center.Y;

    // rotate point
    float xnew = p.X * c - p.Y * s;
    float ynew = p.X * s + p.Y * c;

    // translate point back:
    p.X = xnew + center.X;
    p.Y = ynew + center.Y;
    return p;
}

static string padWithZeros(int padding, int s)
{
    stringstream ss;
    ss << setfill('0') << setw(padding) << s;
    return ss.str();
};

static COLORREF GetConflictGroupColor(toml::value Config, string ConflictGroup) {
    if (ConflictGroup == CoFranceCharacters::Moon)
        return vectorToGdiplusColour(toml::find<std::vector<int>>(Config, "colours", "conflict_group_moon")).ToCOLORREF();
    if (ConflictGroup == CoFranceCharacters::Star)
        return vectorToGdiplusColour(toml::find<std::vector<int>>(Config, "colours", "conflict_group_star")).ToCOLORREF();
    if (ConflictGroup == CoFranceCharacters::Losange)
        return vectorToGdiplusColour(toml::find<std::vector<int>>(Config, "colours", "conflict_group_losange")).ToCOLORREF();
    return COLORREF();
}