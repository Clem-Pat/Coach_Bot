
#ifndef __LINE_H__
#define __LINE_H__


#pragma once
#include <vector>
#include <string> // Include for std::string

class TrainerApp;

class Line {
public:
    std::vector<double> pointA = { 0, 0 }; //Coordinates of the Point
    std::vector<double> pointB = { 0, 0 }; //Coordinates of the Point
    double index = 0;      //index of the subplot we want to plot the Line on
    std::string type = ""; //Either "lowerBound" or "upperBound" or else
    
    TrainerApp* TheTrainerApp;
    virtual void update();

    Line(TrainerApp* app, const std::vector<std::vector<double>> coords, const std::string type = "", double index = 0);
};

class HorizontalLine : public Line {
public:
    double yValue;
    HorizontalLine(TrainerApp* app, double YValue, const std::string type = "", double index = 0) : Line(app, { {},{} }, type, index), yValue(YValue) { update(); } // Temporarily pass an empty vector to the base constructor
    void update() override;
};

class VerticalLine : public Line {
public:
    double xValue;
    VerticalLine(TrainerApp* app, double XValue, const std::string type = "", double index = 0) : Line(app, { {},{} }, type, index), xValue(XValue) { update(); } // Temporarily pass an empty vector to the base constructor
    void update() override;
};

#endif // !__LINE_H__
