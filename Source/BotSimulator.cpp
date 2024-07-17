#include "BotSimulator.h"
#include "TrainerDataManager.h"
#include <iostream>
#include <vector>
#include <cmath>
#include "Trainer.h"



void BotSimulator::Begin(const float a, const float b, const float c) {
    this->a = a;
    this->b = b;
    this->c = c;
    TheTrainer->dataManager.ConfigureDataNames({ "ts", "ang_v_0", "ang_v_1", "ang_a_0", "ang_a_1", "distance_0", "distance_1", "estimate_power_kg[0]", "estimate_power_kg[1]", "estimate_power_both_kg", "estimate_power_N[0]", "estimate_power_N[1]", "estimate_power_both_N", "dennryu_Left", "dennryu_Right", "Loadcell_Left", "Loadcell_Right" });
    //TheTrainer->dataManager.ConfigureDataNames({ "X", "Y" });
}

void BotSimulator::createPoint(const float x) {

    //Create a point from a random function. Useful to test the 1RM experiment
    std::vector<bool> WantToPlot = std::vector<bool>(TheTrainer->dataManager.nbreOfDataToRegister, false) ;
    for (int i = 0; i < TheTrainer->dataManager.nbreOfDataToRegister; i++) { //We only want to plot the speed, acceleration and distance data
        if (i == 36 || i == 37) {
            WantToPlot[i] = true;
        }
    }
    double maxHeight = 90; //in cm 
    double speed = 2; //in Hz
    double PI = std::acos(-1);

    std::srand(std::time(0));       // use current time as seed for random generator
    std::vector<double> vec(TheTrainer->dataManager.nbreOfDataToRegister);    // Create a vector of 40 doubles which will be the data 
    for (double& val : vec) {
        val = static_cast<double>(std::rand()) / RAND_MAX;
    }
    float noise = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2.0) - 1.0;

    // TIME 
    vec[0] = x; // ts
    
    // DISTANCE sinusoidal function
    if (TheTrainer->botManager.TotalWeightWanted != 0) {
        vec[36] = maxHeight/2 * (std::sin(speed * 2 * PI * (static_cast<double>(3) / TheTrainer->botManager.TotalWeightWanted) * x) + 1) + 10 + noise; // distance_0
	}
	else {
		vec[36] = maxHeight/2 * (std::sin(speed * 2 * PI * x) + 1) + 10; // distance_0
	}

    // VELOCITY 
    //vec[1] = - TheTrainer->botManager.TotalWeightWanted + 100 + noise ; // ang_v_0 FOR ONE RM EXPERIMENT
    //vec[2] = - TheTrainer->botManager.TotalWeightWanted + 100 + noise; // ang_v_1 FOR ONE RM EXPERIMENT
    if (TheTrainer->dataManager.experimentData[0].size() != 0) {
        vec[37] = (TheTrainer->dataManager.experimentData[36].back() - vec[36]) / (TheTrainer->dataManager.experimentData[0].back() - vec[0]); // ang_v_0
    }
    else {
        vec[37] = 0; // ang_v_0
    }

    TheTrainer->dataManager.AddData(vec, WantToPlot);

}
