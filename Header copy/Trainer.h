/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org
 * Please refer to the Datasheet to know more about the application
 */


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

    TrainerApp app;
    TrainerDataManager dataManager;
    TrainerDataRenderer dataRenderer;
    TrainerBotManager botManager;
    TrainerPredictor predictor;
    TrainerSimulator TrainerSimulator;

    std::string TypeOfExercise;
    std::string Username;
    bool WantToStartBot;
    bool BotWorkingInProgress;
    bool WantToStopBot;
    bool UsingLoadProfile;
    double MaximumLengthPulledByUser;
    double MinimumLengthPulledByUser;

    bool ExperimentMode = false; // true for experiment mode. In this mode, we store the data of the sets in CSV files. Useful for engineers to conduct experiments.
    bool SimulationMode = false; // true for simulation mode (Will use pre-saved data through TrainerSimulator), false for real bot mode (listen and command the robot). Can be changed with a checkbox in the Bot Mode window
    bool WantQuickSimulatedData = true; // true for quick simulation, false for a complete simulation with a real One RM experiment
    double AquisitionFrequency = (SimulationMode) ? 0.05 : 0.1; //if the bot is in simulation mode, the frequency is 0.05, else it is 0.1. This is just a way to make the simulation fast

    HWND hwnd = NULL;
    bool WantWindowClosed = false;
};