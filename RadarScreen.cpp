#include "pch.h"
#include "RadarScreen.h"

RadarScreen::RadarScreen()
{
	char DllPathFile[_MAX_PATH];
	string DllPath;
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	DllPath = DllPathFile;
	DllPath.resize(DllPath.size() - strlen("CoFrance.dll"));

	ifstream ifs(DllPath + "\\CoFrance.toml", std::ios_base::binary);
	Config = toml::parse(ifs, "CoFrance.toml");
}

RadarScreen::~RadarScreen()
{
}

void RadarScreen::OnRefresh(HDC hDC, int Phase)
{
	Graphics g(hDC);
	try {

		auto sector_ceiling_text_color = toml::find<std::vector<int>>(Config, "colours", "sector_ceiling_text");
		Color SectorCeilingColor(sector_ceiling_text_color.at(0), sector_ceiling_text_color.at(1), sector_ceiling_text_color.at(2));
	
		// Drawing of SCT levels
		if (Phase == EuroScopePlugIn::REFRESH_PHASE_BACK_BITMAP)
		{
			for (const auto& v : toml::find<toml::array>(Config, "sector_ceilings"))
			{
				CPosition p;
				p.LoadFromStrings(toml::find<vector<string>>(v, "position").back().c_str(), toml::find<vector<string>>(v, "position").front().c_str());
				DrawFixedSizedText(&g, p, toml::find<int>(v, "size"), toml::find<string>(v, "text"), SectorCeilingColor);
			}

		}
	} 
	catch (const std::exception& exc) {
		Log(exc.what());
		GetPlugIn()->DisplayUserMessage("Message", "CoFrance PlugIn", string("Error parsing file config file " + string(exc.what())).c_str(), false, false, false, false, false);
	}

	g.ReleaseHDC(hDC);
}
