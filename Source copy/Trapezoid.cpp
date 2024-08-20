#include "Trapezoid.h"
#include "TrainerApp.h"
#include "Trainer.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Trapezoid::Trapezoid(TrainerApp* app) {
    this -> TheTrainerApp = app;

    vertices[0] = { 0.0, 0.0 };
    vertices[1] = { 0.2, 1.0 };
    vertices[2] = { 0.5, 1.0 };
    vertices[3] = { 1.0, 0.0 };
}

void Trapezoid::Begin() {
    vertices[0] = { 0.0 * TheTrainerApp->TheTrainer->MaximumLengthPulledByUser, 0.0 * TheTrainerApp->TheTrainer->botManager.TotalWeightWanted };
    vertices[1] = { 0.2 * TheTrainerApp->TheTrainer->MaximumLengthPulledByUser, 1.0 * TheTrainerApp->TheTrainer->botManager.TotalWeightWanted };
    vertices[2] = { 0.5 * TheTrainerApp->TheTrainer->MaximumLengthPulledByUser, 1.0 * TheTrainerApp->TheTrainer->botManager.TotalWeightWanted };
    vertices[3] = { 1.0 * TheTrainerApp->TheTrainer->MaximumLengthPulledByUser, 0.0 * TheTrainerApp->TheTrainer->botManager.TotalWeightWanted };
}

void Trapezoid::update(double totalWeightWanted) {
    std::vector<double> vertices_y;
    std::vector<double> vertices_x;
    for (int k = 0; k < vertices.size(); ++k) {
        vertices_y.push_back(vertices[k][1]);
		vertices_x.push_back(vertices[k][0]);
	}
    auto maxYValue = std::max_element(vertices_y.begin(), vertices_y.end());
    auto maxXValue = std::max_element(vertices_x.begin(), vertices_x.end());
    auto minXValue = std::min_element(vertices_x.begin(), vertices_x.end());

    for (int k = 0; k < vertices.size(); ++k) {
        vertices[k][1] = vertices[k][1] * totalWeightWanted / *maxYValue;  //The highest vertice must be at 100% of totalWeightWanted
        vertices[k][0] = vertices[k][0] * (TheTrainerApp->TheTrainer->MaximumLengthPulledByUser - TheTrainerApp->TheTrainer->MinimumLengthPulledByUser) / (*maxXValue - *minXValue);
    }
    TheTrainerApp->AddVerticalLineToPlot(TheTrainerApp->TheTrainer->MaximumLengthPulledByUser, "Maximum length pulled by user", 2);
    TheTrainerApp->AddVerticalLineToPlot(TheTrainerApp->TheTrainer->MinimumLengthPulledByUser, "Minimum length pulled by user", 2);
    //vertices[3][0] = TheTrainerApp->TheTrainer->MaximumLengthPulledByUser;
}

int Trapezoid::isNearVertex(std::vector<double> mousePositionAtRightClick) {
    for (int i = 0; i < vertices.size(); ++i) {
        if (abs(vertices[i][0] - mousePositionAtRightClick[0]) < 1 && abs(vertices[i][1] - mousePositionAtRightClick[1]) < 1) {
            return i;
        }
    }
    return -1;
}

void Trapezoid::deletePoint(int index) {
    vertices.erase(vertices.begin() + index);
}

void Trapezoid::createPoint(std::vector<double> point) {
    bool inserted = false;
    for (int i = 0; i < vertices.size(); ++i) {
        if (vertices[i][0] > point[0]){
            vertices.insert(vertices.begin() + i, point);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        vertices.push_back(point);
    }
}
