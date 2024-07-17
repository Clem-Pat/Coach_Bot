#include "Trainer.h"
#include "SeriesAnalyzer.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric> // Include this for std::accumulate
#include "Trainer.h"

void SeriesAnalyzer::Begin() {
	Username = TheTrainer->Username;
	TypeOfExercice = TheTrainer->TypeOfExercice; // We were forced to add this line because we could not initialize the TypeOfExercice in the constructor as the circular dependencies problem was hit due to including Trainer.h in the header file. 
	TotalWeightWanted = TheTrainer->botManager.TotalWeightWanted;
	if (TheTrainer->UsingLoadProfile){
		UsingLoadProfile = true;
		profileShape = TheTrainer->app.trapeze.vertices;
	}
}

double SeriesAnalyzer::getMeanMaxHeight() {
	if (maxHeights.size() == 0) return meanMaxHeight; // We never acquired heights so we return the default value
	meanMaxHeight = std::accumulate(maxHeights.begin(), maxHeights.end(), 0.0) / maxHeights.size();
	return meanMaxHeight;
}

void SeriesAnalyzer::closeAnalyzer(double tf) {
	this->tf = tf;
	this->duration = tf - t0;
	this->meanMaxHeight = std::accumulate(maxHeights.begin(), maxHeights.end(), 0.0) / maxHeights.size();
	this->meanMaxVelocity = std::accumulate(maxVelocities.begin(), maxVelocities.end(), 0.0) / maxVelocities.size();
	this->closed = true;
}

void InitialAnalyzer::update() {
	//Recalculating the max Height Reference();
	if (MaxHeightsRecordedSinceLastTypeOfExerciceChange.size() < 5) {
		referencedMaxHeight = std::accumulate(MaxHeightsRecordedSinceLastTypeOfExerciceChange.begin(), MaxHeightsRecordedSinceLastTypeOfExerciceChange.end(), 0.0) / MaxHeightsRecordedSinceLastTypeOfExerciceChange.size();
	}
}

void SeriesAnalyzer::DoMission() {
	std::cout << "ERROR : No mission has been assigned to this SeriesAnalyzer yet." << id << " " << name << std::endl;
	// Empty, each derived class will implement its own DoMission method
}

void CounterAnalyzer::DoMission() {
	TheTrainer->app.NumberOfReps = numberOfReps; //The mission of those analyzers is to update the amount of reps done in the app.
}

void ContinuousOneRMAnalyzer::DoMission() {
	TheTrainer->OneRMExperimentLoads.push_back(TheTrainer->botManager.TotalWeightWanted);
	TheTrainer->OneRMExperimentVelocities.push_back(maxVelocities.back());
	TheTrainer->app.Plot(50, TheTrainer->OneRMExperimentLoads, TheTrainer->OneRMExperimentVelocities, 1, "Max velocity during reps", "scatter");
}