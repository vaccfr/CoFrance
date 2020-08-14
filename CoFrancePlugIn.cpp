#include "pch.h"
#include "CoFrancePlugIn.h"
#include "RadarScreen.h"



CoFrancePlugIn::CoFrancePlugIn(void):CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER, MY_PLUGIN_COPYRIGHT)
{
    char DllPathFile[_MAX_PATH];

    GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
    DllPath = DllPathFile;
    DllPath.resize(DllPath.size() - strlen("CoFrance.dll"));
    
    LoadConfigFile();

    DisplayUserMessage("Message", "CoFrance PlugIn", string("Version " + string(MY_PLUGIN_VERSION) + " loaded.").c_str(), false, false, false, false, false);
}

CoFrancePlugIn::~CoFrancePlugIn()
{
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

void CoFrancePlugIn::LoadConfigFile(bool fromWeb)
{
    DisplayUserMessage("Message", "CoFrance PlugIn", string("Reading config file from " + DllPath + "\\CoFrance.toml").c_str(), false, false, false, false, false);

    try {
        if (fromWeb) {
            httplib::Client cli(CONFIG_ONLINE_URL_BASE);
            if (auto res = cli.Get(CONFIG_ONLINE_URL_PATH)) {
                if (res->status == 200)
                    CoFranceConfig = toml::parse(res->body);
                else
                    fromWeb = false;
            }
            else 
                fromWeb = false;
                

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
