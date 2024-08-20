#include "Line.h"
#include "TrainerApp.h"
#include "Trainer.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits> // Include for std::numeric_limits


Line::Line(TrainerApp* app, const std::vector<std::vector<double>> coords, const std::string type, double index) {
    this->TheTrainerApp = app;
    this->pointA = coords[0];
    this->pointB = coords[1];
    this->index = index;
    this->type = type; //Either "lowerBound" or "upperBound"
}

void Line::update() {
    std::cout << "classic lines don't have update method (type : " << this->type << ", point A : " << pointA[0] << "," << pointA[1] << ". point B : " << pointB[0] << "," << pointB[1] << ")" << std::endl;
}

void HorizontalLine::update() {
    double x_min = 100000, x_max = -100000;
    for (int j = 0; j < TheTrainerApp->listXToPlot.size(); ++j) {
        for (int k = 0; k < TheTrainerApp->listXToPlot[j].size(); ++k) {
            x_min = (((x_min) < (TheTrainerApp->listXToPlot[j][k])) ? (x_min) : (TheTrainerApp->listXToPlot[j][k]));
            x_max = (((x_max) > (TheTrainerApp->listXToPlot[j][k])) ? (x_max) : (TheTrainerApp->listXToPlot[j][k]));
        }
    }
    pointA = { x_min, yValue };
    pointB = { x_max, yValue };
}

void VerticalLine::update() {
    double y_min = 100000, y_max = -100000;
    for (int j = 0; j < TheTrainerApp->listXToPlot.size(); ++j) {
        for (int k = 0; k < TheTrainerApp->listXToPlot[j].size(); ++k) {
            y_min = (((y_min) < (TheTrainerApp->listYToPlot[j][k])) ? (y_min) : (TheTrainerApp->listYToPlot[j][k]));
            y_max = (((y_max) > (TheTrainerApp->listYToPlot[j][k])) ? (y_max) : (TheTrainerApp->listYToPlot[j][k]));
        }
    }
    if (index == 2) { //If the vertical line is plotted in the load profile graph, the rule is a bit different
        y_min = 0;
        y_max = TheTrainerApp->TheTrainer->botManager.TotalWeightWanted;
    }
    pointA = { xValue, y_min };
    pointB = { xValue, y_max };
}
