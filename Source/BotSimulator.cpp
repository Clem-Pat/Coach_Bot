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

    //Create a point from a known function. Useful to test the TrainerPredictor
    /*float noise = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2.0) - 1.0;
    float y = this->a * x * x + this->b * x + this->c + noise;
    TheTrainer -> dataManager.AddData(0, x);
    TheTrainer -> dataManager.AddData(1, y);*/

    //Create a point from a random function. Useful to test the 1RM experiment
    std::srand(std::time(0));
    std::vector<double> vec(17); // Create a vector of 17 doubles
    for (double& val : vec) {
        val = static_cast<double>(std::rand()) / RAND_MAX;
    }
    vec[0] = x;
    float noise = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2.0) - 1.0;
    vec[1] = - TheTrainer->botManager.InputMotorTorque + 100 + noise ; // ang_v_0
    vec[2] = - TheTrainer->botManager.InputMotorTorque + 100 + noise; // ang_v_1
    TheTrainer->dataManager.AddData(vec);

    //float noise = -0.2 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (0.2 - (-0.2))));
    //vec[1] = -0.024 * TheTrainer->botManager.InputMotorTorque + 1.32 + noise; // ang_v_0
    //vec[2] = -0.024 * TheTrainer->botManager.InputMotorTorque + 1.32 + noise; // ang_v_1
}
