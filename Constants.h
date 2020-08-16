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
}


static int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
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

static int DegToRad(int angle) {
    return angle * M_PI / 180;
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