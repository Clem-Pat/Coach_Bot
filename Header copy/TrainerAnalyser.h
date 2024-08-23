#ifndef __SERIESANALYZER_H__
#define __SERIESANALYZER_H__


#pragma once

#include <vector>
#include <limits>
#include <string>
#include <iostream>

class Trainer;

class TrainerAnalyser {
public:
    Trainer* TheTrainer;
    int id = 0;
    std::string name = "";
    std::string Username = "";
    std::string TypeOfExercise = "";
    double TotalWeightWanted = 0; //equal to the TorqueWanted at the current time of the set
    bool UsingLoadProfile = false;
    std::string Date = ""; //Date is in Year-month-day format. Default date is today's date. When we upload a workout session from the database, the date is the one of the workout session
    std::vector<std::vector<double>> profileShape = std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0)); // The profile shape of the exercise. It is a vector of vectors. Each vector is a point of the trapeze profile.
    double t0 = 0;
    double tf = std::numeric_limits<double>::infinity();
    double duration = 0;
    int numberOfReps = 0;
    int numberOfSets = 0;
    double lastSetEndTime = -1;
    std::vector <double> indexesEnteringARep = std::vector<double>();
    std::vector <double> indexesExitingARep = std::vector<double>();
    double meanMaxHeight = 0;
    double meanMaxVelocity = 0;
    double minMaxVelocities = 0;
    double maxMaxVelocities = 0;
    int indexOfRepOfMinMaxVelocities = 0;
    int indexOfRepOfMaxMaxVelocities = 0;
    std::vector<double> maxHeights = std::vector<double>();
    std::vector<double> maxVelocities = std::vector<double>();
    std::vector<double> MaxHeightsRecordedSinceLastTypeOfExerciseChange = std::vector<double>();
    double referencedMaxHeight = 0;
    bool closed = false;
    std::vector<std::string> problemsInSet = std::vector<std::string>();
    std::vector<int> IndexesOfRepsWithProblems = std::vector<int>();
    std::vector<bool> setsWithProblemsToKeep = std::vector<bool>();

    TrainerAnalyser(Trainer* trainer, int id, std::string name, double t0) {
        this->TheTrainer = trainer;
        this->id = id;
        this->name = name;
        this->t0 = t0;
        Begin();
    };

    //virtual ~TrainerAnalyser() = default; // Ensure a virtual destructor for polymorphism
    void Begin();
    virtual void DoMission();
    double getReferencedMaxHeight();
    void closeAnalyser(double tf);
};

class InitialAnalyser : public TrainerAnalyser {
    public:
        InitialAnalyser(Trainer* trainer, int id, double t0) : TrainerAnalyser(trainer, id, "InitialAnalyser", t0) {};
        void update(bool FinishedLastSet = false);
        std::vector<double> minMaxVelocitiesOfSets = std::vector<double>();
        int numberOfSets = 0;
};

class SetDataSaver : public TrainerAnalyser {
    public:
        SetDataSaver(Trainer* trainer, int id, double t0) : TrainerAnalyser(trainer, id, "SetDataSaver", t0) {};
        void DoMission();
        std::vector<double> TimeOfSet = std::vector<double>(); 
        std::vector<std::vector<double>> HeightOfSet = std::vector<std::vector<double>>(2, std::vector<double>()); //0 : left , 1: right
        std::vector<std::vector<double>> VelocityOfSet = std::vector<std::vector<double>>(2, std::vector<double>()); //0 : left , 1: right
        std::vector<std::vector<double>> AccelerationOfSet = std::vector<std::vector<double>>(2, std::vector<double>()); //0 : left , 1: right
        bool SetUsedForOneRMExperiment = false;

        //variables for continuous analysis. The continuous analysis is not perfectly reliable but it allows to use the endurance law
        int index0_RisingPhaseOfCurrentRep = 0;
        int indexf_RisingPhaseOfCurrentRep = 0;
        std::vector<double> continuousMinHeights = {};
        std::vector<double> continuousMaxHeights = {};
        std::vector<double> continuousMaxVelocities = {};
        std::vector<double> continuouseTimeOfTopRep = {};
        int continuousNumberOfReps = 0;
};

class CounterAnalyser : public TrainerAnalyser {
    public:
        CounterAnalyser(Trainer* trainer, int id, double t0) : TrainerAnalyser(trainer, id, "RepsCounter", t0) {};
	    void DoMission();
};

class OneRMAnalyser : public TrainerAnalyser {
    public:
        OneRMAnalyser(Trainer* trainer, int id, double t0) : TrainerAnalyser(trainer, id, "OneRMAnalyser", t0) {};
        void DoMission();
        void UpdateAndPlotOneRMExperiment(bool forceAddingRegressionToBePlotted = false);
        double GetNumberOfRepAtSpecificVloss(double Vloss);
        void createEnduranceExperimentDataKnowingOneRMExperiment();
        std::vector<int> indexesOfSetDataSaver = std::vector<int>();
        double meanMinVelocities = 0;
        std::vector<double> allMinMaxVelocitiesOfExperiment = std::vector<double>(); // Minimum of the maximum velocities of each sets
        std::vector<double> OneRMExperimentLoads;
        std::vector<double> OneRMExperimentVelocities;
        double OneRMVelocityValue = 0.01;
        double OneRMValue = -1;
        std::vector<double> CoefficientsOneRMRegression = std::vector<double>(2, 0); // {0, 0, 0} will be the coefficients of the "load in kg / velocity in m/s" law
        std::vector<double> EnduranceExperimentVelocitiesLoss;
        std::vector<double> EnduranceExperimentRepetitionRatio;
        double EnduranceValue = -1;
        std::vector<double> CoefficientsEnduranceLaw = std::vector<double>(3, 0); // {0, 0, 0} will be the coefficients of the "repetitions in % / velocity loss in %" law
};

#endif // !__SERIESANALYZER_H__
