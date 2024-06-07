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
    TrainerBotManager botManager;
    TrainerDataManager dataManager;
    TrainerPredictor predictor;
    BotSimulator botSimulator;

    std::string typeOfExercice;
    char username[100];
    bool compilingInTestMode;
    bool Want1RMExperiment;
    bool WantToStartBot;
    bool UsingLoadProfile;
    bool BotWorkingInProgress;
    bool AcquisitionAsked;
    bool WantToStopBot;
    double MaximumLengthPulledByUser;

    std::vector<double> OneRMExperimentLoads;
    std::vector<double> OneRMExperimentVelocities;
    double OneRMVelocityValue = 0.2;

    double getWeightToApply(double actualLength);
};

