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
    TypeOfExercice = "Curl";
    Username = "Username";
    MaximumLengthPulledByUser = 100;
    CompilingInTestMode     = true; // Put it False to compile in normal mode. It is very important to make Emergency switch work
    WantToStartBot          = false;
    BotWorkingInProgress    = false;
    AcquisitionAsked        = false;
    WantToStopBot           = false;
    botSimulator.Begin(1.0, -5.0, 0.0);
    dataManager.Begin("trainerbot_database");
    app.trapeze.Begin();
}

double Trainer::getWeightToApply(double actualLength) {
    //TODO : must be somewhere else. 
    for (int i = 0; i < app.trapeze.vertices.size() - 1; ++i){
        if (actualLength > app.trapeze.vertices[i][0] && actualLength < app.trapeze.vertices[i + 1][0]){
            vector<double> A = app.trapeze.vertices[i];
            vector<double> B = app.trapeze.vertices[i + 1];
            double slope = (B[1] - A[1]) / (B[0] - A[0]);
            double intersect = A[1] - slope * A[0];
            double weight = slope * actualLength + intersect;
            return weight;
        }
    }
    return 0;
}

