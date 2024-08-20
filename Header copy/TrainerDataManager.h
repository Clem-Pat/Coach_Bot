#pragma once
#include <string>
#include <vector>
#include "sqlite3.h"

class SetDataSaver;
class OneRMAnalyser;

class Trainer;

class TrainerDataManager {
public:
    
    void Begin(const std::string& filename);
    std::string GetTodaysDate();
    int GetUserId(const std::string& username);
    int CountUsers();
    void AddUser(const std::string& username, const std::string& password = "NULL");
    void SaveSetInFile();
    void AddData(int index, const double data);
    void AddData(std::vector<double>& data, std::vector<bool> WantToPlot = std::vector<bool>(1, true));
    int findIndexOfEmptyData();
    void ConfigureDataName(int index, const std::string& dataName);
    void ConfigureDataNames(const std::vector<std::string>& dataNames);
    std::string FindInDataBase(std::string username, std::string typeOfExercise, std::string TotalWeightWanted, std::string whatWeLookFor, std::string columnName);
    void UploadLastOneRMExperimentFromDataBase(std::string username, std::string typeOfExercise);
    std::vector<SetDataSaver> FindWorkoutSessionInDataBase(std::string username, std::string date);
    std::vector<OneRMAnalyser> FindOneRMExperimentInDataBase(std::string username, std::string date = "NULL", std::string typeOfExercise = "NULL");
    void SaveSessionInDataBase();
    void End();

    int nbreOfDataToRegister = 50;
    std::vector<std::vector<double>> experimentData = std::vector<std::vector<double>>(nbreOfDataToRegister, std::vector<double>());
    std::vector<std::string> experimentDataNames = std::vector<std::string>(nbreOfDataToRegister, "");
    sqlite3* db;

    Trainer* TheTrainer;
    TrainerDataManager(Trainer* trainer) {
        this->TheTrainer = trainer;
        //experimentDataNames[0] = "X";
        //experimentDataNames[1] = "Y";
    }
};
