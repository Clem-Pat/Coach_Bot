
#ifndef __POINT_H__
#define __POINT_H__


#pragma once
#include <vector>
#include <string> // Include for std::string
#include "imgui.h" // Include for ImVec4

class TrainerApp;

class Point {
public:
    double x = 0; //Coordinates of the Point
    double y = 0; //Coordinates of the Point
    double index = 0;      //index of the subplot we want to plot the Point on
    std::string type = ""; //Either "lowerBound" or "upperBound" or else
    ImVec4 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); //Color of the Point. Red by default
    TrainerApp* TheTrainerApp;

    Point(TrainerApp* app, const std::vector<double> coord, const std::string type, double index = 0, ImVec4 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

};

#endif // !__POINT_H__
