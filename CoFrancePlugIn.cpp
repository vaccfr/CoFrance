#include "pch.h"
#include "CoFrancePlugIn.h"
#include "RadarScreen.h"



CoFrancePlugIn::CoFrancePlugIn(void):CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER, MY_PLUGIN_COPYRIGHT)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    char DllPathFile[_MAX_PATH];

    GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
    DllPath = DllPathFile;
    DllPath.resize(DllPath.size() - strlen("CoFrance.dll"));
    
    LoadConfigFile();

    // Register tag items
    RegisterTagItemType("Two number ground speed", CoFranceTags::GS);
    RegisterTagItemType("Vertical Indicator", CoFranceTags::VZ_INDICATOR);
    RegisterTagItemType("Dynamic Coordinated Point (reduced)", CoFranceTags::COPXN_POINT_REDUCED);
    RegisterTagItemType("Dynamic Coordinated Level (reduced)", CoFranceTags::COPXN_ALT_REDUCED);
    RegisterTagItemType("Dynamic Coordinated Level (full)", CoFranceTags::COPXN_ALT_FULL);

    RegisterTagItemType("Dummy item", CoFranceTags::DUMMY);
    RegisterTagItemType("Dummy item (tagged)", CoFranceTags::DUMMY_TAGGED);

    RegisterTagItemType("Scratchpad Indicator (Detail)", CoFranceTags::SCRATCHPAD_INDIC);

    RegisterTagItemType("Conflict group (Detail)", CoFranceTags::CONFLICT_GROUP_DETAIL);

    RegisterTagItemType("RFL", CoFranceTags::RFL);

    DisplayUserMessage("Message", "CoFrance PlugIn", string("Version " + string(MY_PLUGIN_VERSION) + " loaded.").c_str(), false, false, false, false, false);
}

CoFrancePlugIn::~CoFrancePlugIn()
{
    GdiplusShutdown(gdiplusToken);
}

CRadarScreen* CoFrancePlugIn::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
    return new RadarScreen(this);
}

bool CoFrancePlugIn::OnCompileCommand(const char* sCommandLine)
{
    if (strcmp(sCommandLine, ".cofrance reload") == 0) {
        LoadConfigFile();
        return true;
    }

    return false;
}

void CoFrancePlugIn::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
    if (ItemCode == CoFranceTags::GS) {
        if (!RadarTarget.IsValid())
            return;

        int gs = roundUp(RadarTarget.GetPosition().GetReportedGS(), 10);
        strcpy_s(sItemString, 16, to_string(gs).substr(0, 2).c_str());
    }

    if (ItemCode == CoFranceTags::SCRATCHPAD_INDIC) {
        string txt = "¢";
        if (strlen(FlightPlan.GetControllerAssignedData().GetScratchPadString()) > 0)
            txt = FlightPlan.GetControllerAssignedData().GetScratchPadString();

        strcpy_s(sItemString, 16, txt.c_str());
    }

    if (ItemCode == CoFranceTags::CONFLICT_GROUP_DETAIL) {
        strcpy_s(sItemString, 16, "¥");
    }

    if (ItemCode == CoFranceTags::DUMMY) {
        DetailedAircraft = RadarTarget.GetCallsign();

        strcpy_s(sItemString, 16, "");
    }

    if (ItemCode == CoFranceTags::DUMMY_TAGGED) {
        if (DetailedAircraft == RadarTarget.GetCallsign())
            DetailedAircraft = "";

        strcpy_s(sItemString, 16, "");
    }

    if (ItemCode == CoFranceTags::RFL) {
        string rfl = "RFL";
        if (FlightPlan.IsValid()) {
            if (FlightPlan.GetFinalAltitude() > 0)
                rfl = padWithZeros(2, FlightPlan.GetFinalAltitude() / 1000);
        }

        strcpy_s(sItemString, 16, rfl.c_str());
    }

    if (ItemCode == CoFranceTags::VZ_INDICATOR) {
        if (!RadarTarget.IsValid())
            return;

        string tendency = "-";
        int delta_fl = RadarTarget.GetPosition().GetFlightLevel() -
            RadarTarget.GetPreviousPosition(RadarTarget.GetPosition()).GetFlightLevel();
        if (abs(delta_fl) >= 10)
            tendency = delta_fl < 0 ? "|" : "^";

        strcpy_s(sItemString, 16, tendency.c_str());
    }

    if (ItemCode == CoFranceTags::COPXN_POINT_REDUCED) {
        if (!FlightPlan.IsValid())
            return;

        string copx = "";

        if (FlightPlan.GetTrackingControllerIsMe()) {
            if (strlen(FlightPlan.GetExitCoordinationPointName()) > 0) {
                copx = FlightPlan.GetExitCoordinationPointName();
                *pColorCode = GetCoordinationTagColour(FlightPlan.GetExitCoordinationNameState());
            }
        }
        else {
            if (strlen(FlightPlan.GetEntryCoordinationPointName()) > 0) {
                copx = FlightPlan.GetEntryCoordinationPointName();
                *pColorCode = GetCoordinationTagColour(FlightPlan.GetEntryCoordinationPointState());
            }
                
        }
        
        strcpy_s(sItemString, 16, copx.c_str());
    }

    if (ItemCode == CoFranceTags::COPXN_ALT_REDUCED || ItemCode == CoFranceTags::COPXN_ALT_FULL) {
        if (!FlightPlan.IsValid())
            return;

        string copx = "";

        if (FlightPlan.GetTrackingControllerIsMe() && FlightPlan.GetExitCoordinationAltitude() > 0) {
            copx = "x" + padWithZeros(2, FlightPlan.GetExitCoordinationAltitude() / 1000);
            *pColorCode = GetCoordinationTagColour(FlightPlan.GetExitCoordinationAltitudeState());
        }
        else if (FlightPlan.GetEntryCoordinationAltitude() > 0) {
            copx = "e" + padWithZeros(FlightPlan.GetEntryCoordinationAltitude() / 1000, 2);
            *pColorCode = GetCoordinationTagColour(FlightPlan.GetEntryCoordinationAltitudeState());
        }

        if (ItemCode == CoFranceTags::COPXN_ALT_FULL && copx.length() == 0) {
            if (FlightPlan.GetTrackingControllerIsMe())
                copx = "x...";
            else
                copx = "e...";
        }


        strcpy_s(sItemString, 16, copx.c_str());
    }
}

void CoFrancePlugIn::OnTimer(int Counter)
{
}

void CoFrancePlugIn::LoadConfigFile(bool fromWeb)
{
    DisplayUserMessage("Message", "CoFrance PlugIn", "Reading config file...", false, false, false, false, false);

    try {
        if (fromWeb) {
            httplib::Client cli(CONFIG_ONLINE_URL_BASE);
            if (auto res = cli.Get(CONFIG_ONLINE_URL_PATH)) {
                if (res->status == 200) {
                    std::istringstream is(res->body, std::ios_base::binary | std::ios_base::in);

                    CoFranceConfig = toml::parse(is, "std::string");
                }
                else
                    fromWeb = false;
            }
            else 
                fromWeb = false;
                
            cli.stop();

            if (!fromWeb)
                DisplayUserMessage("Message", "CoFrance PlugIn", "Error loading web config, reverting to local file!", false, false, false, false, false);
                
        }

        if (!fromWeb)
            CoFranceConfig = toml::parse(DllPath + "\\CoFrance.toml");

        DisplayUserMessage("Message", "CoFrance PlugIn", "Config file loaded!", false, false, false, false, false);
    }
    catch (const std::exception& exc) {
        DisplayUserMessage("Message", "CoFrance PlugIn", string("Error reading config file " + string(exc.what())).c_str(), false, false, false, false, false);
    }
}
