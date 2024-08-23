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
#include <TrainerAnalyser.h>
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
    void Begin();
    template<typename AnalyserType>
    int CountAnalysersOfType();
    template<typename AnalyserType>
    int FindLastAnalyserOfType();
    template<typename AnalyserType>
    int FindAnalyserByItsIdOfType(int index);
    void DefinePopupWindowOneRM3StepsIntermediateWindow();
    void PrintOutTheSetDataForFutureSimulations();
    float Lander(const float weightLifted, const float nbreOfReps);
    float Epley(const float weightLifted, const float nbreOfReps);
    float Mayhew(const float weightLifted, const float nbreOfReps);
    double FindOneRM(std::vector<double> X, std::vector<double> regressionCoefficients, int i);
    double FindPersonnalRecord();
    bool calculateFTest(std::vector<double>& x, std::vector<double>& y, vector<double>& linearCoefficients, vector<double>& polynomialCoefficients);
    void CompareLastSetWithOneRM();
    void AnalyzeData();
    void AnalyzeLastSet(vector<double> t_list);
    void ContinuousRepCounter(std::vector<double> t_list, std::vector<double> distance_list, std::vector<double>velocity_list);
    void predictRegression();
    void PlotRegression(std::vector<double> Xvalues, std::vector<double> coefficients, std::string subsublabel = "", int i = -1, bool shallWeKeepThePlotEvenAfterAClearPlot = true);
    void AddGraphToDoRegressionOn(int indexOfAbscissaData, int indexOfDataToDoRegressionOn);
    void EraseGraphToDoRegressionOn(int indexOfAbscissaData, int indexOfDataToDoRegressionOn, bool WantUnplot = true);
    void LepleyExperiment(vector<double> t_list);
    void OneRMExperiment(vector<double> t_list);
    void BotStartedSet(vector<double> t_list);
    void BotFinishedSet(vector<double> t_list);

    vector<int> indexesOfAbscissaData = vector<int>();
    vector<int> indexesOfDataToDoRegressionOn = vector<int>();
    
    std::vector<std::unique_ptr<TrainerAnalyser>> sessionAnalysers = {}; // We use unique_ptr to avoid memory leaks. This is called dynamic memory allocation. The first Analyser is the initial Analyser, the one that carries all the information during the entire use of the app
    
    //All experiments starting and ending booleans
    bool WantStartOneRMExperiment = false;
    bool DoingOneRMExperiment = false;
    bool WantStopOneRMExperiment = false;

    bool WantStartLepleyExperiment = false;
    bool DoingLepleyExperiment = false;
    bool WantStopLepleyExperiment = false;

    //Set Analysis
    double lastTimeWeAnalyzedData = 0;
    bool FinishedSet = false;
    double PercentageOfSet = -1;
    double MaxNumberRepsEstimated = -1;
    double CurrentNumberOfRepsDoneInTheSet = -1;
    double personnalRecordOfNumberOfRepsAtCurrentLoad = 0;
};
