/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org 
 * Please refer to the Datasheet to know more about the application
 */


#include "Trainer.h"

#include <map>
#include <implot/implot.h>
#include <string>
#include <cstring>

Trainer::Trainer()
    : app(this),
    dataManager(this),
    dataRenderer(this),
    botManager(this),
    predictor(this),
    TrainerSimulator(this)
{

    // Constructor body
    TypeOfExercise = "Curl";
    Username = "Username";
    MaximumLengthPulledByUser = 100;
    MinimumLengthPulledByUser = 0;
    WantToStartBot = false;
    BotWorkingInProgress = false;
    WantToStopBot = false;
    dataManager.Begin("trainerbot_database");
    app.trapeze.Begin();
}