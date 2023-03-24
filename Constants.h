#pragma once
#include "EuroScopePlugIn.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"

#define MY_PLUGIN_NAME "CoFrance"
#define MY_PLUGIN_VERSION "@appveyor_build"
#define MY_PLUGIN_DEVELOPER "Pierre Ferran"
#define MY_PLUGIN_COPYRIGHT "GPL v3"

#define CONFIG_ONLINE_URL_BASE "https://raw.githubusercontent.com"
#define CONFIG_ONLINE_URL_PATH "/vaccfr/CoFrance/master/config.toml"
#define API_URL_BASE "https://fire-ops.ew.r.appspot.com"
#define CONFIG_ONLINE_STAND_API_URL_PATH "/api/cfr/stand"
#define CONFIG_ONLINE_STAND_API_QUERY_URL_PATH "/api/cfr/stand/query"

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
    const int CFL = 12;
    const int CFL_DETAILED = 13;
    const int APP_INTENTION = 15;
    const int VZ = 16;
    const int STCA = 17;
    const int CPDLC_STATUS = 18;
    const int OCL_FLAG = 19;
    const int ASSIGNED_SPEED = 20;

    const int FUNCTION_CONFLICT_POPUP = 500;
    const int FUNCTION_HANDLE_CONFLICT_GROUP = 501;
    const int FUNCTION_SEP_TOOL = 502;
    const int FUNCTION_OCL_TP = 503;
    const int FUNCTION_OPEN_ASP = 504;

    const int FUNCTION_ASP_TOOL_MIN = 901;
    const int FUNCTION_ASP_TOOL_MAX = 902;

    const int FUNCTION_ASP_TOOL_LIST = 903;
    const int FUNCTION_ASP_TOOL_TOGGLE_M = 904;
    const int FUNCTION_ASP_TOOL_RESUME = 905;
    const int FUNCTION_ASP_TOOL_CANCEL = 906;
}

namespace CoFranceCharacters {
    const string Moon = "±";
    const string Star = "§";
    const string Losange = "÷";
    const string Pen = "µ";
    const string ClearedVisualIndicator = "©";
    const string ClearedAPPIndicator = "®";
    const string DatalinkIndicator = "ß";
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

    const Gdiplus::Color ListBackground(43, 51, 54);
    const Gdiplus::Color ListForeground(0, 0, 0);

    const Gdiplus::Color ListLightForeground(93, 103, 112);

    const Gdiplus::Color SelectListBackground(32, 61, 64);
    const Gdiplus::Color SelectActiveListBackground(105, 180, 211);
}

namespace SharedData {
    static bool OCLEnabled = true;
    static nlohmann::json OCLData;

    static auto OCL_Tooltip_timer = std::chrono::system_clock::now();
    static string OCL_Tooltip_string;
    static POINT OCL_Tooltip_pt;
}

static bool HasOCL(string callsign) {
    try {
        for (auto d : SharedData::OCLData) {
            if (d.contains("callsign") && d.contains("status")) {
                if (d["callsign"] == callsign && d["status"] == "CLEARED")
                    return true;
            }
        }
    }
    catch (std::exception& exc) {

    }

    return false;
}

static int GetOCLLevel(string callsign) {
    try {
        for (auto d : SharedData::OCLData) {
            if (d.contains("callsign") && d.contains("status") && d.contains("level")) {
                if (d["callsign"] == callsign && d["status"] == "CLEARED")
                    return std::stoi(d["level"].get<std::string>())*100;
            }
        }
    }
    catch (std::exception& exc) {

    }

    return 0;
}

static string GetFullOCL(string callsign) {
    try {
        for (auto d : SharedData::OCLData) {
            if (d.contains("callsign") && d.contains("status") && d.contains("level") && d.contains("fix") && d.contains("mach") && d.contains("extra_info")) {
                if (d["callsign"] == callsign && d["status"] == "CLEARED")
                    return string("OCL ") + d["fix"].get<std::string>() + string(" ") + d["level"].get<std::string>() + string(" m")+ d["mach"].get<std::string>() + string(" ") + d["extra_info"].get<std::string>();
            }
        }
    }
    catch (std::exception& exc) {
        return "";
    }

    return "";
}

const static int ButtonPaddingSides = 5;
const static int ButtonPaddingTop = 2;

const vector<string> Brest_Oceanic_Points = {"ETIKI", "UMLER", "SEPAL", "BUNAV", "SIVIR"};

inline static bool startsWith(const char* pre, const char* str)
{
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
};

static bool StringContainsArray(string str, vector<string> possibilities) {
    for (auto& c : str) c = toupper(c);

    for (auto p : possibilities) {
        if (str.find(p) != std::string::npos)
            return true;
    }

    return false;
}

static Gdiplus::GraphicsPath* RoundedRect(Gdiplus::Rect bounds, int radius)
{
    int diameter = radius * 2;
    Gdiplus::Size size = Gdiplus::Size(diameter, diameter);
    Gdiplus::Point pt;
    bounds.GetLocation(&pt);
    Gdiplus::Rect arc = Gdiplus::Rect(pt, size);
    Gdiplus::GraphicsPath* path = new Gdiplus::GraphicsPath();

    // top left arc 
    path->AddArc(arc, 180, 90);

    // top right arc  
    arc.X = bounds.GetRight() - diameter;
    path->AddArc(arc, 270, 90);

    // bottom right arc  
    arc.Y = bounds.GetBottom() - diameter;
    path->AddArc(arc, 0, 90);

    // bottom left arc 
    arc.X = bounds.GetLeft();
    path->AddArc(arc, 90, 90);

    path->CloseFigure();

    return path;
}

static int DrawCenteredText(CDC* dc, POINT Center, string s) {
    CSize cs = dc->GetTextExtent(s.c_str());
    dc->TextOutA(Center.x - cs.cx / 2, Center.y, s.c_str());
    return cs.cy;
};

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
    p.X = static_cast<int>(xnew) + center.X;
    p.Y = static_cast<int>(ynew) + center.Y;
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

inline static std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        elems.push_back(item);
    return elems;
};
inline static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
};

inline static string join_vector(vector<string> v, string sep = ", ") {
    std::string s;
    for (const auto& piece : v) s += piece + sep;
    return s;
}

// Liang-Barsky function by Daniel White @ http://www.skytopia.com/project/articles/compsci/clipping.html
// This function inputs 8 numbers, and outputs 4 new numbers (plus a boolean value to say whether the clipped line is drawn at all).
//
inline static bool LiangBarsky(RECT Area, POINT fromSrc, POINT toSrc, POINT& ClipFrom, POINT& ClipTo)         // The output values, so declare these outside.
{

    double edgeLeft, edgeRight, edgeBottom, edgeTop, x0src, y0src, x1src, y1src;

    edgeLeft = Area.left;
    edgeRight = Area.right;
    edgeBottom = Area.top;
    edgeTop = Area.bottom;

    x0src = fromSrc.x;
    y0src = fromSrc.y;
    x1src = toSrc.x;
    y1src = toSrc.y;

    double t0 = 0.0;    double t1 = 1.0;
    double xdelta = x1src - x0src;
    double ydelta = y1src - y0src;

    double p = 0, q = 0, r;

    for (int edge = 0; edge < 4; edge++) {   // Traverse through left, right, bottom, top edges.
        if (edge == 0) { p = -xdelta;    q = -(edgeLeft - x0src); }
        if (edge == 1) { p = xdelta;     q = (edgeRight - x0src); }
        if (edge == 2) { p = -ydelta;    q = -(edgeBottom - y0src); }
        if (edge == 3) { p = ydelta;     q = (edgeTop - y0src); }
        r = q / p;
        if (p == 0 && q < 0) return false;   // Don't draw line at all. (parallel line outside)

        if (p < 0) {
            if (r > t1) return false;         // Don't draw line at all.
            else if (r > t0) t0 = r;            // Line is clipped!
        }
        else if (p > 0) {
            if (r < t0) return false;      // Don't draw line at all.
            else if (r < t1) t1 = r;         // Line is clipped!
        }
    }

    ClipFrom.x = long(x0src + t0 * xdelta);
    ClipFrom.y = long(y0src + t0 * ydelta);
    ClipTo.x = long(x0src + t1 * xdelta);
    ClipTo.y = long(y0src + t1 * ydelta);

    return true;        // (clipped) line is drawn
};
