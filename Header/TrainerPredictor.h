#pragma once

#include <string>
#include <vector>
#include <SeriesAnalyzer.h>
#include <memory> // For std::unique_ptr

using std::vector;

class Trainer;

class TrainerPredictor {
public:

    Trainer* TheTrainer;

    TrainerPredictor(Trainer* trainer) {
        this->TheTrainer = trainer;
        Begin();
    }
    template<typename AnalyzerType>
    int CountAnalyzersOfType();
    template<typename AnalyzerType>
    int FindLastAnalyzerOfType();

    void Begin();
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
    void LepleyExperiment(vector<double> t_list);
    void ContinuousOneRMExperiment(vector<double> t_list);
    void BotStartedSet(vector<double> t_list);
    void BotFinishedSet(vector<double> t_list);

    vector<int> indexesOfAbscissaData = vector<int>();
    vector<int> indexesOfDataToDoRegressionOn = vector<int>();
    
    bool WantToAnalyzeSeriesForOneRM = false;
    bool WantCountNumberOfReps = false;
    bool CountingNumberOfReps = false;
    bool WantStopCountNumberOfReps = false;
    
    bool WantContinuous1RMExperiment = false;
    bool DoingContinuous1RMExperiment = false;
    bool WantStopContinuous1RMExperiment = false;
    std::vector<std::unique_ptr<SeriesAnalyzer>> seriesAnalyzers = {}; // We use unique_ptr to avoid memory leaks. This is called dynamic memory allocation. The first Analyzer is the initial Analyzer, the one that carries all the information during the entire use of the app

    double lastTimeWeAnalyzedData = 0;
};
