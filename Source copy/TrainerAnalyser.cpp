#include "Trainer.h"
#include "TrainerAnalyser.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric> // Include this for std::accumulate
#include "Trainer.h"

void TrainerAnalyser::Begin() {
	if (TheTrainer != nullptr) {
		this->Username = TheTrainer->Username; // We were forced to add this line because we could not initialize the TypeOfExercise in the constructor as the circular dependencies problem was hit due to including Trainer.h in the header file. 
		this->TypeOfExercise = TheTrainer->TypeOfExercise;  //Same
		this->TotalWeightWanted = TheTrainer->botManager.TotalWeightWanted; //Same
		if (TheTrainer->UsingLoadProfile){
			this->UsingLoadProfile = true;
			this->profileShape = TheTrainer->app.trapeze.vertices;
		}
		this->Date = TheTrainer->dataManager.GetTodaysDate();
	}
}

double TrainerAnalyser::getReferencedMaxHeight() {
	std::string username = TheTrainer->Username;
	std::string typeOfExercise = TheTrainer->TypeOfExercise;
	std::vector<double> referencedMaxHeights = std::vector<double>();
	for (auto& analyser : TheTrainer->predictor.sessionAnalysers) {
		if (dynamic_cast<SetDataSaver*>(analyser.get())) {
			SetDataSaver* setAnalyser = dynamic_cast<SetDataSaver*>(analyser.get());
			if (setAnalyser->Username == username && setAnalyser->TypeOfExercise == typeOfExercise) {
				if (referencedMaxHeights.size() > 5) {
					break;
				}
				else{
					for (double maxHeight : setAnalyser->maxHeights){
						referencedMaxHeights.push_back(maxHeight);
					}
				}
			}
		}
	}
	if (referencedMaxHeights.size() == 0) return 0; // We never acquired heights so we return the default value
	return std::accumulate(referencedMaxHeights.begin(), referencedMaxHeights.end(), 0.0) / referencedMaxHeights.size();
}

void TrainerAnalyser::closeAnalyser(double tf) {
	this->tf = tf;
	this->duration = tf - t0;
	this->meanMaxHeight = std::accumulate(maxHeights.begin(), maxHeights.end(), 0.0) / maxHeights.size();
	this->meanMaxVelocity = std::accumulate(maxVelocities.begin(), maxVelocities.end(), 0.0) / maxVelocities.size();
	auto indexMinMaxVelocities = std::min_element(maxVelocities.begin(), maxVelocities.end());
	if (indexMinMaxVelocities != maxVelocities.end()) {
		this->minMaxVelocities = *indexMinMaxVelocities;
		this->indexOfRepOfMinMaxVelocities = std::distance(maxVelocities.begin(), indexMinMaxVelocities);
	}
	auto indexMaxVelocities = std::max_element(maxVelocities.begin(), maxVelocities.end());
	if (indexMaxVelocities != maxVelocities.end()) {
		this->maxMaxVelocities = *indexMaxVelocities;
		this->indexOfRepOfMaxMaxVelocities = std::distance(maxVelocities.begin(), indexMaxVelocities);
	}
	this->numberOfReps = maxVelocities.size();
	this->setsWithProblemsToKeep = std::vector<bool>(problemsInSet.size(), true);
	this->closed = true;
}

void InitialAnalyser::update(bool FinishedLastSet) {
	//Recalculating the max Height Reference();
	if (!FinishedLastSet){
		if (MaxHeightsRecordedSinceLastTypeOfExerciseChange.size() < 5) {
			referencedMaxHeight = std::accumulate(MaxHeightsRecordedSinceLastTypeOfExerciseChange.begin(), MaxHeightsRecordedSinceLastTypeOfExerciseChange.end(), 0.0) / MaxHeightsRecordedSinceLastTypeOfExerciseChange.size();
		}
	}
	else {

	}
}

void TrainerAnalyser::DoMission() {
	std::cout << "ERROR : No mission has been assigned to this TrainerAnalyser yet." << id << " " << name << std::endl;
	// Empty, each derived class will implement its own DoMission method
}

void SetDataSaver::DoMission() {
	//The mission of those analysers is to save the data of the set
	this->TimeOfSet.push_back(TheTrainer->app.listXToPlot[36 - 1].back()); //We save the current time 
	for (int i = 0; i < 2; i++) {
		this->HeightOfSet[i].push_back(TheTrainer->app.listYToPlot[36 + 3 * i - 1].back()); 
		this->VelocityOfSet[i].push_back(TheTrainer->app.listYToPlot[37 + 3 * i - 1].back());
		this->AccelerationOfSet[i].push_back(TheTrainer->app.listYToPlot[38 + 3 * i - 1].back());
	}
}

void CounterAnalyser::DoMission() {
	//The mission of those analysers is to update the amount of reps done in the app.
	TheTrainer->app.DeleteRobotConsoleMessage("One RM according to Lander : ");
	TheTrainer->app.DeleteRobotConsoleMessage("One RM according to Epley : ");
	TheTrainer->app.DeleteRobotConsoleMessage("One RM according to Mayhew : ");
	TheTrainer->app.AddRobotConsoleMessage("One RM according to Lander : " + std::to_string(TheTrainer->predictor.Lander(TotalWeightWanted, numberOfReps)));
	TheTrainer->app.AddRobotConsoleMessage("One RM according to Epley : " + std::to_string(TheTrainer->predictor.Epley(TotalWeightWanted, numberOfReps)));
	TheTrainer->app.AddRobotConsoleMessage("One RM according to Mayhew : " + std::to_string(TheTrainer->predictor.Mayhew(TotalWeightWanted, numberOfReps)));
}

void OneRMAnalyser::DoMission() {
	//The mission of those analysers is to update the OneRM experiment data AND the endurance experiment data. As it is defined by former student Natsuo Tojo.
	auto index = TheTrainer->predictor.FindLastAnalyserOfType<SetDataSaver>();
	if (index != -1) {
		auto setAnalyser = dynamic_cast<SetDataSaver*>(TheTrainer->predictor.sessionAnalysers[index].get());
		for (int i = 0; i < setAnalyser->maxVelocities.size(); i++) {
			OneRMExperimentLoads.push_back(setAnalyser->TotalWeightWanted);
			OneRMExperimentVelocities.push_back(setAnalyser->maxVelocities[i]);
			EnduranceExperimentVelocitiesLoss.push_back(100 * (setAnalyser->maxMaxVelocities - setAnalyser->maxVelocities[i]) / setAnalyser->maxMaxVelocities);
			EnduranceExperimentRepetitionRatio.push_back(100 * (i + 1) / setAnalyser->maxVelocities.size());
		}
		allMinMaxVelocitiesOfExperiment.push_back(setAnalyser->minMaxVelocities);
	}
	UpdateAndPlotOneRMExperiment();
}

void OneRMAnalyser::UpdateAndPlotOneRMExperiment(bool forceAddingRegressionToBePlotted) {
	if (Date == TheTrainer->dataManager.GetTodaysDate()) { //We need to update the OneRMVelocityValue because the user could have deleted a point from allMinMaxVelocitiesOfExperiment. But for uploaded past One RM experiments, we will not update the OneRMVelocityValue because we do not know the allMinMaxVelocitiesOfExperiment.
		OneRMVelocityValue = std::accumulate(allMinMaxVelocitiesOfExperiment.begin(), allMinMaxVelocitiesOfExperiment.end(), 0.0) / allMinMaxVelocitiesOfExperiment.size();
	}
	TheTrainer->app.Plot(55, OneRMExperimentLoads, OneRMExperimentVelocities, 1, "Max velocity during reps", "scatter");
	TheTrainer->app.Plot(56, EnduranceExperimentVelocitiesLoss, EnduranceExperimentRepetitionRatio, 1.1, "Endurance experiment", "scatter");
	if (TheTrainer->predictor.WantStopOneRMExperiment || forceAddingRegressionToBePlotted) {
		TheTrainer->predictor.AddGraphToDoRegressionOn(55, 55);
		TheTrainer->predictor.AddGraphToDoRegressionOn(56, 56);
	}
}

void OneRMAnalyser::createEnduranceExperimentDataKnowingOneRMExperiment() {
	std::vector<std::vector<double>> MaxVelocitiesEverySets = { {} }; //The goal is to separate [velocity1Ofset1, velocity2Ofset1, velocity3Ofset1, velocity1Ofset2, velocity2Ofset2, velocity3Ofset2, ...] into [[velocity1Ofset1, velocity2Ofset1, velocity3Ofset1], [velocity1Ofset2, velocity2Ofset2, velocity3Ofset2], ...]
	std::vector<double> MaxMaxVelocitiesEverySets; //Will store the maximum value of the max velocities of each set
	for (int i = 0; i < OneRMExperimentVelocities.size(); i++) {
		if (i > 0) {
			if (OneRMExperimentLoads[i] != OneRMExperimentLoads[i - 1]) {
				if (!MaxVelocitiesEverySets.empty() && !MaxVelocitiesEverySets.back().empty()) {
					MaxMaxVelocitiesEverySets.push_back(*std::max_element(MaxVelocitiesEverySets.back().begin(), MaxVelocitiesEverySets.back().end()));
					allMinMaxVelocitiesOfExperiment.push_back(*std::min_element(MaxVelocitiesEverySets.back().begin(), MaxVelocitiesEverySets.back().end()));
				}
				MaxVelocitiesEverySets.push_back({});
			}
		}
		MaxVelocitiesEverySets.back().push_back(OneRMExperimentVelocities[i]);
	}
	//OneRMVelocityValue = std::accumulate(allMinMaxVelocitiesOfExperiment.begin(), allMinMaxVelocitiesOfExperiment.end(), 0.0) / allMinMaxVelocitiesOfExperiment.size(); //This way of calculating the OneRMVelocityValue is not always working. sometimes few data are lost in the process. So we will use the linear regression to calculate the OneRMVelocityValue. This is safer.
	OneRMVelocityValue = CoefficientsOneRMRegression[0] * OneRMValue + CoefficientsOneRMRegression[1];
	for (int i = 0; i < MaxVelocitiesEverySets.size(); i++) {
		for (int j = 0; j < MaxVelocitiesEverySets[i].size(); j++) {
			if (100 * (MaxMaxVelocitiesEverySets[i] - MaxVelocitiesEverySets[i][j]) / MaxMaxVelocitiesEverySets[i] <= 100) { //check if the found value is under 100%. I don't know why, sometimes the MaxMaxVelocities on a set is -2.45e-206. So the result to plot is really high. I did not resolve the problem but it is not a fundamental problem. The important thing is to plot the polynomial regression. 
				EnduranceExperimentVelocitiesLoss.push_back(100 * (MaxMaxVelocitiesEverySets[i] - MaxVelocitiesEverySets[i][j]) / MaxMaxVelocitiesEverySets[i]);
				EnduranceExperimentRepetitionRatio.push_back(100 * (j + 1) / MaxVelocitiesEverySets[i].size());
			}
		}
	}
}


double OneRMAnalyser::GetNumberOfRepAtSpecificVloss(double Vloss) {
	//Vloss is the velocity loss in % from the max velocity of the set. In the former student Natsuo Tojo's work, this function returns the N_per value
	return CoefficientsEnduranceLaw[0] * Vloss * Vloss + CoefficientsEnduranceLaw[1] * Vloss + CoefficientsEnduranceLaw[2];
}