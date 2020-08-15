#pragma once
#include "EuroScopePlugIn.h"
#include "Constants.h"
#include <string>

using namespace EuroScopePlugIn;
using namespace std;

class CoFrancePlugIn :
    public CPlugIn
{
public:
    CoFrancePlugIn();
    ~CoFrancePlugIn();

    //---OnRadarScreenCreated------------------------------------------

    CRadarScreen* OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

    //---OnCompileCommand-----------------------------------------------

    bool OnCompileCommand(const char* sCommandLine);

    toml::value CoFranceConfig;
    string DllPath;

    void LoadConfigFile(bool fromWeb = true);

    void Log(string s)
    {
        std::ofstream file;
        file.open("C:/Users/Pierre/source/repos/CoFrance/Debug/CoFrance_custom.log", std::ofstream::out | std::ofstream::app);
        file << s << endl;
        file.close();
    }

};

