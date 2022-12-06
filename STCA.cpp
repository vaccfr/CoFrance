#include "pch.h"
#include "STCA.h"

CSTCA::CSTCA(toml::value CurrentConfig)
{
	this->high_level_sep = toml::find<int>(CurrentConfig, "stca", "high_level_sep");
	this->low_level_sep = toml::find<int>(CurrentConfig, "stca", "low_level_sep");
	this->disable_level = toml::find<int>(CurrentConfig, "stca", "disable_level");
	this->level_reduced_sep = toml::find<int>(CurrentConfig, "stca", "level_reduced_sep");
	this->time_to_extrapolate = toml::find<int>(CurrentConfig, "stca", "time_to_extrapolate");
	this->altitude_sep = toml::find<int>(CurrentConfig, "stca", "altitude_sep");
	//this->altitude_coarse_filter = toml::find<int>(CurrentConfig, "stca", "altitude_coarse_filter");
	//this->distance_coarse_filter = toml::find<int>(CurrentConfig, "stca", "distance_coarse_filter");
	
}


CSTCA::~CSTCA()
{

}

bool CSTCA::IsSTCA(string cs)
{
	if (std::find(Alerts.begin(), Alerts.end(), cs) != Alerts.end())
	{
		return true;
	}
	return false;
};

void CSTCA::OnRefresh(CPlugIn* pl)
{
	Alerts.clear();

	CRadarTarget rt;
	for (rt = pl->RadarTargetSelectFirst();
		rt.IsValid();
		rt = pl->RadarTargetSelectNext(rt))
	{
		if (rt.GetPosition().GetRadarFlags() == EuroScopePlugIn::RADAR_POSITION_PRIMARY)
			continue;

		if (rt.GetPosition().GetPressureAltitude() < disable_level)
			continue;

		if (strcmp(rt.GetPosition().GetSquawk(), "7000") == 0)
			continue;

		if (rt.GetCorrelatedFlightPlan().IsValid())
		{
			if (rt.GetCorrelatedFlightPlan().GetFlightPlanData().GetPlanType()[0] == 'V')
				continue;
		}

		CRadarTarget conflicting;
		for (conflicting = pl->RadarTargetSelectFirst();
			conflicting.IsValid();
			conflicting = pl->RadarTargetSelectNext(conflicting))
		{
			int separation_distance = high_level_sep;
			int extrapolationTime = time_to_extrapolate;
			double current_horiz_distance = 0;
			int current_vert_distance = 0;
			int vz = 0;
			int vz_conflicting = 0;
			int alt = 0;
			int alt_conflicting = 0;

			if (rt.GetCallsign() == conflicting.GetCallsign())
				continue;

			if (conflicting.GetPosition().GetRadarFlags() == EuroScopePlugIn::RADAR_POSITION_PRIMARY)
				continue;

			if (conflicting.GetPosition().GetPressureAltitude() < disable_level)
				continue;

			if (strcmp(conflicting.GetPosition().GetSquawk(), "7000") == 0)
				continue;

			if (conflicting.GetCorrelatedFlightPlan().IsValid())
			{
				if (conflicting.GetCorrelatedFlightPlan().GetFlightPlanData().GetPlanType()[0] == 'V')
					continue;
			}
			
			if (rt.GetPosition().GetPressureAltitude() <= level_reduced_sep
				&& conflicting.GetPosition().GetPressureAltitude() <= level_reduced_sep)
			{
				separation_distance = low_level_sep;
			}

			alt = rt.GetPosition().GetPressureAltitude();
			alt_conflicting = conflicting.GetPosition().GetPressureAltitude();
			current_horiz_distance = rt.GetPosition().GetPosition().DistanceTo(conflicting.GetPosition().GetPosition());
			current_vert_distance = abs(alt - alt_conflicting);

			// Coarse filter on alt/dist difference (to avoid unecessary extrapolation on obviously separated traffics)
			if (current_vert_distance > altitude_coarse_filter)
				continue;
			if ( current_horiz_distance > distance_coarse_filter)
				continue;

			// Are the traffics already in conflict ?
			if (current_vert_distance < altitude_sep &&
				current_horiz_distance < separation_distance)
			{
				if (std::find(Alerts.begin(), Alerts.end(), rt.GetCallsign()) == Alerts.end())
					Alerts.push_back(rt.GetCallsign());
				if (std::find(Alerts.begin(), Alerts.end(), conflicting.GetCallsign()) == Alerts.end())
					Alerts.push_back(conflicting.GetCallsign());
				continue;
			}

			// Compute vertical speed, with filter on low altitude deviation
			if (rt.GetPreviousPosition(rt.GetPosition()).IsValid() &&
					conflicting.GetPreviousPosition(conflicting.GetPosition()).IsValid())
				{
					int dalt = alt - rt.GetPreviousPosition(rt.GetPosition()).GetPressureAltitude();
					int dalt_conflicting = alt_conflicting - conflicting.GetPreviousPosition(conflicting.GetPosition()).GetPressureAltitude();

					int dt = rt.GetPreviousPosition(rt.GetPosition()).GetReceivedTime() - rt.GetPosition().GetReceivedTime();
					int dt_conflicting = conflicting.GetPreviousPosition(conflicting.GetPosition()).GetReceivedTime() - conflicting.GetPosition().GetReceivedTime();

					if (dalt < 10)
					{
						vz = 0;
					}
					else if (dt > 0)
					{
						vz = (dalt * 60) /dt;
					}
					
					if (dalt_conflicting < 10)
					{
						vz_conflicting = 0;
					}
					else if (dt_conflicting > 0)
					{
						vz_conflicting = (dalt_conflicting * 60) / dt_conflicting;
					}
				}	
			// Coarse filters based on altitude and vertical speed
			
			// If we are stable or climbing and above comflicting, no risk if conflicting is stable or descending
			if (vz >= 0 && alt > ( alt_conflicting + 2000 ) && vz_conflicting <= 0)
				continue;
			// If we are stable or descending and below comflicting, no risk if conflicting is stable or climbing
			if (vz <= 0 && alt < ( alt_conflicting - 2000 ) && vz_conflicting >= 0)
				continue;

			// Faire un calcul en vectoriel pour voir le rapprochement (ou pas)

			for (int i = 10; i <= time_to_extrapolate; i += 10)
			{

				CPosition ex1 = Extrapolate(rt.GetPosition().GetPosition(), rt.GetTrackHeading(), rt.GetPosition().GetReportedGS() * 0.000277778 * i);
				CPosition ex2 = Extrapolate(conflicting.GetPosition().GetPosition(), conflicting.GetTrackHeading(), conflicting.GetPosition().GetReportedGS() * 0.000277778 * i);

				int alt1 = alt;
				int alt2 = alt_conflicting;

				if (vz != 0 || vz_conflicting != 0)
				{
					alt1 += (vz * i) / 60;
					alt2 += (vz_conflicting * i) / 60;
				}

				if (ex1.DistanceTo(ex2) < separation_distance &&
					abs(alt1 - alt2) < altitude_sep)
				{
					if (std::find(Alerts.begin(), Alerts.end(), rt.GetCallsign()) == Alerts.end())
						Alerts.push_back(rt.GetCallsign());
					if (std::find(Alerts.begin(), Alerts.end(), conflicting.GetCallsign()) == Alerts.end())
						Alerts.push_back(conflicting.GetCallsign());
					break;
				}
			}


		}

	}

}