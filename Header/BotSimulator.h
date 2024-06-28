#pragma once
#include <string>

class Trainer;

class BotSimulator {
public:
    
    Trainer* TheTrainer;
    BotSimulator(Trainer* trainer) {
        this->TheTrainer = trainer;
    }

    void Begin(const float a, const float b, const float c);
    void createPoint(const float x);

private:
    float a=1.0;
    float b=-5.0;
    float c=0.0;

};

