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

class Trainer;

class TrainerSimulator {
public:
    
    Trainer* TheTrainer;
    TrainerSimulator(Trainer* trainer) {
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

