#pragma once
#include "pch.h"
#include "Constants.h"
#include <EuroScopePlugIn.h>

using namespace std;
using namespace EuroScopePlugIn;

class VERA
{
public:
	struct VERADataStruct {
		int minDistanceSeconds = -1;
		double minDistanceNm = -1;
		CPosition predictedFirstPos, predictedSecondPos;
	};

	static VERADataStruct Calculate(CRadarTarget FirstTarget, CRadarTarget SecondTarget, int time_to_extrapolate = 30) {
		CPosition Prediction;
		CPosition PredictionOther;
		CPosition FirstTargetPos = FirstTarget.GetPosition().GetPosition();
		CPosition SecondTargetPos = SecondTarget.GetPosition().GetPosition();

		VERADataStruct defaultOut;
		VERADataStruct out;

		double lastDistance = FirstTargetPos.DistanceTo(SecondTargetPos);

		for (int i = 30; i <= 60 * time_to_extrapolate; i += 10) {
			Prediction = Extrapolate(FirstTargetPos, FirstTarget.GetTrackHeading(),
				FirstTarget.GetPosition().GetReportedGS() * 0.000277778 * i);
			PredictionOther = Extrapolate(SecondTargetPos, SecondTarget.GetTrackHeading(),
				SecondTarget.GetPosition().GetReportedGS() * 0.000277778 * i);

			// The distance started to increase, we passed the minimum point
			if (Prediction.DistanceTo(PredictionOther) > lastDistance) {
				if (lastDistance == FirstTargetPos.DistanceTo(SecondTargetPos))
					return defaultOut;

				out.minDistanceSeconds = i;
				out.minDistanceNm = Prediction.DistanceTo(PredictionOther);
				out.predictedFirstPos = Prediction;
				out.predictedSecondPos = PredictionOther;

				return out;
			}

			lastDistance = Prediction.DistanceTo(PredictionOther);
		}

		return defaultOut;
	};
};