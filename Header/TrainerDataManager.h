#pragma once
#include <string>
#include <vector>

class Trainer;

class TrainerDataManager {
public:
    
    void Begin(const std::string& filename);
    void AddLineToFile(float ts, double theta_deg[], double theta_diff[], double distance[]);
    void AddLineToFile(char line[200], bool WithEndOfLine);
    void AddData(int index, const double data);
    void AddData(std::vector<double>& data, std::vector<bool> WantToPlot = std::vector<bool>(1, true));
    int findIndexOfEmptyData();
    void ConfigureDataName(int index, const std::string& dataName);
    void ConfigureDataNames(const std::vector<std::string>& dataNames);
    void End();

    int nbreOfDataToRegister = 40;
    std::vector<std::vector<double>> experimentData = std::vector<std::vector<double>>(nbreOfDataToRegister, std::vector<double>());
    std::vector<std::string> experimentDataNames = std::vector<std::string>(nbreOfDataToRegister, "");

    //For 1RM experiment
    bool WantMeasureRep = false;
    std::vector<std::vector<double>> TimeDistanceAndVelocityInRep = std::vector<std::vector<double>>(3, std::vector<double>()); //[[time in a rep], [all distances measured in a rep], [all velocities measured in a rep]]

    Trainer* TheTrainer;
    TrainerDataManager(Trainer* trainer) {
        this->TheTrainer = trainer;
        //experimentDataNames[0] = "X";
        //experimentDataNames[1] = "Y";
    }
};
