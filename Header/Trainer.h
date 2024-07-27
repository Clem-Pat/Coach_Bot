#pragma once
#include <string>
#include "TrainerApp.h"
#include "TrainerBotManager.h"
#include "TrainerDataManager.h"
#include "TrainerPredictor.h"
#include "BotSimulator.h"
#include "TrainerDataRenderer.h"


class Trainer {
public:

    Trainer();

    TrainerApp app;
    TrainerDataManager dataManager;
    TrainerDataRenderer dataRenderer;
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
    double AquisitionFrequency = 0.1;

    bool WantWindowClosed = false;
    double getWeightToApply(double actualLength);
};

