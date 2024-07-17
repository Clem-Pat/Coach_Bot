#include "Point.h"
#include "TrainerApp.h"
#include "Trainer.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Point::Point(TrainerApp* app, const std::vector<double> wantedCoord, const std::string wantedType, double wantedIndex) {
    this->TheTrainerApp = app;

    x = wantedCoord[0];
    y = wantedCoord[1];
    index = wantedIndex;
    type = wantedType; //Either "lowerBound" or "upperBound"
}
