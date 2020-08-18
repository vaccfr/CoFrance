#include "pch.h"
#include "RadarScreen.h"

RadarScreen::RadarScreen(CoFrancePlugIn* CoFrancepluginInstance)
{
	this->CoFrancepluginInstance = CoFrancepluginInstance;
}

RadarScreen::~RadarScreen()
{
	
}

void RadarScreen::OnRefresh(HDC hDC, int Phase)
{
	Graphics g(hDC);
	g.SetPageUnit(UnitPixel);
	g.SetSmoothingMode(SmoothingModeAntiAlias);

	CDC dc;
	dc.Attach(hDC);

	POINT p;
	if (GetCursorPos(&p)) {
		if (ScreenToClient(GetActiveWindow(), &p)) {
			MousePt = p;
		}
	}

	try {

		auto sector_ceiling_text_color = toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "sector_vertical_limits");
		Color SectorCeilingColor(sector_ceiling_text_color.at(0), sector_ceiling_text_color.at(1), sector_ceiling_text_color.at(2), sector_ceiling_text_color.at(3));
	
		// Drawing of SCT levels
		if (Phase == EuroScopePlugIn::REFRESH_PHASE_BACK_BITMAP && GetPlugIn()->ControllerMyself().IsController())
		{

			string MyPositionId = GetPlugIn()->ControllerMyself().GetPositionId();

			// Get a list of all other online controllers
			vector<string> AllControllers;
			for (CController cc = GetPlugIn()->ControllerSelectFirst(); cc.IsValid(); cc = GetPlugIn()->ControllerSelectNext(cc))
			{
				if (cc.IsController())
					AllControllers.push_back(cc.GetPositionId());
			}


			for (const auto& v : toml::find<toml::array>(this->CoFrancepluginInstance->CoFranceConfig, "sector_vertical_limit"))
			{
				// Check if we are the appropriate controller
				
				if (MyPositionId != toml::find<string>(v, "controller_me"))
					continue;

				// Check if any of the other controllers required are online
				bool DrawText = false;
				auto neededControllers = toml::find<vector<string>>(v, "controller_others");
				for (const auto& c : neededControllers)
				{
					if (find(AllControllers.begin(), AllControllers.end(), c) != AllControllers.end())
						DrawText = true; break;
				}

				// Check if any of the controllers which hide the text are online
				if (v.contains("controller_not_others")) {
					auto notNeededControllers = toml::find<vector<string>>(v, "controller_not_others");
					for (const auto& c : notNeededControllers)
					{
						if (find(AllControllers.begin(), AllControllers.end(), c) != AllControllers.end())
							DrawText = false; break;
					}
				}

				if (DrawText) {
					CPosition p;
					p.LoadFromStrings(toml::find<vector<string>>(v, "position").back().c_str(), toml::find<vector<string>>(v, "position").front().c_str());
					DrawFixedSizedText(&g, p, toml::find<int>(v, "size"), toml::find<string>(v, "text"), SectorCeilingColor);
				}

			}

		}

		if (Phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS) {
			POINT StartOfMenu = { GetRadarArea().right - toml::find<int>(this->CoFrancepluginInstance->CoFranceConfig, "menu", "menu_position"), GetRadarArea().top };
			CRect r = this->DrawMenuBarButton(&dc, StartOfMenu, "CoFrance v" + string(MY_PLUGIN_VERSION), false);
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, "RAD", EnableTagDrawings);
			AddScreenObject(BUTTON_RAD, "", r, false, "");
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, "FILTRES", EnableFilters);
			AddScreenObject(BUTTON_FILTRES, "", r, false, "");
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, "I" + padWithZeros(3, Filter_Lower / 100), false);
			AddScreenObject(BUTTON_FILTRES_LOWER, "", r, false, "");
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, "S" + padWithZeros(3, Filter_Upper / 100), false);
			AddScreenObject(BUTTON_FILTRES_UPPER, "", r, false, "");
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, "VV", EnableVV);
			AddScreenObject(BUTTON_VV, "", r, false, "");
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, to_string(VV_Minutes) + "min", false);
			AddScreenObject(BUTTON_VV_TIME, "", r, false, "");
			StartOfMenu.x = r.right;
			r = this->DrawMenuBarButton(&dc, StartOfMenu, "APP", ApproachMode);
			AddScreenObject(BUTTON_APPROACH, "", r, false, "");
		}

		// Drawing of AC Symbols and trails
		if (Phase == EuroScopePlugIn::REFRESH_PHASE_BEFORE_TAGS) {
			// Menubar

			// We stop here if no trag drawings
			if (!EnableTagDrawings)
				goto close_all;

			// Ac Symvols

			int SymbolSize = toml::find<int>(this->CoFrancepluginInstance->CoFranceConfig, "ac_symbols", "size");

			for (CRadarTarget radarTarget = GetPlugIn()->RadarTargetSelectFirst(); radarTarget.IsValid();
				radarTarget = GetPlugIn()->RadarTargetSelectNext(radarTarget))
			{

				// We skip invalid targets
				if (!radarTarget.IsValid() || (!radarTarget.GetPosition().GetTransponderC() && !radarTarget.GetPosition().GetTransponderI()))
					continue;


				// If we have an aircaft assumed, we force it through the filters
				bool owned_by_me = radarTarget.GetCorrelatedFlightPlan().GetTrackingControllerIsMe();

				// We skip targets slower than 40kts
				if (radarTarget.GetPosition().GetReportedGS() < 40)
					continue;

				if (EnableFilters && !owned_by_me) {
					if (radarTarget.GetPosition().GetFlightLevel() < Filter_Lower || radarTarget.GetPosition().GetFlightLevel() > Filter_Upper)
						continue;
				}

				Color AcColor = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "ac_not_concerned"));

				CFlightPlan CorrFp = radarTarget.GetCorrelatedFlightPlan();
				if (CorrFp.IsValid()) {
					if (CorrFp.GetState() == FLIGHT_PLAN_STATE_COORDINATED)
						AcColor = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "ac_notified"));

					if (CorrFp.GetState() == FLIGHT_PLAN_STATE_TRANSFER_TO_ME_INITIATED || CorrFp.GetState() == FLIGHT_PLAN_STATE_TRANSFER_FROM_ME_INITIATED)
						AcColor = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "ac_transferred"));

					if (CorrFp.GetState() == FLIGHT_PLAN_STATE_ASSUMED)
						AcColor = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "ac_assumed"));

					if (CorrFp.GetState() == FLIGHT_PLAN_STATE_REDUNDANT)
						AcColor = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "ac_redundant"));

					if (this->CoFrancepluginInstance->ConflictGroups.find(CorrFp.GetCallsign()) != this->CoFrancepluginInstance->ConflictGroups.end()) {
						AcColor.SetFromCOLORREF(GetConflictGroupColor(this->CoFrancepluginInstance->CoFranceConfig, this->CoFrancepluginInstance->ConflictGroups[CorrFp.GetCallsign()]));
					}
				}

				if (this->CoFrancepluginInstance->DetailedAircraft == string(radarTarget.GetCallsign())) {
					AcColor = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "ac_redundant"));
				}

				// If we are in a conflict group, we show a forced leader line of 2 minutes
				// We also displayed speed vectors if turned on
				if (this->CoFrancepluginInstance->ConflictGroups.find(CorrFp.GetCallsign()) != this->CoFrancepluginInstance->ConflictGroups.end() || EnableVV) {
					
					CPosition EndOfLine = Extrapolate(radarTarget.GetPosition().GetPosition(), radarTarget.GetTrackHeading(), radarTarget.GetPosition().GetReportedGS() * 0.0166667 * VV_Minutes);

					POINT ptRad = ConvertCoordFromPositionToPixel(radarTarget.GetPosition().GetPosition());
					Point pAcPosition(ptRad.x, ptRad.y);
					POINT PtEndOfLine = ConvertCoordFromPositionToPixel(EndOfLine);
					Point pEndOfLinePt(PtEndOfLine.x, PtEndOfLine.y);

					g.DrawLine(&Pen(AcColor, 1.5f), pAcPosition, pEndOfLinePt);
				}

				// We can now draw the ac symbol
				POINT TargetCenter = ConvertCoordFromPositionToPixel(radarTarget.GetPosition().GetPosition());
				g.DrawEllipse(&Pen(AcColor), Rect(TargetCenter.x - SymbolSize, TargetCenter.y - SymbolSize, SymbolSize * 2, SymbolSize * 2));
				
				int TrailSize = SymbolSize;
				int NumberOfTrails = toml::find<int>(this->CoFrancepluginInstance->CoFranceConfig, "ac_symbols", "number_of_trails");

				CRadarTargetPositionData previousPos;
				// if Approach Mode we cut it in half
				if (ApproachMode)
					previousPos = radarTarget.GetPreviousPosition(radarTarget.GetPosition());
				else
					previousPos = radarTarget.GetPreviousPosition(radarTarget.GetPreviousPosition(radarTarget.GetPosition()));

				CPosition compareToPos = radarTarget.GetPosition().GetPosition();
				for (int j = 0; j < NumberOfTrails; j++) {
					POINT pCoordNative = ConvertCoordFromPositionToPixel(previousPos.GetPosition());
					Point pCoord(pCoordNative.x, pCoordNative.y);

					int LengthOfLine = TrailSize * 2;
					LengthOfLine = LengthOfLine - ((TrailSize*2) / NumberOfTrails) * j;

					if (LengthOfLine <= 0)
						continue;

					Point LeftSide, RightSide;
					LeftSide.X = pCoord.X - LengthOfLine / 2;
					LeftSide.Y = pCoord.Y;

					RightSide.X = pCoord.X + LengthOfLine / 2;
					RightSide.Y = pCoord.Y;

					LeftSide = rotatePoint(pCoord, DegToRad(previousPos.GetPosition().DirectionTo(compareToPos)), LeftSide);
					RightSide = rotatePoint(pCoord, DegToRad(previousPos.GetPosition().DirectionTo(compareToPos)), RightSide);

					g.DrawLine(&Pen(AcColor), LeftSide, RightSide);

					compareToPos = previousPos.GetPosition();
					if (ApproachMode)
						previousPos = radarTarget.GetPreviousPosition(radarTarget.GetPosition());
					else
						previousPos = radarTarget.GetPreviousPosition(radarTarget.GetPreviousPosition(previousPos));
				}
			}
		}
	} 
	catch (const std::exception& exc) {
		Log(exc.what());
		GetPlugIn()->DisplayUserMessage("Message", "CoFrance PlugIn", string("Error parsing file config file " + string(exc.what())).c_str(), false, false, false, false, false);
	}

close_all:

	dc.Detach();
	g.ReleaseHDC(hDC);
}

void RadarScreen::OnOverScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area)
{
	MousePt = Pt;
	RequestRefresh();
}

void RadarScreen::OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button)
{
	MousePt = Pt;

	if (ObjectType == BUTTON_FILTRES)
		EnableFilters = !EnableFilters;

	if (ObjectType == BUTTON_RAD)
		EnableTagDrawings = !EnableTagDrawings;

	if (ObjectType == BUTTON_VV)
		EnableVV = !EnableVV;

	if (ObjectType == BUTTON_APPROACH)
		ApproachMode = !ApproachMode;

	if (ObjectType == BUTTON_VV_TIME) {
		GetPlugIn()->OpenPopupList(Area, "Vecteur Vitesse", 1);
		for (int i = 1; i <= 9; i++)
			GetPlugIn()->AddPopupListElement(string(string(" ") + to_string(i) + string(" ")).c_str(), "", FUNCTION_SET_VV_TIME);
	}

	if (ObjectType == BUTTON_FILTRES_LOWER) {
		GetPlugIn()->OpenPopupList(Area, "Filtres Inferieurs", 1);
		FillInAltitudeList(GetPlugIn(), FUNCTION_SET_LOWER_FILTER, Filter_Lower);
	}

	if (ObjectType == BUTTON_FILTRES_UPPER) {
		GetPlugIn()->OpenPopupList(Area, "Filtres Superieurs", 1);
		FillInAltitudeList(GetPlugIn(), FUNCTION_SET_HIGHER_FILTER, Filter_Upper);
	}

	RequestRefresh();
}

void RadarScreen::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
	if (FunctionId == FUNCTION_SET_VV_TIME) {
		VV_Minutes = atoi(sItemString);
	}

	if (FunctionId == FUNCTION_SET_LOWER_FILTER) {
		Filter_Lower = atoi(sItemString) * 100;
	}

	if (FunctionId == FUNCTION_SET_HIGHER_FILTER) {
		Filter_Upper = atoi(sItemString) * 100;
	}
}
