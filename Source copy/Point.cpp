/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org
 * Please refer to the Datasheet to know more about the application
 */


#include "Point.h"
#include "TrainerApp.h"
#include "Trainer.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Point::Point(TrainerApp* app, const std::vector<double> wantedCoord, const std::string wantedType, double wantedIndex, ImVec4 wantedColor) {
    this->TheTrainerApp = app;

    x = wantedCoord[0];
    y = wantedCoord[1];
    index = wantedIndex;
    type = wantedType; //Either "lowerBound" or "upperBound"
    color = wantedColor;
}
