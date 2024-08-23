#pragma once
#include <string>
#include <vector>

class Trainer;

class BotSimulator {
public:
    
    Trainer* TheTrainer;
    BotSimulator(Trainer* trainer) {
        this->TheTrainer = trainer;
        Begin(1.0, -5.0, 0.0);
    }

    void Begin(const float a, const float b, const float c);
    void createPoint(const float x);
    void makeSetAtLoad(const float load);
    void AddDataSimulated(int tIndex);

    std::vector<double> t_list = {};
    std::vector<double> d_list = {};
    std::vector<double> v_list = {};
    std::vector<double> a_list = {};

private:
    float a=1.0;
    float b=-5.0;
    float c=0.0;

    //if closest == 7
    
};

