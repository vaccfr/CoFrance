#pragma once
#include "pch.h"
#include <string>
#include "Constants.h"

class DYPWindow {
protected:
	const static int WIDTH = 650;
	const static int HEIGHT = 20 + 15 + 20 + 40 + 25;

	bool initialise = true;
public:

	const static int DRAW_DYP_WINDOW = 1001;
	const static int FUNC_DYP_WINDOW_TABS = 1201;
	Gdiplus::Point TopLeft = {200, 400};
	
	enum Tabs { PRINCIPAL, ROUTE, OCL, MISC };

	Tabs active_tab = PRINCIPAL;
	Tabs hovered_tab = PRINCIPAL;
	string active_ac = "";


	inline void Reset() {
		active_ac = "";
		initialise = true;
	};

	inline void Draw(Gdiplus::Graphics* g, CDC* dc, CRadarScreen* radar, POINT MousePt) {
		int svDc = dc->SaveDC();
		dc->SetTextColor(RGB(255, 255, 255));
		int y_Cursor = 0;


		CFlightPlan fp = radar->GetPlugIn()->FlightPlanSelect(active_ac.c_str());

		if (!fp.IsValid())
			return;

		g->FillRectangle(&SolidBrush(StaticColours::ListBackground), Rect(TopLeft, Size(WIDTH, HEIGHT)));
		g->DrawRectangle(&Pen(StaticColours::ListForeground), Rect(TopLeft, Size(WIDTH, HEIGHT)));

		y_Cursor = 20;
		g->FillRectangle(&HatchBrush(HatchStyle::HatchStyleDarkUpwardDiagonal, StaticColours::ListForeground, StaticColours::ListBackground), Rect(TopLeft, Size(WIDTH, y_Cursor)));
		dc->TextOutA(TopLeft.X+4, TopLeft.Y+4, "DyP Info");

		radar->AddScreenObject(DRAW_DYP_WINDOW, "", CRect({ TopLeft.X, TopLeft.Y }, CSize(WIDTH, HEIGHT)), true, "");

		// Tabs
		CSize measureSize = dc->GetTextExtent("ABC1111 FRANCE SOLEIL");
		if (active_tab == PRINCIPAL || hovered_tab == PRINCIPAL) {
			measureSize = dc->GetTextExtent("PRINCIPAL");
			g->FillRectangle(&SolidBrush(StaticColours::ListForeground), Rect(Point(TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor), Size(measureSize.cx, measureSize.cy)));
		}

		if (active_tab == ROUTE || hovered_tab == ROUTE) {
			measureSize = dc->GetTextExtent("PRINCIPAL ");
			CSize measureSize2 = dc->GetTextExtent("ROUTE");
			g->FillRectangle(&SolidBrush(StaticColours::ListForeground), Rect(Point(TopLeft.X + 4 + measureSize.cx, TopLeft.Y + 4 + y_Cursor), Size(measureSize2.cx, measureSize2.cy)));
		}
		 
		if (active_tab == OCL || hovered_tab == OCL) {
			measureSize = dc->GetTextExtent("PRINCIPAL ROUTE ");
			CSize measureSize2 = dc->GetTextExtent("OCL");
			g->FillRectangle(&SolidBrush(StaticColours::ListForeground), Rect(Point(TopLeft.X + 4 + measureSize.cx, TopLeft.Y + 4 + y_Cursor), Size(measureSize2.cx, measureSize2.cy)));
		}

		// Add the screen objects for click
		measureSize = dc->GetTextExtent("PRINCIPAL");
		radar->AddScreenObject(FUNC_DYP_WINDOW_TABS, "principal", CRect({ TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor }, measureSize), false, "");

		measureSize = dc->GetTextExtent("PRINCIPAL ROUTE ");
		CSize measureSize2 = dc->GetTextExtent("OCL");
		radar->AddScreenObject(FUNC_DYP_WINDOW_TABS, "ocl", CRect({ TopLeft.X + 4 + measureSize.cx, TopLeft.Y + 4 + y_Cursor }, measureSize2), false, "");
		

		dc->TextOutA(TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor, "PRINCIPAL ROUTE OCL MISC");
		y_Cursor += 20;



		// Callsign and company and first set of information
		g->DrawLine(&Pen(Gdiplus::Color::Black), Point(TopLeft.X, TopLeft.Y + y_Cursor), Point(TopLeft.X + WIDTH, TopLeft.Y + y_Cursor));

		string s = fp.GetCallsign();
		s += " company";
		dc->TextOutA(TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor, s.c_str());
		measureSize = dc->GetTextExtent("ABC1111 FRANCE SOLEIL"); 

		s = string(fp.GetControllerAssignedData().GetSquawk()) + " pssr sts";
		s += "    x1   " + string(fp.GetFlightPlanData().GetAircraftFPType()) + " /" + fp.GetFlightPlanData().GetAircraftWtc() + " K0" + to_string(fp.GetFlightPlanData().GetTrueAirspeed());
		
		// special flags
		if (!fp.GetFlightPlanData().IsRvsm())
			s += "      noW";


		dc->TextOutA(TopLeft.X + 4 + measureSize.cx, TopLeft.Y + 4 + y_Cursor, s.c_str());

		// Vertical sep

		y_Cursor += 20;

		// Main data area
		g->FillRectangle(&SolidBrush(StaticColours::ListLightForeground), Rect(Point(TopLeft.X+1, TopLeft.Y + y_Cursor), Size(WIDTH-2, 40)));
		g->DrawLine(&Pen(Gdiplus::Color::Black), Point(TopLeft.X, TopLeft.Y + y_Cursor), Point(TopLeft.X + WIDTH, TopLeft.Y + y_Cursor));

		if (active_tab == Tabs::PRINCIPAL) {
			string line0 = "";
			if (string(fp.GetEntryCoordinationPointName()).size() > 0) {
				line0 += string(fp.GetEntryCoordinationPointName()) + " ";
				line0 += to_string(fp.GetEntryCoordinationAltitude() / 100);
			}
			else {
				line0 += "copn efl";
			}

			line0 += "  " + string(fp.GetFlightPlanData().GetOrigin()) + " " + string(fp.GetFlightPlanData().GetDestination()) + "  " + to_string(fp.GetFlightPlanData().GetFinalAltitude()/100)+"    ";

			// We then show the next 3 sectors
			string id = "";
			int k = 0;
			CFlightPlanPositionPredictions PosPred = fp.GetPositionPredictions();
			for (int i = 0; i < PosPred.GetPointsNumber(); i++) {

				// New controller, add it
				if (id != PosPred.GetControllerId(i) && PosPred.GetControllerId(i) != "--") {

					line0 += string(PosPred.GetControllerId(i)) + " " + to_string(PosPred.GetAltitude(i) / 100) + "   ";

					id = PosPred.GetControllerId(i);
					k++;
					if (k >= 4)
						break;
				}

			}

			dc->TextOutA(TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor, line0.c_str());

			string line0end = "";
			if (string(fp.GetExitCoordinationPointName()).size() > 0) {
				line0end += string(fp.GetExitCoordinationPointName()) + " ";
				line0end += to_string(fp.GetExitCoordinationAltitude() / 100);
			}
			else {
				line0end += "copx xfl";
			}
			measureSize = dc->GetTextExtent("line0end");
			dc->TextOutA(TopLeft.X + WIDTH - 4 - measureSize.cx, TopLeft.Y + 4 + y_Cursor, line0end.c_str());

			
		}

		if (active_tab == Tabs::ROUTE) {

		}

		if (active_tab == Tabs::OCL) {
			string line0 = "no OCL";
			if (HasOCL(fp.GetCallsign())) {
				line0 = GetFullOCL(fp.GetCallsign());
			}

			dc->TextOutA(TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor, line0.c_str());
		}
		

		y_Cursor += 40;
		g->DrawLine(&Pen(Gdiplus::Color::Black), Point(TopLeft.X, TopLeft.Y + y_Cursor), Point(TopLeft.X + WIDTH, TopLeft.Y + y_Cursor));

		// Bottom area

		s = string(fp.GetTrackingControllerId()) + "  " + to_string(radar->GetPlugIn()->ControllerSelectByPositionId(fp.GetTrackingControllerId()).GetPrimaryFrequency()).substr(0, 7);
		if (string(fp.GetTrackingControllerId()).size() == 0)
			s = "cs  cs---.---";
		dc->TextOutA(TopLeft.X + 4, TopLeft.Y + 4 + y_Cursor, s.c_str());
		measureSize = dc->GetTextExtent("EURW  111.111   ");


		s = "@"+string(fp.GetCallsign())+" @"+string(fp.GetCorrelatedRadarTarget().GetPosition().GetSquawk())+" @h"+ padWithZeros(3, (int)fmod(fp.GetCorrelatedRadarTarget().GetPosition().GetReportedHeading(), 360));
		dc->TextOutA(TopLeft.X + 4 + measureSize.cx, TopLeft.Y + 4 + y_Cursor, s.c_str());
		measureSize = dc->GetTextExtent("@ABC1111 @1000                       ");

		s = "optext";
		if (string(fp.GetControllerAssignedData().GetScratchPadString()).size() > 0)
			s = string(fp.GetControllerAssignedData().GetScratchPadString()).substr(0, 40);
		dc->TextOutA(TopLeft.X + 4 + measureSize.cx, TopLeft.Y + 4 + y_Cursor, s.c_str());
		

		CController ns = radar->GetPlugIn()->ControllerSelect(fp.GetCoordinatedNextController());
		s = string(ns.GetPositionId()) + "  " + to_string(ns.GetPrimaryFrequency()).substr(0, 7);
		if (string(ns.GetPositionId()).size() == 0)
			s = "ns  ns---.---";

		measureSize = dc->GetTextExtent(s.c_str());
		dc->TextOutA(TopLeft.X + WIDTH - 4 - measureSize.cx, TopLeft.Y + 4 + y_Cursor, s.c_str());

		/*dc->SetTextColor(COLORREF(RGB(209, 209, 209)));
		y_Cursor += DrawCenteredText(dc, { x_Center , TopRight.Y + y_Cursor }, "ASP");
		y_Cursor += DrawCenteredText(dc, { x_Center , TopRight.Y + y_Cursor }, fp.GetCallsign());

		y_Cursor = 35 + 2;
		string calculated_speed = "@";
		if (is_mach)
			calculated_speed += "m." + to_string(perf_mach);
		else
			calculated_speed += "k" + to_string(perf_ias);
		y_Cursor += DrawCenteredText(dc, { x_Center , TopRight.Y + y_Cursor }, calculated_speed.c_str());
		y_Cursor += 3;

		Gdiplus::GraphicsPath* pt = RoundedRect(Rect(TopRight.X + 2, TopRight.Y + y_Cursor, WIDTH - 4, 20 * 8), 3);
		g->DrawPath(&Pen(StaticColours::ListForeground), pt);
		g->FillPath(&SolidBrush(StaticColours::SelectListBackground), pt);

		// If we have an assigned speed, we center on that
		if (fp.GetControllerAssignedData().GetAssignedMach() != 0)
			perf_mach = fp.GetControllerAssignedData().GetAssignedMach();

		if (fp.GetControllerAssignedData().GetAssignedSpeed() != 0)
			perf_ias = fp.GetControllerAssignedData().GetAssignedSpeed();


		y_Cursor += 7;
		int range_min = 0;
		int range_max = 0;
		int increment = 10;

		if (is_mach) {
			range_min = perf_mach - 3;
			range_max = perf_mach + 3;
			increment = 1;
		}
		else {
			range_min = roundUp(perf_ias, 10) - 30;
			range_max = roundUp(perf_ias, 10) + 30;
		}

		for (int i = range_max; i >= range_min; i = i - increment) {
			string str_version = std::to_string(i);

			if (hovered_item == str_version) {
				g->FillRectangle(&SolidBrush(StaticColours::SelectActiveListBackground), Rect(TopRight.X + 2, TopRight.Y + y_Cursor - 3, WIDTH - 4, 20));
			}
			radar->AddScreenObject(CoFranceTags::FUNCTION_ASP_TOOL_LIST, str_version.c_str(), CRect({ TopRight.X + 2, TopRight.Y + y_Cursor - 3 }, CSize(WIDTH - 4, 20)), false, "");

			y_Cursor += DrawCenteredText(dc, { x_Center , TopRight.Y + y_Cursor }, is_mach ? "." + str_version : str_version) + 7;
		}

		y_Cursor += 5;
		// min button
		int button_x = TopRight.X + 5;
		int button_width = WIDTH / 2 - 10;
		pt = RoundedRect(Rect(button_x, TopRight.Y + y_Cursor, button_width, 20), 3);
		g->DrawPath(&Pen(StaticColours::ListForeground), pt);
		if (min_active) {
			g->FillPath(&SolidBrush(StaticColours::SelectActiveListBackground), pt);
		}
		else {
			g->FillPath(&SolidBrush(StaticColours::ListBackground), pt);
		}
		DrawCenteredText(dc, { button_x + button_width / 2 , TopRight.Y + y_Cursor + 2 }, "min");
		radar->AddScreenObject(CoFranceTags::FUNCTION_ASP_TOOL_MIN, "", CRect({ button_x, TopRight.Y + y_Cursor }, CSize(button_width, 20)), false, "");

		// max button
		button_x += button_width + 10;
		pt = RoundedRect(Rect(button_x, TopRight.Y + y_Cursor, button_width, 20), 3);
		g->DrawPath(&Pen(StaticColours::ListForeground), pt);
		if (max_active) {
			g->FillPath(&SolidBrush(StaticColours::SelectActiveListBackground), pt);
		}
		else {
			g->FillPath(&SolidBrush(StaticColours::ListBackground), pt);
		}
		DrawCenteredText(dc, { button_x + button_width / 2, TopRight.Y + y_Cursor + 2 }, "max");
		radar->AddScreenObject(CoFranceTags::FUNCTION_ASP_TOOL_MAX, "", CRect({ button_x, TopRight.Y + y_Cursor }, CSize(button_width, 20)), false, "");

		// Resume
		y_Cursor += 25;
		button_width = WIDTH - 10;
		button_x = TopRight.X + 5;

		pt = RoundedRect(Rect(button_x, TopRight.Y + y_Cursor, button_width, 20), 3);
		g->DrawPath(&Pen(StaticColours::ListForeground), pt);
		g->FillPath(&SolidBrush(StaticColours::ListBackground), pt);
		DrawCenteredText(dc, { button_x + button_width / 2, TopRight.Y + y_Cursor + 2 }, "Resume");

		radar->AddScreenObject(CoFranceTags::FUNCTION_ASP_TOOL_RESUME, "", CRect({ button_x, TopRight.Y + y_Cursor }, CSize(button_width, 20)), false, "");

		// K/M toggle
		y_Cursor += 25;
		button_width = 20;
		button_x = TopRight.X + 5;

		pt = RoundedRect(Rect(button_x, TopRight.Y + y_Cursor, button_width, 20), 3);
		g->DrawPath(&Pen(StaticColours::ListForeground), pt);
		g->FillPath(&SolidBrush(StaticColours::ListBackground), pt);
		DrawCenteredText(dc, { button_x + button_width / 2, TopRight.Y + y_Cursor + 2 }, is_mach ? "M" : "K");

		radar->AddScreenObject(CoFranceTags::FUNCTION_ASP_TOOL_TOGGLE_M, "", CRect({ button_x, TopRight.Y + y_Cursor }, CSize(button_width, 20)), false, "");

		// Manual input
		button_width += 2;
		g->DrawRectangle(&Pen(StaticColours::ListForeground, 2.0f), Rect(button_x + button_width, TopRight.Y + y_Cursor + 1, WIDTH - button_width - 12, 18));
		*/
		dc->RestoreDC(svDc);
	};
};