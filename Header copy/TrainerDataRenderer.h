/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org
 * Please refer to the Datasheet to know more about the application
 */


#pragma once
#include <string>
#include <vector>
#include <TrainerAnalyser.h>
#include <memory> // For std::unique_ptr

using std::vector;

class Trainer;

class TrainerDataRenderer {
public:
    std::vector<double> getMonthActivity(int month, int year);
    void defineWorkoutSessionPopup(int year, int month, int day);
    void defineCalendarPopup();
    void Plot(std::vector<double> X, std::vector<double> Y, std::string label, std::string type);

    int selectedDay = 0;
    int selectedMonth = 0;
    int selectedYear = 0;

    std::string WantOpenSetsOfTypeExercise = "";
    double WantOpenSetsOfLoad = 0;
    bool WantOpenOneRM = false;

    Trainer* TheTrainer;
    TrainerDataRenderer(Trainer* trainer) {
        this->TheTrainer = trainer;
    }
};