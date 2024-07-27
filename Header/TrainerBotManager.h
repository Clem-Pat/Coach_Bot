#pragma once
#include <windows.h>

#include <time.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <iostream>

class Trainer;

class TrainerBotManager {
public:
    bool BotIsInitiated = false;

    int Rg = 0;
    double AnalogOutput[2] = { 32768, 32768 }; // 32768 is the zero value of the output
    std::vector<double> distancesMeasured{}; //�p���x�̈ړ����ϒl

    int ClutchModeSelected;
    int OldClutchModeSelected;
    bool WantTakeOffTheClutch = false;
    int TemporaryTotalWeightWanted; // This variable will hold the current value of the slider. It is the old INPUT_TORQUE variable
    int TotalWeightWanted; // This variable will hold the current value of the slider. It is the old INPUT_TORQUE variable
    int MaxOutputMotorTorque;

    //Contec board
    long  Ret;								// 戻り値 return value
    short Id;								// Device ID for interface board
    short IdCNT; 						    // Device ID for Counter
    short ChNo[2] = { 0, 1 };				// listen counter on channel 0 and 1 (0 : left, 1: right)
    unsigned long CntDat[2];				// Counter data
    
    //Measuring data
    std::chrono::steady_clock::time_point ExperimentT0 = std::chrono::steady_clock::now();;
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    double pre_t;
    double MotorTorque[2] = { 0, 0 };
    double deltaTheta[2];
    int PotentiometerRoundCounter[2] = { 0, 0 };
    float PotentiometerOrigin[2];
    std::vector<std::vector<double>> ListPotentiometerRawVoltageValue; // [[leftValue, leftValue, ...], [rightValue, rightValue, ...]]
    double PrePotentiometerRelativeValue[2] = { 0, 0 };
    double AngleRelativeValueUsingPotentiometer[2];
    double initialCounterValue[2];
    double AngleRelativeValueUsingCounter[2];
    double PreAngleRelativeValueUsingCounter[2];
    double DistanceRelativeValueUsingPotentiometer[2];
    double DistanceRelativeValueUsingCounter[2];
    double AngleVelocity[2];
    std::vector<std::vector<double>> ListAngleVelocity = { {},{} }; // [[leftValue, leftValue, ...], [rightValue, rightValue, ...]]    Useful for Kalman filter
    double KalmanPValueForVelocity[2] = { 0.1, 0.1 };   //Initial value for Kalman filter
    double AngleAcceleration[2];


    void Begin();
    void BotSetup();
    void ChangeClutchOption();
    void TakeOffTheClutch();
    void PutOnTheClutch();
    double MovingAverageAndExponentialSmoothing(std::vector<double>& oldSmoothedValues, double ValueToSmooth);
    std::vector<double> KalmanFilter(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P);
    std::vector<double> KalmanFilter2(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P);

    void BotStart();
    void BotAquisition(); 
    void BotCommand();
    void BotSaveData();
    void MotorStop();
    void BotStop();
    void BotKill();


    Trainer* TheTrainer;
    TrainerBotManager(Trainer* trainer) {
        this->TheTrainer = trainer;
        this->ClutchModeSelected = 0;
        this->OldClutchModeSelected = -1;
        this->TotalWeightWanted = 10;
        this->TemporaryTotalWeightWanted = this->TotalWeightWanted;
        this->MaxOutputMotorTorque = 40;
        BotSetup();
    }
};