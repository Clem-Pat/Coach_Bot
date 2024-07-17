#pragma once
#include <string>
#include "TrainerApp.h"
#include "TrainerBotManager.h"
#include "TrainerDataManager.h"
#include "TrainerPredictor.h"
#include "BotSimulator.h"


class Trainer {
public:

    Trainer();

    TrainerApp app;
    TrainerDataManager dataManager;
    TrainerBotManager botManager;
    TrainerPredictor predictor;
    BotSimulator botSimulator;

    std::string TypeOfExercice;
    std::string Username;
    bool CompilingInTestMode;
    bool WantToStartBot;
    bool UsingLoadProfile;
    bool BotWorkingInProgress;
    bool AcquisitionAsked;
    bool WantToStopBot;
    double MaximumLengthPulledByUser;

    std::vector<double> OneRMExperimentLoads;
    std::vector<double> OneRMExperimentVelocities;
    double OneRMVelocityValue = 0.005;

    double getWeightToApply(double actualLength);
};

