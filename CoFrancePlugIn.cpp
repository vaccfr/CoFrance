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
    RegisterTagItemType("Conflict group (Tagged)", CoFranceTags::CONFLICT_GROUP_TAGGED);

    RegisterTagItemType("RFL", CoFranceTags::RFL);

    RegisterTagItemType("CFL", CoFranceTags::CFL);
    RegisterTagItemType("CFL (Detail)", CoFranceTags::CFL_DETAILED);

    RegisterTagItemType("Abbreviated SID", CoFranceTags::ABBR_SID);

    RegisterTagItemFunction("Assign Conflict Group", CoFranceTags::FUNCTION_CONFLICT_POPUP);


    DisplayUserMessage("Message", "CoFrance PlugIn", string("Version " + string(MY_PLUGIN_VERSION) + " loaded.").c_str(), false, false, false, false, false);
}

CoFrancePlugIn::~CoFrancePlugIn()
{
    GdiplusShutdown(gdiplusToken);
}

CRadarScreen* CoFrancePlugIn::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
    if (CanLoadRadarScreen)
        return new RadarScreen(this);
    else
        return NULL;
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
        string gs_s = padWithZeros(3, gs);
        strcpy_s(sItemString, 16, gs_s.substr(0, 2).c_str());
    }

    if (ItemCode == CoFranceTags::SCRATCHPAD_INDIC) {
        string txt = CoFranceCharacters::Pen;
        if (strlen(FlightPlan.GetControllerAssignedData().GetScratchPadString()) > 0)
            txt = FlightPlan.GetControllerAssignedData().GetScratchPadString();

        strcpy_s(sItemString, 16, txt.substr(0, 15).c_str());
    }

    if (ItemCode == CoFranceTags::CONFLICT_GROUP_DETAIL) {
        if (ConflictGroups.find(FlightPlan.GetCallsign()) != ConflictGroups.end()) {
            strcpy_s(sItemString, 16, ConflictGroups[FlightPlan.GetCallsign()].c_str());
            *pColorCode = TAG_COLOR_RGB_DEFINED;
            *pRGB = GetConflictGroupColor(this->CoFranceConfig, ConflictGroups[FlightPlan.GetCallsign()].c_str());
        }
        else {
            strcpy_s(sItemString, 16, CoFranceCharacters::Moon.c_str());
        }
    }

    if (ItemCode == CoFranceTags::CONFLICT_GROUP_TAGGED) {
        if (ConflictGroups.find(FlightPlan.GetCallsign()) != ConflictGroups.end()) {
            strcpy_s(sItemString, 16, ConflictGroups[FlightPlan.GetCallsign()].c_str());
            *pColorCode = TAG_COLOR_RGB_DEFINED;
            *pRGB = GetConflictGroupColor(this->CoFranceConfig, ConflictGroups[FlightPlan.GetCallsign()].c_str());
        }
        else {
            strcpy_s(sItemString, 16, "");
        }
    }

    if (ItemCode == CoFranceTags::DUMMY) {
        DetailedAircraft = RadarTarget.GetCallsign();

        strcpy_s(sItemString, 16, "");
    }

    if (ItemCode == CoFranceTags::ABBR_SID) {
        string abbr_sid = "";
        if (strlen(FlightPlan.GetFlightPlanData().GetSidName()) > 0) {
            abbr_sid = FlightPlan.GetFlightPlanData().GetSidName();
            abbr_sid = abbr_sid.substr(0, 3);
        }

        strcpy_s(sItemString, 16, abbr_sid.c_str());
    }

    if (ItemCode == CoFranceTags::DUMMY_TAGGED) {
        if (DetailedAircraft == RadarTarget.GetCallsign())
            DetailedAircraft = "";

        strcpy_s(sItemString, 16, "");
    }

    if (ItemCode == CoFranceTags::RFL) {
        string rfl = "RFL";
        if (FlightPlan.IsValid()) {
            if (FlightPlan.GetFinalAltitude() > 0) {
                rfl = padWithZeros(2, FlightPlan.GetFinalAltitude() / 1000);

                // VFRs can be cleared up to 500 feet interval so we have to display more
                if (FlightPlan.GetFlightPlanData().GetPlanType() == "V")
                    rfl = padWithZeros(3, FlightPlan.GetClearedAltitude() / 100);

                if (FlightPlan.GetClearedAltitude() <= GetTransitionAltitude())
                    rfl = "A" + rfl;
            }
        }

        strcpy_s(sItemString, 16, rfl.c_str());
    }

    if (ItemCode == CoFranceTags::CFL || ItemCode == CoFranceTags::CFL_DETAILED) {
        string cfl = "";
        if (ItemCode == CoFranceTags::CFL_DETAILED)
            cfl = "CFL";

        if (FlightPlan.IsValid()) {
            if (FlightPlan.GetClearedAltitude() > 0) {
                cfl = padWithZeros(2, FlightPlan.GetClearedAltitude() / 1000);

                // VFRs can be cleared up to 500 feet interval so we have to display more
                if (FlightPlan.GetFlightPlanData().GetPlanType() == "V")
                    cfl = padWithZeros(3, FlightPlan.GetClearedAltitude() / 100);

                if (FlightPlan.GetClearedAltitude() <= GetTransitionAltitude())
                    cfl = "A" + cfl;
            }

            if (FlightPlan.GetControllerAssignedData().GetClearedAltitude() == 1)
                cfl = "®";

            if (FlightPlan.GetControllerAssignedData().GetClearedAltitude() == 2)
                cfl = "©";

            // If not detailed and reached alt, then nothing to show
            if (ItemCode == CoFranceTags::CFL && abs(RadarTarget.GetPosition().GetFlightLevel() - FlightPlan.GetClearedAltitude()) < 100)
                cfl = "";
        }

        strcpy_s(sItemString, 16, cfl.c_str());
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
        
        strcpy_s(sItemString, 16, copx.substr(0, 15).c_str());
    }

    if (ItemCode == CoFranceTags::COPXN_ALT_REDUCED || ItemCode == CoFranceTags::COPXN_ALT_FULL) {
        if (!FlightPlan.IsValid())
            return;

        string copx_alt = "";

        if (FlightPlan.GetTrackingControllerIsMe() && FlightPlan.GetExitCoordinationAltitude() > 0) {
            copx_alt = string("x") + padWithZeros(2, FlightPlan.GetExitCoordinationAltitude() / 1000);
            *pColorCode = GetCoordinationTagColour(FlightPlan.GetExitCoordinationAltitudeState());
        }
        else if (FlightPlan.GetEntryCoordinationAltitude() > 0) {
            copx_alt = string("e") + padWithZeros(2, FlightPlan.GetEntryCoordinationAltitude() / 1000);
            *pColorCode = GetCoordinationTagColour(FlightPlan.GetEntryCoordinationAltitudeState());
        }

        if (ItemCode == CoFranceTags::COPXN_ALT_FULL && copx_alt.size() == 0) {
            if (FlightPlan.GetTrackingControllerIsMe())
                copx_alt = "x...";
            else
                copx_alt = "e...";
        }


        strcpy_s(sItemString, 16, copx_alt.substr(0, 15).c_str());
    }
}

void CoFrancePlugIn::OnTimer(int Counter)
{
}

void CoFrancePlugIn::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
    if (FunctionId == CoFranceTags::FUNCTION_CONFLICT_POPUP) {
        SetASELAircraft(FlightPlanSelect(sItemString));
        OpenPopupList(Area, "Conflict Group", 1);
        AddPopupListElement(CoFranceCharacters::Moon.c_str(), "", CoFranceTags::FUNCTION_HANDLE_CONFLICT_GROUP);
        AddPopupListElement(CoFranceCharacters::Star.c_str(), "", CoFranceTags::FUNCTION_HANDLE_CONFLICT_GROUP);
        AddPopupListElement(CoFranceCharacters::Losange.c_str(), "", CoFranceTags::FUNCTION_HANDLE_CONFLICT_GROUP);
        AddPopupListElement("Remove", "", CoFranceTags::FUNCTION_HANDLE_CONFLICT_GROUP, false, 2, false, true);
    }

    if (FunctionId == CoFranceTags::FUNCTION_HANDLE_CONFLICT_GROUP) {
        if (ConflictGroups.find(FlightPlanSelectASEL().GetCallsign()) != ConflictGroups.end() && strcmp(sItemString, "Remove") == 0)
            ConflictGroups.erase(FlightPlanSelectASEL().GetCallsign());
        else if (strcmp(sItemString, "Remove") != 0)
            ConflictGroups[FlightPlanSelectASEL().GetCallsign()] = sItemString;
    }

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

        CanLoadRadarScreen = true;
        DisplayUserMessage("Message", "CoFrance PlugIn", "Config file loaded!", false, false, false, false, false);
    }
    catch (const std::exception& exc) {
        CanLoadRadarScreen = false;
        DisplayUserMessage("Message", "CoFrance PlugIn", string("Error reading config file " + string(exc.what())).c_str(), false, false, false, false, false);
    }
}
