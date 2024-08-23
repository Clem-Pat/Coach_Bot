#pragma once
#include <string>
#include "TrainerApp.h"
#include "TrainerBotManager.h"
#include "TrainerDataManager.h"
#include "TrainerPredictor.h"
#include "TrainerSimulator.h"
#include "TrainerDataRenderer.h"


class Trainer {
public:

    Trainer();
    Trainer(std::string Username, std::string TypeOfExercise);

    TrainerApp app;
    TrainerDataManager dataManager;
    TrainerDataRenderer dataRenderer;
    TrainerBotManager botManager;
    TrainerPredictor predictor;
    BotSimulator botSimulator;

    std::string TypeOfExercise;
    std::string Username;
    bool CompilingInTestMode;
    bool WantToStartBot;
    bool UsingLoadProfile;
    bool BotWorkingInProgress;
    bool WantToStopBot;
    double MaximumLengthPulledByUser;

    bool simulationMode = false; // true for simulation mode (Will use pre-saved data through TrainerSimulator), false for real bot mode (listen and command the robot). Can be changed with a checkbox in the Bot Mode window
    bool WantQuickSimulatedData = true; // true for quick simulation, false for a complete simulation with a real One RM experiment
    double AquisitionFrequency = (simulationMode) ? 0.05 : 0.1; //if the bot is in simulation mode, the frequency is 0.05, else it is 0.1. This is just a way to make the simulation fast

    HWND hwnd = NULL;
    bool WantWindowClosed = false;
    double getWeightToApply(double actualLength);
};

