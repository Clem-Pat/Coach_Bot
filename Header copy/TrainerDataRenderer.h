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