#pragma once
#include "pch.h"
#include <string>
#include "Constants.h"

class PopupSpeedAssign {
protected:
	const static int WIDTH = 100;
	const static int HEIGHT = 8 * 20 + 50 + 20 + 20 + 20 + 25;

	bool initialise = true;

public:
	std::chrono::system_clock::time_point LeftAt;

	bool min_active = false;
	bool max_active = false;

	bool is_mach = false;

	string hovered_item = "";
	string active_ac = "";


	Gdiplus::Point Center = Point(200, 400);

	inline void Reset() {
		active_ac = "";
		min_active = false;
		max_active = false;
		hovered_item = "";
		LeftAt = std::chrono::system_clock::now();
		is_mach = false;
		initialise = true;
	};

	inline void Draw(Gdiplus::Graphics* g, CDC* dc, CRadarScreen* radar, POINT MousePt) {
		int svDc = dc->SaveDC();
		int y_Cursor = 0;


		CFlightPlan fp = radar->GetPlugIn()->FlightPlanSelect(active_ac.c_str());

		if (!fp.IsValid())
			return;


		if (initialise) {
			if (fp.GetCorrelatedRadarTarget().IsValid() && fp.GetCorrelatedRadarTarget().GetPosition().GetPressureAltitude() >= 28000) {
				is_mach = true;
			}

			int aspeed = fp.GetControllerAssignedData().GetAssignedSpeed();

			if (aspeed == 1)
				min_active = true;

			if (aspeed == 2)
				max_active = true;

			if (aspeed != 0 && aspeed % 10 == 9)
				max_active = true;

			if (aspeed != 0 && aspeed % 10 == 1)
				min_active = true;

			initialise = false;
		}


		int Tendency = 0;
		if (fp.GetCorrelatedRadarTarget().GetVerticalSpeed() > 100)
			Tendency = 1;
		if (fp.GetCorrelatedRadarTarget().GetVerticalSpeed() < 100)
			Tendency = -1;

		int perf_mach = fp.GetFlightPlanData().PerformanceGetMach(fp.GetCorrelatedRadarTarget().GetPosition().GetPressureAltitude(), Tendency);
		int perf_ias = fp.GetFlightPlanData().PerformanceGetIas(fp.GetCorrelatedRadarTarget().GetPosition().GetPressureAltitude(), Tendency);

		Gdiplus::Point TopRight = Gdiplus::Point(Center.X - PopupSpeedAssign::WIDTH / 2, Center.Y - PopupSpeedAssign::HEIGHT / 2);


		if (IsInRect(MousePt, CRect({ TopRight.X, TopRight.Y }, CSize(HEIGHT, WIDTH)))) {
			LeftAt = std::chrono::system_clock::now();
		}
		else {
			std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - LeftAt;
			if (elapsed_seconds.count() > 6) {
				active_ac = "";
				Reset();
				return;
			}

		}

		int x_Center = TopRight.X + WIDTH / 2;

		g->FillRectangle(&SolidBrush(StaticColours::ListBackground), Rect(TopRight, Size(WIDTH, HEIGHT)));
		g->DrawRectangle(&Pen(StaticColours::ListForeground), Rect(TopRight, Size(WIDTH, HEIGHT)));

		y_Cursor = 35;
		g->FillRectangle(&HatchBrush(HatchStyle::HatchStyleDarkUpwardDiagonal, StaticColours::ListForeground, StaticColours::ListBackground), Rect(TopRight, Size(WIDTH, y_Cursor)));

		y_Cursor = 2;
		dc->SetTextColor(COLORREF(RGB(209, 209, 209)));
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

		dc->RestoreDC(svDc);
	};
};