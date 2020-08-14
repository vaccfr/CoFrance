#include "pch.h"
#include "RadarScreen.h"

RadarScreen::RadarScreen(CoFrancePlugIn* CoFrancepluginInstance)
{
	this->CoFrancepluginInstance = CoFrancepluginInstance;
}

RadarScreen::~RadarScreen()
{
	delete CoFrancepluginInstance;
}

void RadarScreen::OnRefresh(HDC hDC, int Phase)
{
	Graphics g(hDC);
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
	} 
	catch (const std::exception& exc) {
		Log(exc.what());
		GetPlugIn()->DisplayUserMessage("Message", "CoFrance PlugIn", string("Error parsing file config file " + string(exc.what())).c_str(), false, false, false, false, false);
	}

	g.ReleaseHDC(hDC);
}
