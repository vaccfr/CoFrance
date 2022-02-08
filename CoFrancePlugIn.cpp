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

    Stca = new CSTCA(CoFranceConfig);

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

    RegisterTagItemType("Approach Intention Code", CoFranceTags::APP_INTENTION);

    RegisterTagItemType("Vertical Speed", CoFranceTags::VZ);

    RegisterTagItemType("STCA Indicator", CoFranceTags::STCA);

    RegisterTagItemType("CPDLC Flag", CoFranceTags::CPDLC_STATUS);

    RegisterTagItemType("OCL Flag", CoFranceTags::OCL_FLAG);

    RegisterTagItemType("Assigned Speed", CoFranceTags::ASSIGNED_SPEED);

    RegisterTagItemFunction("Assign Conflict Group", CoFranceTags::FUNCTION_CONFLICT_POPUP);
    //RegisterTagItemFunction("Show OCL", CoFranceTags::FUNCTION_OCL_TP);

    RegisterTagItemFunction("Open ASP Popup", CoFranceTags::FUNCTION_OPEN_ASP);

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


    if (ItemCode == CoFranceTags::ASSIGNED_SPEED) {
        string aspeed = "";
        if (FlightPlan.GetControllerAssignedData().GetAssignedMach() != 0) {
            aspeed = "m";

            aspeed += "." + to_string(FlightPlan.GetControllerAssignedData().GetAssignedMach());
            if (string(FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2)) == string("+"))
                aspeed += "+";
            if (string(FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2)) == string("-"))
                aspeed += "-";
        } else if (FlightPlan.GetControllerAssignedData().GetAssignedSpeed() != 0) {
            aspeed = "k";
            if (FlightPlan.GetControllerAssignedData().GetAssignedSpeed() % 10 == 1)
                aspeed += to_string((FlightPlan.GetControllerAssignedData().GetAssignedSpeed() - 1)/10) + "+";
            else if (FlightPlan.GetControllerAssignedData().GetAssignedSpeed() % 10 == 9)
                aspeed += to_string((FlightPlan.GetControllerAssignedData().GetAssignedSpeed() + 1)/10) + "-";
            else
                aspeed += to_string(FlightPlan.GetControllerAssignedData().GetAssignedSpeed()/10);
        }

        strcpy_s(sItemString, 16, aspeed.c_str());
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
                    rfl = padWithZeros(3, FlightPlan.GetFinalAltitude() / 100);

                if (FlightPlan.GetFinalAltitude() <= GetTransitionAltitude())
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

    if (ItemCode == CoFranceTags::APP_INTENTION) {
        // Three types of codes:
        // 1. VFR
        // 2. Arrival (Assigned Runway with different colours)
        // 3. Departure (Abbreviated SID)
        
        if (FlightPlan.IsValid()) {

            if (FlightPlan.GetFlightPlanData().GetPlanType()[0] == 'V') {
                // We have a VFR flight
                *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_vfr");
                *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                strcpy_s(sItemString, 16, "VFR");
            }
            else {

                // We have a departure
                if (FlightPlan.GetDistanceFromOrigin() <= 50) {
                    string abbr_sid = "";
                    if (strlen(FlightPlan.GetFlightPlanData().GetSidName()) > 0) {
                        abbr_sid = FlightPlan.GetFlightPlanData().GetSidName();
                        abbr_sid = abbr_sid.substr(0, 3);
                    }

                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                    auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_departure");
                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                    strcpy_s(sItemString, 16, abbr_sid.c_str());
                }
                else {
                    // We have an arrival
                    string arr_rwy = "";
                    if (strlen(FlightPlan.GetFlightPlanData().GetArrivalRwy()) > 0) {
                        arr_rwy = FlightPlan.GetFlightPlanData().GetArrivalRwy();
                        arr_rwy = arr_rwy.substr(0, 3);
                    }

                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                    auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_arrival");
                    
                    if (startsWith("LFPG", FlightPlan.GetFlightPlanData().GetDestination()) &&
                        (startsWith("26", FlightPlan.GetFlightPlanData().GetArrivalRwy()) || startsWith("08", FlightPlan.GetFlightPlanData().GetArrivalRwy()))) {
                        // South arrival at LFPG, different colour
                        element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_lfpg_arr_south");
                    }

                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                    strcpy_s(sItemString, 16, arr_rwy.c_str());
                }
            }

        }

    }

    if (ItemCode == CoFranceTags::VZ) {
        if (!RadarTarget.IsValid())
            return;

        string VerticalRate = "00";
        CRadarTargetPositionData pos = RadarTarget.GetPosition();
        CRadarTargetPositionData oldpos = RadarTarget.GetPreviousPosition(pos);
        int mathVerticalRate = 0;
        if (pos.IsValid() && oldpos.IsValid()) {
            int deltaalt = pos.GetFlightLevel() - oldpos.GetFlightLevel();
            int deltaT = oldpos.GetReceivedTime() - pos.GetReceivedTime();

            if (deltaT > 0) {
                float vz = abs(deltaalt) * (60.0f / deltaT);
                mathVerticalRate = (int)vz;

                // If the rate is too much
                if ((int)abs(vz) >= 9999) {
                    VerticalRate = "++";
                    if (deltaalt < 0)
                        VerticalRate = "--";
                }
                else if (abs(vz) >= 100 && abs(deltaalt) >= 20) {
                    string rate = padWithZeros(2, (int)abs(vz / 100));
                    VerticalRate = "+" + rate;

                    if (deltaalt < 0)
                        VerticalRate = "-" + rate;
                }
            }
        }

        strcpy_s(sItemString, 16, VerticalRate.c_str());
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

        if (string(FlightPlan.GetControllerAssignedData().GetDirectToPointName()) == copx) {\
            // If direct to COPX has been given, no need to display it
            copx = "";
        }
        
        strcpy_s(sItemString, 16, copx.substr(0, 15).c_str());
    }

    if (ItemCode == CoFranceTags::COPXN_ALT_REDUCED || ItemCode == CoFranceTags::COPXN_ALT_FULL) {
        if (!FlightPlan.IsValid())
            return;

        string copx_alt = "";

        if (FlightPlan.GetTrackingControllerIsMe()) {
            if (FlightPlan.GetExitCoordinationAltitude() > 0) {
                copx_alt = string("x") + padWithZeros(2, FlightPlan.GetExitCoordinationAltitude() / 1000);
                *pColorCode = GetCoordinationTagColour(FlightPlan.GetExitCoordinationAltitudeState());
            }
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

    if (ItemCode == CoFranceTags::STCA) {
        if (Stca->IsSTCA(RadarTarget.GetCallsign())) {

            *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
            auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "sep_warning");
            if (Blink)
                element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "stca_warning");
            
            *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

            strcpy_s(sItemString, 16, "STCA");
        }
        else {
            strcpy_s(sItemString, 16, "");
        }
    }

    if (ItemCode == CoFranceTags::OCL_FLAG) {
        if (!FlightPlan.IsValid())
            return;
        string test = FlightPlan.GetFlightPlanData().GetRoute();
        const char* destination = FlightPlan.GetFlightPlanData().GetDestination();
        // if is OCL revelant
        if (StringContainsArray(test, Brest_Oceanic_Points)) {
            // Display OCL flag
            if (HasOCL(FlightPlan.GetCallsign())) {
                // Check if level change required

                int ocl_level = GetOCLLevel(FlightPlan.GetCallsign());

                int cfl = FlightPlan.GetControllerAssignedData().GetClearedAltitude();
                if (cfl == 0)
                    cfl = FlightPlan.GetControllerAssignedData().GetFinalAltitude();

                if (cfl != ocl_level && ocl_level != 0) {
                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                    auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_departure");
                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                    string item = "LCHG";
                    item += std::to_string(ocl_level / 1000);

                    strcpy_s(sItemString, 16, item.c_str());
                }
            }
            // If there is no OCL and exit of sector is within 45mins and the flight is headed to the Americas (ICAOs starting with KCPSTMN, and SPEM) we display an alert
            else if (startsWith("K", destination) || startsWith("C", destination) || 
            startsWith("P", destination) || startsWith("T", destination) || 
            startsWith("S", destination) || startsWith("M", destination) || 
            startsWith("N", destination) || startsWith("LFVP", destination) || startsWith("LFVM", destination)) {
                int minutes = 99;
                for (int i = FlightPlan.GetExtractedRoute().GetPointsCalculatedIndex(); i < FlightPlan.GetExtractedRoute().GetPointsNumber(); i++) {
                    if (StringContainsArray(FlightPlan.GetExtractedRoute().GetPointName(i), Brest_Oceanic_Points)) {
                        minutes = FlightPlan.GetExtractedRoute().GetPointDistanceInMinutes(i);
                        break;
                    }
                }

                if (minutes <= 30) {
                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

                    auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_departure");
                    if (minutes < 15)
                        element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "sep_warning");

                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);
                    strcpy_s(sItemString, 16, "OCL");
                }
            }
        }
        else {
            strcpy_s(sItemString, 16, "");
        }
    }

    if (ItemCode == CoFranceTags::CPDLC_STATUS) {
        if (!FlightPlan.IsValid())
            return;

        if (CPDLCStatusTagMap.find(FlightPlan.GetCallsign()) != CPDLCStatusTagMap.end()) 
        {
            // Status is response pending
            if (CPDLCStatusTagMap[FlightPlan.GetCallsign()] == 1) {
                *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

                auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "intention_code_departure");
                *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);
            }

            // Status is response negative
            if (CPDLCStatusTagMap[FlightPlan.GetCallsign()] == 2) {
                *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
            
                auto element_colour = toml::find<std::vector<int>>(CoFranceConfig, "colours", "conflict_group_losange");
                *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);
            }
            
            strcpy_s(sItemString, 16, "ß");
        }
        else {
            strcpy_s(sItemString, 16, "");
        }

    }

}

void CoFrancePlugIn::OnTimer(int Counter)
{
    for (auto it = PendingStands.begin(), next_it = it; it != PendingStands.end(); it = next_it)
    {
        bool must_delete = false;
        if (it->second.valid() && it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            std::string stand = it->second.get();

            string ScratchPad = FlightPlanSelect(it->first.c_str()).GetControllerAssignedData().GetScratchPadString();
            ScratchPad = "STAND=" + stand + " " + ScratchPad;
            FlightPlanSelect(it->first.c_str()).GetControllerAssignedData().SetScratchPadString(ScratchPad.c_str());

            must_delete = true;
        }

        ++next_it;
        if (must_delete)
        {
            PendingStands.erase(it);
        }
    }

    if (CPDLCAPiData.valid() && CPDLCAPiData.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        string d = CPDLCAPiData.get();

        CPDLCStatusTagMap.clear();
        std::vector<std::string> row = split(d, '|');
        for (auto r : row) {

            std::vector<std::string> column = split(r, ',');
            if (column.size() >= 2) {
                try {
                    CPDLCStatusTagMap.insert(make_pair(column[0], stoi(column[1])));
                }
                catch (const std::exception& exc) {

                }
            }
        }
    }

    if (RawOCLData.valid() && RawOCLData.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        string d = RawOCLData.get();

        try {
            SharedData::OCLData = nlohmann::json::parse(d);
        }
        catch (std::exception &exc) {

        }
    }

    Stca->OnRefresh(this);
    Blink = !Blink;

    // Every 5 seconds send CPDLC data and poll OCL data
    if (Counter % 5 == 0) {
        if (ControllerMyself().IsValid() && ControllerMyself().IsController()) {
            string message = "";

            for (CFlightPlan fp = FlightPlanSelectFirst(); fp.IsValid(); fp = FlightPlanSelectNext(fp)) {

                if (fp.GetState() == EuroScopePlugIn::FLIGHT_PLAN_STATE_NON_CONCERNED)
                    continue;

                message += fp.GetCallsign();
                message += ",";
                message += fp.GetTrackingControllerIsMe() ? "1" : "0";
                message += ",";
                message += fp.GetFlightPlanData().GetOrigin();
                message += ",";
                message += fp.GetFlightPlanData().GetDestination();
                message += ",";


                
                for (int k = fp.GetExtractedRoute().GetPointsCalculatedIndex(); k < fp.GetExtractedRoute().GetPointsNumber(); k++) {
                    message += fp.GetExtractedRoute().GetPointName(k);
                    if (k+1 != fp.GetExtractedRoute().GetPointsNumber())
                        message += "-";
                }

                message += ",";

                CController NextController = ControllerSelect(fp.GetCoordinatedNextController());
                if (NextController.IsValid()) {
                    message += std::to_string(NextController.GetPrimaryFrequency());
                    message += ",";
                    message += NextController.GetCallsign();
                    message += ",";
                }
                else {
                    message += ",";
                    message += ",";
                }

                message += "|";
            }

            CPDLCAPiData = async(&CoFrancePlugIn::SendCPDLCActiveAircrafts, this, string(ControllerMyself().GetCallsign()), message);

            //
            // Polling OCL
            // 

            if (SharedData::OCLEnabled) {
                RawOCLData = async(&CoFrancePlugIn::LoadOCLData, this);
            }
        }
    }
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

    if (FunctionId == CoFranceTags::FUNCTION_OCL_TP) {
        CFlightPlan fp = FlightPlanSelectASEL();
        auto t = fp.GetCallsign();
        if (!fp.IsValid())
            return;

        SharedData::OCL_Tooltip_timer = std::chrono::system_clock::now();

        if (HasOCL(fp.GetCallsign())) {
            SharedData::OCL_Tooltip_string = GetFullOCL(fp.GetCallsign());
            SharedData::OCL_Tooltip_pt = Pt;
        }
    }

}

void CoFrancePlugIn::OnFlightPlanControllerAssignedDataUpdate(CFlightPlan FlightPlan, int DataType)
{

    // Send CPDLC events
    if (!FlightPlan.IsValid() || !FlightPlan.GetTrackingControllerIsMe())
        return;

    // FlightPlan is CPDLC enabled
    if (CPDLCStatusTagMap.find(FlightPlan.GetCallsign()) == CPDLCStatusTagMap.end())
        return;

    string message = "";
    if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_TEMPORARY_ALTITUDE && FlightPlan.GetControllerAssignedData().GetClearedAltitude() >= 100)
        message = std::to_string(FlightPlan.GetControllerAssignedData().GetClearedAltitude() / 100);
    if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_DIRECT_TO)
        message = FlightPlan.GetControllerAssignedData().GetDirectToPointName();
    if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_HEADING && FlightPlan.GetControllerAssignedData().GetAssignedHeading() != 0)
        message = std::to_string(FlightPlan.GetControllerAssignedData().GetAssignedHeading());
    if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_SQUAWK)
        message = FlightPlan.GetControllerAssignedData().GetSquawk();
    if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_SPEED)
        message = std::to_string(FlightPlan.GetControllerAssignedData().GetAssignedSpeed());
    if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_MACH)
        message = std::to_string(FlightPlan.GetControllerAssignedData().GetAssignedMach());

    // Send out the event
    async(&CoFrancePlugIn::SendCPDLCEvent, this, string(FlightPlan.GetCallsign()), DataType, message);

}

void CoFrancePlugIn::OnRadarTargetPositionUpdate(CRadarTarget RadarTarget)
{

    CFlightPlan CorrFp = RadarTarget.GetCorrelatedFlightPlan();
    if (!CorrFp.IsValid() || !CorrFp.GetTrackingControllerIsMe())
        return;

    string ScratchPad = string(CorrFp.GetControllerAssignedData().GetScratchPadString());


    // There is already an assigned stand
    if (ScratchPad.find("STAND=") != std::string::npos)
        return;

    
    if (std::find(StandApiAvailableFor.begin(), StandApiAvailableFor.end(), string(CorrFp.GetFlightPlanData().GetDestination())) != StandApiAvailableFor.end()) {

        if (CorrFp.GetDistanceToDestination() < 10) {
           
            if (PendingStands.find(string(CorrFp.GetCallsign())) == PendingStands.end()) {
                
                PendingStands.insert(std::make_pair(string(CorrFp.GetCallsign()), 
                    async(&CoFrancePlugIn::LoadRemoteStandAssignment, this, string(CorrFp.GetCallsign()), string(CorrFp.GetFlightPlanData().GetOrigin()),
                        string(CorrFp.GetFlightPlanData().GetDestination()),
                        string(string("") + CorrFp.GetFlightPlanData().GetAircraftWtc()))));
            }
        }
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

    DisplayUserMessage("Message", "CoFrance PlugIn", "Reading stand API config...", false, false, false, false, false);

    try {
        if (fromWeb) {
            httplib::Client cli(CONFIG_ONLINE_URL_BASE);
            if (auto res = cli.Get(CONFIG_ONLINE_STAND_API_URL_PATH)) {
                if (res->status == 200) {
                    std::istringstream is(res->body, std::ios_base::binary | std::ios_base::in);

                    toml::value StandApiConfig = toml::parse(is, "std::string");

                    StandApiAvailableFor = toml::find<vector<string>>(StandApiConfig, "data", "icaos");

                    DisplayUserMessage("Message", "CoFrance PlugIn", string("Stand API available for: " + join_vector(StandApiAvailableFor)).c_str(), false, false, false, false, false);
                }
            }

            cli.stop();

        }

    }
    catch (const std::exception& exc) {
        DisplayUserMessage("Message", "CoFrance PlugIn", string("Error reading stand api file " + string(exc.what())).c_str(), false, false, false, false, false);
    }
}

string CoFrancePlugIn::SendCPDLCActiveAircrafts(string my_callsign, string message)
{
    string r = "null";

    try {
        httplib::Client cli("http://127.0.0.1:9596");
        cli.set_connection_timeout(0, 500000);

        httplib::Params params;
        params.emplace("my_callsign", my_callsign);
        params.emplace("data", message);

        if (auto res = cli.Post("/api/ping", params)) {
            if (res->status == 200) {
                cli.stop();
                return res->body;
            }
            else {
                r = "null";
            }
        }

        cli.stop();
    }
    catch (const std::exception& exc) {

    }

    return r;
}

string CoFrancePlugIn::SendCPDLCEvent(string ac_callsign, int event_type, string value)
{
    try {
        httplib::Client cli("http://127.0.0.1:9596");
        cli.set_connection_timeout(0, 500000);

        httplib::Params params;
        params.emplace("callsign", ac_callsign);
        params.emplace("event", std::to_string(event_type));
        params.emplace("value", value);

        if (auto res = cli.Post("/api/event", params)) {
            if (res->status == 200) {
                cli.stop();
                return "ok";
            }
        }

        cli.stop();
    }
    catch (const std::exception& exc) {

    }

    return "null";
}

string CoFrancePlugIn::LoadRemoteStandAssignment(string callsign, string origin, string destination, string wtc)
{
    try {
        httplib::Client cli(CONFIG_ONLINE_URL_BASE);
        httplib::Params params;
        params.emplace("callsign", callsign);
        params.emplace("dep", origin);
        params.emplace("arr", destination);
        params.emplace("wtc", wtc);


        if (auto res = cli.Post(CONFIG_ONLINE_STAND_API_QUERY_URL_PATH, params)) {
            if (res->status == 200) {
                
                std::istringstream is(res->body, std::ios_base::binary | std::ios_base::in);

                toml::value StandData = toml::parse(is, "std::string");

                cli.stop();
                return toml::find<string>(StandData, "data", "stand");
            }
            else {
                cli.stop();
                return "NoGate";
            }
        }

        cli.stop();
    }
    catch (const std::exception& exc) {
        
    }

    return "NoGate";
}

string CoFrancePlugIn::LoadOCLData()
{
    try {
        httplib::Client cli("https://nattrak.vatsim.net");
        cli.set_connection_timeout(0, 500000);

        if (auto res = cli.Get("/pluginapi.php")) {
            if (res->status == 200) {
                cli.stop();
                return res->body;
            }
            else {
                cli.stop();
                return "[]";
            }
        }

        cli.stop();
    }
    catch (const std::exception& exc) {
        return "[]";
    }

    return "[]";

    //return "[ { \"callsign\": \"BER1PE\", \"status\": \"CLEARED\", \"nat\": \"A\", \"fix\": \"MALOT\", \"level\": \"320\", \"mach\": \"0.89\", \"estimating_time\": \"1921\", \"clearance_issued\": \"2021-03-26 00:21:19\", \"extra_info\": \"CROSS MALOT NOT BEFORE 1925\" }, { \"callsign\":\"ADB3908\", \"status\":\"PENDING\", \"nat\":\"RR\", \"fix\":\"PORTI\", \"level\": \"350\", \"mach\": \"0.82\", \"estimating_time\":\"18:41\", \"clearance_issued\":null, \"extra_info\":null } ]";
}