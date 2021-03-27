#pragma once
#include "EuroScopePlugIn.h"
#include "Constants.h"
#include <string>
#include <algorithm>
#include <thread>
#include <future>
#include <map>
#include "STCA.h"

using namespace EuroScopePlugIn;
using namespace Gdiplus;
using namespace std;

class CoFrancePlugIn :
    public CPlugIn
{
public:
    CoFrancePlugIn();
    ~CoFrancePlugIn();

    //---OnRadarScreenCreated------------------------------------------

    CRadarScreen* OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

    //---OnCompileCommand-----------------------------------------------

    bool OnCompileCommand(const char* sCommandLine);

    void OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize);

    void OnTimer(int Counter);

    void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);

    string LoadRemoteStandAssignment(string callsign, string origin, string destination, string wtc);

    void OnRadarTargetPositionUpdate(CRadarTarget RadarTarget);

    toml::value CoFranceConfig;
    vector<string> StandApiAvailableFor;
    map<string, future<string>> PendingStands;
    string DetailedAircraft;
    string DllPath;
    map<string, string> ConflictGroups;
    bool CanLoadRadarScreen = true;
    bool Blink = false;

    CSTCA *Stca = nullptr;

    void LoadConfigFile(bool fromWeb = true);

    string SendCPDLCActiveAircrafts(string my_callsign, string message);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    std::future<string> CPDLCAPiData;
    map<string, int> CPDLCStatusTagMap;

    void Log(string s)
    {
        std::ofstream file;
        file.open("C:/Users/Pierre/source/repos/CoFrance/Debug/CoFrance_custom.log", std::ofstream::out | std::ofstream::app);
        file << s << endl;
        file.close();
    }

};

