#pragma once

#include <string>
#include <vector>
using std::vector;

class Trainer;

class TrainerPredictor {
public:

    Trainer* TheTrainer;
    TrainerPredictor(Trainer* trainer) {
        this->TheTrainer = trainer;
    }
    float Lander(const float weightLifted, const float nbreOfReps);
    float Epley(const float weightLifted, const float nbreOfReps);
    float Mayhew(const float weightLifted, const float nbreOfReps);
    double FindOneRM(std::vector<double> X, std::vector<double> regressionCoefficients, int i);
    bool calculateFTest(std::vector<double>& x, std::vector<double>& y, vector<double>& linearCoefficients, vector<double>& polynomialCoefficients);
    void AnalyzeData();
    void predictRegression();
    void PlotRegression(std::vector<double> Xvalues, std::vector<double> coefficients, std::string subsublabel = "", int i = -1, bool shallWeKeepThePlotEvenAfterAClearPlot = true);
    void AddGraphToDoRegressionOn(int indexOfAbscissaData, int indexOfDataToDoRegressionOn);
    void EraseGraphToDoRegressionOn(int indexOfAbscissaData, int indexOfDataToDoRegressionOn);

    vector<int> indexesOfAbscissaData = vector<int>();
    vector<int> indexesOfDataToDoRegressionOn = vector<int>();
    bool WantToAnalyzeData = false;
};
