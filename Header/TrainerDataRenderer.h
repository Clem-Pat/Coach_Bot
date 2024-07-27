#pragma once
#include <string>
#include <vector>
#include <SeriesAnalyzer.h>
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

    Trainer* TheTrainer;
    TrainerDataRenderer(Trainer* trainer) {
        this->TheTrainer = trainer;
    }
};