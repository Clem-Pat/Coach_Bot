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
    botSimulator(this)
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