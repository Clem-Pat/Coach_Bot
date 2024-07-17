#ifndef __SERIESANALYZER_H__
#define __SERIESANALYZER_H__


#pragma once

#include <vector>
#include <limits>
#include <string>
#include <iostream>

class Trainer;

class SeriesAnalyzer {
public:
    Trainer* TheTrainer;
    int id = 0;
    std::string name = "";
    std::string Username = "";
    std::string TypeOfExercice = "";
    double TotalWeightWanted = 0; //equal to the TorqueWanted at the current time of the set
    bool UsingLoadProfile = false;
    std::vector<std::vector<double>> profileShape = std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0)); // The profile shape of the exercise. It is a vector of vectors. Each vector is a point of the trapeze profile.
    double t0 = 0;
    double tf = std::numeric_limits<double>::infinity();
    double duration = 0;
    int numberOfReps = 0;
    double meanMaxHeight = 0;
    double meanMaxVelocity = 0;
    std::vector<double> maxHeights = std::vector<double>();
    std::vector<double> maxVelocities = std::vector<double>();
    std::vector<double> MaxHeightsRecordedSinceLastTypeOfExerciceChange = std::vector<double>();
    double referencedMaxHeight = 0;
    bool closed = false;
    bool duringARep = false;
    double index0_RisingPhaseOfCurrentRep = 0;
    double indexf_RisingPhaseOfCurrentRep = 0;
    double timeWhenWeFoundMaxVelocity = 0; // The time at which we found the max velocity of the last repetition. NOT the time at which the MaxVelocity has been reached

    SeriesAnalyzer(Trainer* trainer, int id, std::string name, double t0) {
        this->TheTrainer = trainer;
        this->id = id;
        this->name = name;
        this->t0 = t0;
        Begin();
    };

    //virtual ~SeriesAnalyzer() = default; // Ensure a virtual destructor for polymorphism
    void Begin();
    virtual void DoMission();
    double getMeanMaxHeight();
    void closeAnalyzer(double tf);
};

class InitialAnalyzer : public SeriesAnalyzer {
    public:
        InitialAnalyzer(Trainer* trainer, int id, double t0) : SeriesAnalyzer(trainer, id, "InitialAnalyzer", t0) {};
        void update();
        std::vector<double> minMaxVelocitiesOfSets = std::vector<double>();
        int numberOfSets = 0;
};

class SetDataSaver : public SeriesAnalyzer {
    public:
        SetDataSaver(Trainer* trainer, int id, double t0) : SeriesAnalyzer(trainer, id, "SetDataSaver", t0) {};
};

class CounterAnalyzer : public SeriesAnalyzer {
    public:
        CounterAnalyzer(Trainer* trainer, int id, double t0) : SeriesAnalyzer(trainer, id, "RepsCounter", t0) {};
	    void DoMission();
};

class ContinuousOneRMAnalyzer : public SeriesAnalyzer {
	public:
		ContinuousOneRMAnalyzer(Trainer* trainer, int id, double t0) : SeriesAnalyzer(trainer, id, "ContinuousOneRMAnalyzer", t0) {};
	    void DoMission();
};

#endif // !__SERIESANALYZER_H__
