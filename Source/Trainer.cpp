#include "Trainer.h"

#include <map>
#include <implot/implot.h>
#include <string>
#include <cstring>

Trainer::Trainer()
    : app(this),
    botManager(this),
    dataManager(this),
    predictor(this),
    botSimulator(this)
{

    // Constructor body
    typeOfExercice = "Bench Press";
    strcpy(username, "username");
    MaximumLengthPulledByUser = 100;
    compilingInTestMode     = true; // Put it False to compile in normal mode. It is very important to make Emergency switch work
    WantToStartBot          = false;
    BotWorkingInProgress    = false;
    AcquisitionAsked        = false;
    WantToStopBot           = false;
    botSimulator.Begin(1.0, -5.0, 0.0);
    app.trapeze.Begin();
    //botManager.Begin();


}

double Trainer::getWeightToApply(double actualLength) {

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

