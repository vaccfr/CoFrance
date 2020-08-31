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
					if (find(AllControllers.begin(), AllControllers.end(), c) != AllControllers.end()) {
						DrawText = true; 
						break;
					}
						
				}

				// Check if any of the controllers which hide the text are online
				if (v.contains("controller_not_others")) {
					auto notNeededControllers = toml::find<vector<string>>(v, "controller_not_others");
					for (const auto& c : notNeededControllers)
					{
						if (find(AllControllers.begin(), AllControllers.end(), c) != AllControllers.end()) {
							DrawText = false; 
							break;
						}
							
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

			for (auto kv : ToAddAcSymbolScreenObject) {
				AddScreenObject(AC_SYMBOL, kv.first.c_str(), kv.second, false, "");
			}
		}

		// Drawing of AC Symbols and trails
		if (Phase == EuroScopePlugIn::REFRESH_PHASE_BEFORE_TAGS) {
			// Menubar

			// We stop here if no trag drawings
			if (!EnableTagDrawings)
				goto close_all;


			Color SepToolColour = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "sep_tool"));
			Pen SepToolDashPen(SepToolColour);
			Pen SepToolPen(SepToolColour);
			REAL dashVals[2] = { 4.0f, 4.0f };
			SepToolDashPen.SetDashPattern(dashVals, 2);

			Color SepToolBackground = vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "sep_background"));
			Pen SepToolBorder(vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "sep_border")));

			int saveDcActiveSepTool = dc.SaveDC();
			dc.SetTextColor(SepToolColour.ToCOLORREF());

			int SymbolSize = toml::find<int>(this->CoFrancepluginInstance->CoFranceConfig, "ac_symbols", "size");

			// Sep tool drawing
			if (FirstSepToolCallsign.size() > 0) {
				CRadarTarget rt = GetPlugIn()->RadarTargetSelect(FirstSepToolCallsign.c_str());
				
				POINT AcPosPix = ConvertCoordFromPositionToPixel(rt.GetPosition().GetPosition());
				g.DrawLine(&SepToolDashPen, Point(AcPosPix.x, AcPosPix.y), Point(MousePt.x, MousePt.y));

				// Angled Pointer Around the mouse
				Rect MousePointer(Point(MousePt.x-20, MousePt.y-20), Size(40, 40));
				Rect MousePointerCenter(Point(MousePt.x-3, MousePt.y-3), Size(6, 6));
				
				g.DrawLine(&SepToolBorder, Point(MousePointerCenter.GetLeft(), MousePointerCenter.GetTop()), Point(MousePointer.GetLeft(), MousePointer.GetTop()));
				g.DrawLine(&SepToolBorder, Point(MousePointerCenter.GetLeft(), MousePointerCenter.GetBottom()), Point(MousePointer.GetLeft(), MousePointer.GetBottom()));
				g.DrawLine(&SepToolBorder, Point(MousePointerCenter.GetRight(), MousePointerCenter.GetTop()), Point(MousePointer.GetRight(), MousePointer.GetTop()));
				g.DrawLine(&SepToolBorder, Point(MousePointerCenter.GetRight(), MousePointerCenter.GetBottom()), Point(MousePointer.GetRight(), MousePointer.GetBottom()));

				RequestRefresh();
			}


			for (auto kv : ActiveSepTools) {
				CRadarTarget FirstTarget = GetPlugIn()->RadarTargetSelect(kv.first.c_str());
				CRadarTarget SecondTarget = GetPlugIn()->RadarTargetSelect(kv.second.c_str());

				if (FirstTarget.IsValid() && SecondTarget.IsValid()) {
					CPosition FirstTargetPos = FirstTarget.GetPosition().GetPosition();
					CPosition SecondTargetPos = SecondTarget.GetPosition().GetPosition();

					POINT FirstPos = ConvertCoordFromPositionToPixel(FirstTargetPos);
					POINT SecondPos = ConvertCoordFromPositionToPixel(SecondTargetPos);

					g.DrawLine(&SepToolDashPen, Point(FirstPos.x, FirstPos.y), Point(SecondPos.x, SecondPos.y));

					// First the distance tool

					string headingText = padWithZeros(3, (int)FirstTargetPos.DirectionTo(SecondTargetPos));

					string distanceText = to_string(FirstTargetPos.DistanceTo(SecondTargetPos));
					size_t decimal_pos = distanceText.find(".");
					distanceText = distanceText.substr(0, decimal_pos + 2)+"Nm";

					distanceText = distanceText + " " + headingText + "°";

					POINT MidPointDistance = { (int)((FirstPos.x + SecondPos.x) / 2), (int)((FirstPos.y + SecondPos.y) / 2) };

					CSize Measure = dc.GetTextExtent(distanceText.c_str());

					// We have to calculate the text angle 
					double AdjustedLineHeading = fmod(FirstTargetPos.DirectionTo(SecondTargetPos) - 90.0, 360.0);
					double NewAngle = fmod(AdjustedLineHeading + 90, 360);

					if (FirstTargetPos.DirectionTo(SecondTargetPos) > 180.0) {
						NewAngle = fmod(AdjustedLineHeading - 90, 360);
					}

					POINT TextPositon;
					TextPositon.x = long(MidPointDistance.x + float((20 + Measure.cx) * cos(DegToRad(NewAngle))));
					TextPositon.y = long(MidPointDistance.y + float((20) * sin(DegToRad(NewAngle))));

					TextPositon.x -= Measure.cx / 2;

					CRect AreaRemoveTool = { TextPositon.x, TextPositon.y, TextPositon.x + Measure.cx, TextPositon.y + Measure.cy };
					
					VERA::VERADataStruct vera = VERA::Calculate(FirstTarget, SecondTarget, toml::find<int>(this->CoFrancepluginInstance->CoFranceConfig, "sep", "lookup_time"));

					string veraDistanceText = "";
					if (vera.minDistanceNm != -1) {
						veraDistanceText = to_string(vera.minDistanceNm);
						decimal_pos = veraDistanceText.find(".");
						veraDistanceText = veraDistanceText.substr(0, decimal_pos + 2) + "Nm";

						veraDistanceText = veraDistanceText + " " + to_string((int)vera.minDistanceSeconds / 60) + "'" +
							to_string((int)vera.minDistanceSeconds % 60) + '"';

						Measure = dc.GetTextExtent(veraDistanceText.c_str());

						AreaRemoveTool.right = max(AreaRemoveTool.right, TextPositon.x + Measure.cx);
						AreaRemoveTool.bottom = TextPositon.y + Measure.cy * 2;
						AreaRemoveTool.right += 10;
						AreaRemoveTool.left -= 10;
						AreaRemoveTool.InflateRect(4, 4);

						if (IsInRect(MousePt, AreaRemoveTool)) {
							Point FirstPosPredictedPt(ConvertCoordFromPositionToPixel(vera.predictedFirstPos).x, ConvertCoordFromPositionToPixel(vera.predictedFirstPos).y);
							g.DrawLine(&SepToolPen, Point(FirstPos.x, FirstPos.y), FirstPosPredictedPt);
							g.FillEllipse(&SolidBrush(SepToolColour), Rect(FirstPosPredictedPt.X - SymbolSize, FirstPosPredictedPt.Y - SymbolSize, SymbolSize * 2, SymbolSize * 2));
							Point SecondPosPredictedPt(ConvertCoordFromPositionToPixel(vera.predictedSecondPos).x, ConvertCoordFromPositionToPixel(vera.predictedSecondPos).y);
							g.DrawLine(&SepToolPen, Point(SecondPos.x, SecondPos.y), SecondPosPredictedPt);
							g.FillEllipse(&SolidBrush(SepToolColour), Rect(SecondPosPredictedPt.X - SymbolSize, SecondPosPredictedPt.Y - SymbolSize, SymbolSize * 2, SymbolSize * 2));
						}
					}
					else {
						AreaRemoveTool.right += 10;
						AreaRemoveTool.left -= 10;
						AreaRemoveTool.InflateRect(4, 4);
					}

					POINT ClipFrom, ClipTo;
					if (LiangBarsky(AreaRemoveTool, MidPointDistance, AreaRemoveTool.CenterPoint(), ClipFrom, ClipTo))
						g.DrawLine(&SepToolPen, Point(MidPointDistance.x, MidPointDistance.y), Point(ClipFrom.x, ClipFrom.y));

					if (IsInRect(MousePt, AreaRemoveTool)) {
						
						Rect ConcernedRect = Rect(AreaRemoveTool.left, AreaRemoveTool.top, AreaRemoveTool.Width(), AreaRemoveTool.Height());
						g.FillRectangle(&SolidBrush(SepToolBackground), ConcernedRect);
						dc.Draw3dRect(AreaRemoveTool, StaticColours::MenuButtonTop.ToCOLORREF(), StaticColours::MenuButtonBottom.ToCOLORREF());

						if (vera.minDistanceNm != -1)
							g.DrawLine(&Pen(StaticColours::MenuButtonTop), Point(AreaRemoveTool.left, AreaRemoveTool.top-1 + AreaRemoveTool.Height() /2), Point(AreaRemoveTool.right, AreaRemoveTool.top-1 + AreaRemoveTool.Height() / 2));
						
					}

					dc.TextOutA(TextPositon.x, TextPositon.y, distanceText.c_str());

					if (vera.minDistanceNm < toml::find<int>(this->CoFrancepluginInstance->CoFranceConfig, "sep", "warning_threshold") && vera.minDistanceNm != -1) {
						dc.SetTextColor(vectorToGdiplusColour(toml::find<std::vector<int>>(this->CoFrancepluginInstance->CoFranceConfig, "colours", "sep_warning")).ToCOLORREF());
					}
					else {
						dc.SetTextColor(SepToolColour.ToCOLORREF());
					}

					dc.TextOutA(TextPositon.x, TextPositon.y + Measure.cy + 3, veraDistanceText.c_str());

					AddScreenObject(SCREEN_SEP_TOOL, string(kv.first + "," + kv.second).c_str(), AreaRemoveTool, false, "");
				}
			}

			dc.RestoreDC(saveDcActiveSepTool);

			// Ac Symvols

			

			ToAddAcSymbolScreenObject.clear();

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
					if (radarTarget.GetPosition().GetFlightLevel() <= Filter_Lower || radarTarget.GetPosition().GetFlightLevel() >= Filter_Upper)
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

				// If we are in a conflict group, we show a forced leader line
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
				Rect SymbolRect = Rect(TargetCenter.x - SymbolSize, TargetCenter.y - SymbolSize, SymbolSize * 2, SymbolSize * 2);
				g.DrawEllipse(&Pen(AcColor), SymbolRect);

				ToAddAcSymbolScreenObject.insert(make_pair(radarTarget.GetCallsign(), CRect(SymbolRect.GetLeft(), SymbolRect.GetTop(), SymbolRect.GetRight(), SymbolRect.GetBottom())));
				
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
						previousPos = radarTarget.GetPreviousPosition(previousPos);
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

void RadarScreen::OnAsrContentToBeClosed()
{
	delete this;
}

void RadarScreen::OnAsrContentToBeSaved()
{
	SaveDataToAsr(SaveData_RadarDrawing, "CoFrance Radar Drawings", EnableTagDrawings ? "1" : "0");
	SaveDataToAsr(SaveData_Filters, "CoFrance Filters Enabled", EnableFilters ? "1" : "0");
	SaveDataToAsr(SaveData_VVEnabled, "CoFrance Vecteur Vitesse Enabled", EnableVV ? "1" : "0");

	SaveDataToAsr(SaveData_FiltersAbove, "CoFrance Filters Above", to_string(Filter_Upper).c_str());
	SaveDataToAsr(SaveData_FiltersBelow, "CoFrance Filters Below", to_string(Filter_Lower).c_str());
	SaveDataToAsr(SaveData_VVTime, "CoFrance Vecteur Vitesse Time", to_string(VV_Minutes).c_str());
}

void RadarScreen::OnAsrContentLoaded(bool Loaded)
{
	if (!Loaded)
		return;

	const char* j_value;
	if ((j_value = GetDataFromAsr(SaveData_RadarDrawing)) != NULL)
		EnableTagDrawings = (j_value == "1");

	if ((j_value = GetDataFromAsr(SaveData_Filters)) != NULL)
		EnableFilters = (j_value == "1");

	if ((j_value = GetDataFromAsr(SaveData_VVEnabled)) != NULL)
		EnableVV = (j_value == "1");

	if ((j_value = GetDataFromAsr(SaveData_FiltersAbove)) != NULL)
		Filter_Upper = stoi(j_value);
	if ((j_value = GetDataFromAsr(SaveData_FiltersBelow)) != NULL)
		Filter_Lower = stoi(j_value);
	if ((j_value = GetDataFromAsr(SaveData_VVTime)) != NULL)
		VV_Minutes = stoi(j_value);
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

	if (ObjectType == AC_SYMBOL) {
		if (FirstSepToolCallsign.size() > 0 && FirstSepToolCallsign != string(sObjectId)) {
			ActiveSepTools.insert(make_pair(FirstSepToolCallsign, sObjectId));
			FirstSepToolCallsign = "";
		}
		else if (FirstSepToolCallsign == string(sObjectId)) {
			FirstSepToolCallsign = "";
		}
		else {
			FirstSepToolCallsign = sObjectId;
		}
			
	}

	if (ObjectType == SCREEN_SEP_TOOL) {
		vector<string> s = split(sObjectId, ',');
		pair<string, string> toRemove = pair<string, string>(s.front(), s.back());

		typedef multimap<string, string>::iterator iterator;
		std::pair<iterator, iterator> iterpair = ActiveSepTools.equal_range(toRemove.first);

		iterator it = iterpair.first;
		for (; it != iterpair.second; ++it) {
			if (it->second == toRemove.second) {
				it = ActiveSepTools.erase(it);
				break;
			}
		}
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
