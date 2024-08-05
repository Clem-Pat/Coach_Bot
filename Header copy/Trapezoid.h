#ifndef __TRAPEZOID_H__
#define __TRAPEZOID_H__


#pragma once
#include <vector>


class TrainerApp;

class Trapezoid {
public:
    std::vector<std::vector<double>> vertices = std::vector<std::vector<double>>(4, std::vector<double>(2, 0.0));

    TrainerApp* TheTrainerApp;
    Trapezoid(TrainerApp* app);
    
    // Check if a point is close to one of the vertices
    void Begin();
    void update(double weight);
    int isNearVertex(std::vector<double> point);
    void deletePoint(int index);
    void createPoint(std::vector<double> point);
};

#endif // !__TRAPEZOID_H__
