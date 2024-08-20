#pragma once
#include <string>
#include <vector>

class Trainer;

class BotSimulator {
public:
    
    Trainer* TheTrainer;
    BotSimulator(Trainer* trainer) {
        this->TheTrainer = trainer;
    }

    void makeSetAtLoad(const float load);
    void AddSimulatedData(int tIndex);

    std::vector<double> t_list = {};
    std::vector<double> d_list = {};
    std::vector<double> v_list = {};
    std::vector<double> a_list = {};

};

