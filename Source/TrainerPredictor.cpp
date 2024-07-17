
#include "TrainerPredictor.h"
#include "Trainer.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include "eigen-3.4.0/Eigen/Dense"
#include "boost/math/distributions/fisher_f.hpp"
#include "boost/math/distributions.hpp"

using namespace std;

std::vector<double> linspace(double start, double end, int nbreOfReps) {
    std::vector<double> result;
    double step = (end - start) / (nbreOfReps - 1);

    for (int j = 0; j < nbreOfReps; j++) {
        result.push_back(start + j * step);
    }

    return result;
}

int FindIndexOfListAfterOrBeforeSpecifiedValue(std::vector<double> t_list, double bound) {
    //if BoundIsAfter = false : finds the index of the first value from the list that is over the bound. 
    //if BoundIsAfter = true  : finds the index of the last value from the list that is under the bound value 
    for (int i = 0; i < t_list.size(); i++) {
        if (t_list[i] > bound) {
            return i;
        }
    }
    return t_list.size();
}

std::vector<int> detectPeaks(const std::vector<int>& data, int windowSize, int threshold = INT_MIN) {
    std::vector<int> peaks;

    for (size_t i = 0; i < data.size(); ++i) {
        int left = (std::max)(static_cast<int>(i) - windowSize, 0);
        int right = (std::min)(i + windowSize, data.size() - 1);
        bool isPeak = true;

        for (int j = left; j <= right; ++j) {
            if (j != i && data[j] >= data[i]) {
                isPeak = false;
                break;
            }
        }

        if (isPeak && data[i] > threshold) {
            peaks.push_back(i); // Store the index of the peak
        }
    }

    return peaks;
}



void TrainerPredictor::Begin() {
    seriesAnalyzers.push_back(std::make_unique<InitialAnalyzer>(TheTrainer, 0, 0));
}
// Function template to count analyzers of a specific type

template<typename AnalyzerType>
int TrainerPredictor::CountAnalyzersOfType() {
    int count = 0;
    for (auto& analyzer : seriesAnalyzers) { // Correctly iterating over seriesAnalyzers
        if (dynamic_cast<AnalyzerType*>(analyzer.get()) != nullptr) { // Correct use of dynamic_cast
            ++count; // Increment count if the dynamic_cast is successful
        }
    }
    return count;
}

template<typename AnalyzerType>
int TrainerPredictor::FindLastAnalyzerOfType() {
    for (int i = seriesAnalyzers.size() - 1; i >= 0; --i) {
        if (dynamic_cast<AnalyzerType*>(seriesAnalyzers[i].get()) != nullptr) {
            return i;
        }
    }
    return -1; // Return -1 if no AnalyzerType analyzer is found
}

//Useful functions to validate or not the different methods that exist. We chose the most relevant and different ones.  

//Calculate 1RM via Lander formula
float TrainerPredictor::Lander(const float weightLifted, const float nbreOfReps) {
    return weightLifted / (1.013 - 0.0267123 * nbreOfReps);
}

//Calculate 1RM via Epley formula
float TrainerPredictor::Epley(const float weightLifted, const float nbreOfReps) {
    return weightLifted * (1 + nbreOfReps / 30);
}

//Calculate 1RM via Mayhew formula 
float TrainerPredictor::Mayhew(const float weightLifted, const float nbreOfReps) {
    return 100 * weightLifted / (52.2 + 41.9 * exp(-0.055 * nbreOfReps));
}

double TrainerPredictor::FindOneRM(std::vector<double> X, std::vector<double> regressionCoefficients, int i) {
    //FindOneRM 

    //Begin Horizontal line at y=OneRMVelocityValue
    std::vector<double> XOneRM = linspace(-5, X.back() + 5, 10 * X.size());
    std::vector<double> YOneRM;
    for (double xOneRM : XOneRM) {
        YOneRM.push_back(TheTrainer->OneRMVelocityValue);
    }

    string OneRMLabel = "y = " + std::to_string(TheTrainer->OneRMVelocityValue);
    int index = -1;
    //If the plot for this regression already exists, we update it 
    for (int j = 0; j < TheTrainer->app.listLabelsToPlot.size(); ++j) {
        if (TheTrainer->app.listLabelsToPlot[j].find(OneRMLabel) != std::string::npos) {
            index = j;
            break;
        }
    }
    //If it does not exist, we create a new one at the first empty index
    if (index == -1) {
        index = TheTrainer->app.findIndexOfEmptyPlot();
    }

    TheTrainer->app.Plot(index, XOneRM, YOneRM, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], OneRMLabel, "lines"); //Draw horizontal line at 0.2
    //END Horizontal line at y=OneRMVelocityValue

    if (regressionCoefficients[0] < 0 && (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0] > 0){
        //Conjuncture point between the regression line and the horizontal line
        std::string OneRMPointlabel = "(" + std::to_string((TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0]) + " ; " + std::to_string(TheTrainer->OneRMVelocityValue) + ")";
        std::vector<double> x_values = { (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0] };
        std::vector<double> y_values = { TheTrainer->OneRMVelocityValue };
        TheTrainer->app.Plot(index + 1, x_values, y_values, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], OneRMPointlabel, "scatter"); // Plot the point of the conjuncture
        //END Conjuncture point between the regression line and the horizontal line
        return (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0];
    }
    else {
        TheTrainer->app.DeleteRobotConsoleMessage("Estimated One RM is ");
        TheTrainer->app.AddRobotConsoleError("The regression line is not decreasing so we cannot find the One RM");
        //cout << " ---- ERROR The regression line is not decreasing so we cannot find the One RM ---- " << endl;
        return -1;
    }
}

bool TrainerPredictor::calculateFTest(std::vector<double>& x, std::vector<double>& y, vector<double>& linearCoefficients, vector<double>& polynomialCoefficients) {
    //Calculating F test as defined in the article https://delladata.fr/regression-polynomiale/
    //Using Weierstrass theorem principle
    
    int n = x.size();

    if (n <= 3) {
        //Not enough points to compare both methods so we choose linear regression
		return false;
	}
    else{
        //Computing RSS1
        double al = linearCoefficients[0];
        double bl = linearCoefficients[1];
        double RSS1 = 0.0;
        for (int j = 0; j < n; ++j) {
            double yi = y[j];
            double fi = al * x[j] + bl;
            double residual = yi - fi;
            RSS1 += residual * residual;
        }

        //Computing RSS2
        double a = polynomialCoefficients[0];
        double b = polynomialCoefficients[1];
        double c = polynomialCoefficients[2];
        double RSS2 = 0.0;
        for (int j = 0; j < n; ++j) {
            double yi = y[j];
            double fi = a * x[j] * x[j] + b * x[j] + c;
            double residual = yi - fi;
            RSS2 += residual * residual;
        }

        //Computing F
        double F = ((RSS1 - RSS2) / (3 - 2)) / (RSS2 / (n - 3));

        if (F < 0 || !std::isfinite(F)) {
            // F is not finite or is negative so we choose linear regression
            //std::cout << "F is not finite or is negative " << std::to_string(F) << " so we choose linear regression" << endl;
            return false;
        }
        else {
            boost::math::fisher_f f_dist(3 - 2, n - 3);
            double p_value = 1 - boost::math::cdf(f_dist, F);
            return { p_value < 0.05 };
        }
    }
}

//Polynomial regression to get the shape of Force-Velocity and the Weight-Velocity
std::vector<double> polynomialRegression(std::vector<double>& x, std::vector<double>& y) {

    Eigen::MatrixXd X1(x.size(), 3);
    Eigen::VectorXd Y1(x.size());

    for (int j = 0; j < x.size(); ++j) {
        X1(j, 0) = x[j] * x[j];  // x^2
        X1(j, 1) = x[j];         // x
        X1(j, 2) = 1;            // constant term
        Y1(j) = y[j];
    }

    Eigen::VectorXd coeffs = X1.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(Y1);

    return { coeffs[0], coeffs[1], coeffs[2] };
}

// Linear regression to predict the shape of the Force-Velocity 
std::vector<double> linearRegression(std::vector<double>& x, std::vector<double>& y) {
    int n = x.size();
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

    for (int j = 0; j < n; ++j) {
        sum_x += x[j];
        sum_y += y[j];
        sum_xy += x[j] * y[j];
        sum_x2 += x[j] * x[j];
    }

    double x_mean = sum_x / n;
    double y_mean = sum_y / n;
    double a = (sum_xy - n * x_mean * y_mean) / (sum_x2 - n * x_mean * x_mean);
    double b = y_mean - a * x_mean;

    // Calculate R^2
    double ss_tot = 0.0;
    double ss_res = 0.0;
    for (int j = 0; j < n; ++j) {
        double yi = y[j];
        double fi = a * x[j] + b;
        ss_tot += (yi - y_mean) * (yi - y_mean);
        ss_res += (yi - fi) * (yi - fi);
    }
    double r2 = 1 - (ss_res / ss_tot);

    return { a, b };
}

void TrainerPredictor::AddGraphToDoRegressionOn(int indexOfAbscissaData, int indexOfDataToDoRegressionOn) {
    indexesOfAbscissaData.push_back(indexOfAbscissaData);
    indexesOfDataToDoRegressionOn.push_back(indexOfDataToDoRegressionOn);
}

void TrainerPredictor::EraseGraphToDoRegressionOn(int indexOfAbscissaData, int indexOfDataToDoRegressionOn) {
    std::string label = "regression " + TheTrainer->app.listLabelsToPlot[indexOfDataToDoRegressionOn];
    TheTrainer->app.Unplot(label);
    indexesOfAbscissaData.erase(std::remove(indexesOfAbscissaData.begin(), indexesOfAbscissaData.end(), indexOfAbscissaData), indexesOfAbscissaData.end());
    indexesOfDataToDoRegressionOn.erase(std::remove(indexesOfDataToDoRegressionOn.begin(), indexesOfDataToDoRegressionOn.end(), indexOfDataToDoRegressionOn), indexesOfDataToDoRegressionOn.end());
}

void TrainerPredictor::PlotRegression(std::vector<double> Xvalues, std::vector<double> coefficients, std::string subsublabel, int i, bool shallWeKeepThePlotEvenAfterAClearPlot) {
    //subsublabel = specified text to put in the label. Not necessary to put one when i is specified
    std::vector<double> X = linspace(Xvalues[0], Xvalues[Xvalues.size() - 1], 10 * Xvalues.size());
    std::vector<double> Y;
    std::string type = "";
    std::string equation = "";
    int indexOfSubPlotToPlotOn = 0;

    if (i >= 0 && i < indexesOfDataToDoRegressionOn.size()) {
        subsublabel = TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
        indexOfSubPlotToPlotOn = TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]];
    }
    if (coefficients.size() == 3) {
        type = "Polynomial";

        //Just to make annotation on the graph with the equation
        for (double xi : X) {
            double yi2 = coefficients[0] * xi * xi + coefficients[1] * xi + coefficients[2];
            Y.push_back(yi2);
        }

        equation = "y = " + std::to_string(coefficients[0]) + "x^2 " +
            (coefficients[1] < 0 ? "- " : "+ ") + std::to_string(abs(coefficients[1])) + "x " +
            (coefficients[2] < 0 ? "- " : "+ ") + std::to_string(abs(coefficients[2]));
    }
    else {
        type = "Linear";

        //Just to make annotation on the graph with the equation
        for (double xi : X) {
            double yi = coefficients[0] * xi + coefficients[1];
            Y.push_back(yi);
        }
        std::string equation = "y = " + std::to_string(coefficients[0]) + "x" +
            (coefficients[1] < 0 ? "- " : "+ ") + std::to_string(abs(coefficients[1]));
    }
    

    //Find the proper index to register the data to plot
    std::string sublabel = "regression " + subsublabel;
    int index = -1;
    //If the plot for this regression already exists, we update it 
    for (int j = 0; j < TheTrainer->app.listLabelsToPlot.size(); ++j) {
        if (TheTrainer->app.listLabelsToPlot[j].find(sublabel) != std::string::npos) {
            index = j;
            break;
        }
    }
    //If it does not exist, we create a new one at the first empty index
    if (index == -1) {
        index = TheTrainer->app.findIndexOfEmptyPlot();
    }
    if (index != -1){
        std::string label = type + " regression " + subsublabel;
        TheTrainer->app.Plot(index, X, Y, indexOfSubPlotToPlotOn, label, "lines", equation, shallWeKeepThePlotEvenAfterAClearPlot);
    }
    else {
        cout << "---- ERROR : Could not find empty lot to plot the regression" << endl;
        TheTrainer->app.AddRobotConsoleError("ERROR : Could not find empty lot to plot the regression");
    }
}

void TrainerPredictor::predictRegression() {
    for (int i = 0; i < indexesOfAbscissaData.size(); i++)
    {

        std::vector<double> x = TheTrainer->app.listXToPlot[indexesOfAbscissaData[i]];
        std::vector<double> y = TheTrainer->app.listYToPlot[indexesOfDataToDoRegressionOn[i]];

        if (y.size() > 2) {
            if (std::to_string(y[0]) != "-nan(ind)"){
                std::string line = "";

                std::vector<double> coefficients_pol = polynomialRegression(x, y);
                std::vector<double> coefficients_lin = linearRegression(x, y);

                std::vector<double> X = linspace(x[0], x[x.size() - 1], 10 * x.size());
                std::vector<double> Y;
                std::vector<double> Y2;
                bool isPolynomial = calculateFTest(x, y, coefficients_lin, coefficients_pol);

                if (TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]] == "Max velocity during reps") {
                    while (coefficients_lin[0] * X.back() + coefficients_lin[1] > 0 && X.size() < 1000) {
                        X.push_back(X.back() + 1); // Increment the last value in X by 1kg
                    }
                    double OneRM = FindOneRM(X, coefficients_lin, i);
                    if (OneRM != -1) {
                        TheTrainer->app.DeleteRobotConsoleError(" so we cannot find the One RM ");
                        TheTrainer->app.DeleteRobotConsoleMessage("Estimated One RM is ");
                        TheTrainer->app.AddRobotConsoleMessage("Estimated One RM is " + std::to_string(OneRM) + " kg");
					}
                }

                if (isPolynomial) {
                    //We tell user that polynomial regression fits better
                    line = "Polynomial regression fits better on " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                    PlotRegression(x, coefficients_pol, "", i, TheTrainer->app.listOfPlotsToKeep[indexesOfDataToDoRegressionOn[i]]);
                }
                else {
                    //We tell user that linear regression fits better
                    line = "Linear regression fits better on " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                    PlotRegression(x, coefficients_lin, "", i, TheTrainer->app.listOfPlotsToKeep[indexesOfDataToDoRegressionOn[i]]);
                }
                //We add the message to the console
                std::string lineString = line;
                std::string subLine = lineString.substr(11);
                TheTrainer->app.DeleteRobotConsoleMessage(subLine);
                //TheTrainer->app.AddRobotConsoleMessage(line);         //We do not want to add this message to the console because it is not useful for the user
                cout << line << endl;
            }
            else {
                TheTrainer->app.AddRobotConsoleError("Issue doing regression. Values in y axis are not defined : y[0] = " + std::to_string(y[0]));
            }
        }
    }
}

void TrainerPredictor::BotStartedSet(vector<double> t_list) {
    cout << "START SET" << endl;
    int index = CountAnalyzersOfType<SetDataSaver>();
    seriesAnalyzers.push_back(std::make_unique<SetDataSaver>(TheTrainer, index, t_list.back()));
    dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->numberOfSets++;
}

void TrainerPredictor::BotFinishedSet(vector<double> t_list) {
    cout << "STOP SET" << endl;
    for (auto& analyzer : seriesAnalyzers) {
        if (!analyzer.get()->closed && analyzer.get()->name == "SetDataSaver") {
            analyzer.get()->closeAnalyzer(t_list.back()); 
            if (analyzer.get()->maxVelocities.size() != 0){
                dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->minMaxVelocitiesOfSets.push_back(*std::min_element(analyzer.get()->maxVelocities.begin(), analyzer.get()->maxVelocities.end()));
            }
        }
    }
}

void TrainerPredictor::LepleyExperiment(vector<double> t_list) {
    // COUNTING NUMBER OF REPS EXPERIMENT
    if (WantCountNumberOfReps) {
        cout << "START LEPLEY EXPERIMENT" << endl;
        int index = CountAnalyzersOfType<CounterAnalyzer>();
        seriesAnalyzers.push_back(std::make_unique<CounterAnalyzer>(TheTrainer, index, t_list.back()));
        WantCountNumberOfReps = false;
        CountingNumberOfReps = true;
    }
    if (CountingNumberOfReps) {
        for (auto& analyzer : seriesAnalyzers) {
            if (!analyzer.get()->closed && analyzer.get()->name == "RepsCounter") {
                analyzer.get()->DoMission();
            }
        }
        if (!TheTrainer->BotWorkingInProgress) {
            WantStopCountNumberOfReps = true;
        }
    }
    if (WantStopCountNumberOfReps) {
        cout << "STOP LEPLEY EXPERIMENT" << endl;
        for (auto& analyzer : seriesAnalyzers) {
            if (!analyzer.get()->closed && analyzer.get()->name == "RepsCounter") { analyzer.get()->closeAnalyzer(t_list.back()); }
        }
        CountingNumberOfReps = false;
        WantStopCountNumberOfReps = false;
    }
    // END COUNTING NUMBER OF REPS EXPERIMENT
}

void TrainerPredictor::ContinuousOneRMExperiment(vector<double> t_list) {
    // CONTINUOUS ONE RM EXPERIMENT
    if (WantContinuous1RMExperiment) {
        int index = CountAnalyzersOfType<ContinuousOneRMAnalyzer>();
        seriesAnalyzers.push_back(std::make_unique<ContinuousOneRMAnalyzer>(TheTrainer, index, t_list.back()));
        WantContinuous1RMExperiment = false;
        DoingContinuous1RMExperiment = true;
    }
    if (DoingContinuous1RMExperiment) {
        for (auto& analyzer : seriesAnalyzers) {
            if (!analyzer.get()->closed && analyzer.get()->name == "ContinuousOneRMAnalyzer") {
                if (t_list.back() == analyzer.get()->timeWhenWeFoundMaxVelocity){ //The mission of this analyzer is to update the max velocities found. 
                    analyzer.get()->DoMission();
                }
            }
        }
        if (!TheTrainer->BotWorkingInProgress) {
            WantStopContinuous1RMExperiment = true;
        }
    }
    if (WantStopContinuous1RMExperiment) {
        for (auto& analyzer : seriesAnalyzers) {
            if (!analyzer.get()->closed && analyzer.get()->name == "ContinuousOneRMAnalyzer") { analyzer.get()->closeAnalyzer(t_list.back()); }
        }
        DoingContinuous1RMExperiment = false;
        WantStopContinuous1RMExperiment = false;
    }
    // END CONTINUOUS ONE RM EXPERIMENT
}

void TrainerPredictor::AnalyzeData() {
    std::vector<double> t_list = TheTrainer->app.listXToPlot[36-1];
    std::vector<double> distance_list = TheTrainer->app.listYToPlot[36-1];
    std::vector<double> velocity_list = TheTrainer->app.listYToPlot[37-1];

    // ANALYZING SERIES FOR ONE RM      -->   Old way to do that we kept. In the future, it will be good TODO a silent analyzer that we call at the end of the series. The same way we did the SeriesAnalyzers. 
    if (WantToAnalyzeSeriesForOneRM) {
        TheTrainer->app.DeleteRobotConsoleMessage("Max velocity of the repetition ");
        TheTrainer->app.DeleteRobotConsoleWarning("The final lobe is not finished ");
        TheTrainer->app.DeleteRobotConsoleWarning("Max velocity found is absurd ");
        if (!(t_list == std::vector<double>{0.0, 0.0, 0.0} || distance_list == std::vector<double>{0.0, 0.0, 0.0} || velocity_list == std::vector<double>{0.0, 0.0, 0.0} || t_list.size() == 0 || distance_list.size() == 0 || velocity_list.size() == 0)) {
            std::cout << "Velocity list " << velocity_list[0] << " " << velocity_list.back() << " " << velocity_list.size() << endl;
            //Get the boundaries and the portion of the lists we are studying
            double minBound = (std::max)(TheTrainer->app.minBound, t_list.front());
            double maxBound = (std::min)(TheTrainer->app.maxBound, t_list.back());
            auto indexMin = FindIndexOfListAfterOrBeforeSpecifiedValue(t_list, minBound);
            auto indexMax = FindIndexOfListAfterOrBeforeSpecifiedValue(t_list, maxBound);
            //Make sure indexMax and indexMin are credible indexes 
            indexMax = ((indexMax) < (static_cast<decltype(indexMax)>(t_list.size()))) ? (indexMax) : (static_cast<decltype(indexMax)>(t_list.size()));
            indexMin = ((indexMin) < (static_cast<decltype(indexMin)>(t_list.size()))) ? (indexMin) : (static_cast<decltype(indexMin)>(t_list.size()));
            std::cout << "indexMin " << indexMin << " indexMax " << indexMax << endl;
            std::vector<double> XSubList;
            std::vector<double> DistanceSubList;
            std::vector<double> VelocitySubList;
            if (static_cast<size_t>(indexMin) < static_cast<size_t>(indexMax) && static_cast<size_t>(indexMin) < t_list.size() && static_cast<size_t>(indexMax) <= t_list.size()) {
                XSubList = std::vector<double>(t_list.begin() + indexMin, t_list.begin() + indexMax);
                DistanceSubList = std::vector<double>(distance_list.begin() + indexMin, distance_list.begin() + indexMax); //Distance in the interval
                VelocitySubList = std::vector<double>(velocity_list.begin() + indexMin, velocity_list.begin() + indexMax); //Velocity in the interval

                std::cout << "VelocitySubList " << VelocitySubList[0] << " " << VelocitySubList.back() << " " << VelocitySubList.size() << endl;
                //Understand where the lobe is in velocityData (It means where is the rising up of the load)
                std::vector<int> startLobes = {};
                std::vector<int> finishLobes = {};
                for (int i = 1; i < VelocitySubList.size(); i++) {
                    if (VelocitySubList[i] > 0 && VelocitySubList[i - 1] <= 0) { //We just started a lobe
                        std::cout << "We found the start point of the lobe at " << i << " " << VelocitySubList[i - 1] << " " << VelocitySubList[i] << endl;
                        startLobes.push_back(i);
                        finishLobes.push_back(VelocitySubList.size());
                    }
                    if (VelocitySubList[i] < 0 && VelocitySubList[i - 1] >= 0 && startLobes.size() != 0) { //We just finished a lobe
                        std::cout << "We found the end point of the lobe at " << i << " " << VelocitySubList[i - 1] << " " << VelocitySubList[i] << endl;
                        finishLobes.back() = i;
                    }
                }
                for (int k = 0; k < startLobes.size(); k++) {
                    if (finishLobes[k] != VelocitySubList.size()) {

                        std::vector<double> TimeLobe = std::vector<double>(XSubList.begin() + startLobes[k], XSubList.begin() + finishLobes[k]);
                        std::vector<double> VelocityLobe = std::vector<double>(VelocitySubList.begin() + startLobes[k], VelocitySubList.begin() + finishLobes[k]);
                        std::vector<double> DistanceLobe = std::vector<double>(DistanceSubList.begin() + startLobes[k], DistanceSubList.begin() + finishLobes[k]);

                        //Do polynomial regression on the lobe
                        std::cout << "TimeLobe " << TimeLobe[0] << " " << TimeLobe.back() << " " << TimeLobe.size() << endl;
                        std::cout << "VelocityLobe " << VelocityLobe[0] << " " << VelocityLobe.back() << " " << VelocityLobe.size() << endl;
                        std::vector<double> coefficients_pol = polynomialRegression(TimeLobe, VelocityLobe);

                        //find the max of the polynomial regression
                        double maxVelocity = (-coefficients_pol[1] * coefficients_pol[1] / (4 * coefficients_pol[0])) + coefficients_pol[2]; //max value of the polynomial regression
                        double timeAtMaxVelocity = -coefficients_pol[1] / (2 * coefficients_pol[0]); //time at max value of the polynomial regression

                        double maxVelocityNormalized = (maxVelocity - *std::min_element(VelocityLobe.begin(), VelocityLobe.end())) / (*std::max_element(VelocityLobe.begin(), VelocityLobe.end()) - *std::min_element(VelocityLobe.begin(), VelocityLobe.end())); // We want to compare the max Velcity found with the max Velocity acquired to make sure it is not an absurd value.
                        if (maxVelocityNormalized > 1 - 0.2 && maxVelocityNormalized < 1 + 0.2) {  //If max Velocity found is inbetween min and max values acquired (with a 20% margin), we keep it. Otherwise we consider it is an absurd value so we don't keep the lobe analysis
                            std::cout << "Max velocity is " << maxVelocity << " at time " << timeAtMaxVelocity << endl;
                            TheTrainer->app.AddRobotConsoleMessage("Max velocity of the repetition " + std::to_string(k) + " is " + std::to_string(maxVelocity) + " at time " + std::to_string(timeAtMaxVelocity));

                            std::vector<double> pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                            pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                            TheTrainer->app.addPointToPlot(pointCoordinates, "maxVelocity " + std::to_string(k));

                            //Plot polynomial regression
                            PlotRegression(TimeLobe, coefficients_pol, "on lobe " + std::to_string(k), -1, false);

                            TheTrainer->OneRMExperimentLoads.push_back(TheTrainer->botManager.TotalWeightWanted);
                            TheTrainer->OneRMExperimentVelocities.push_back(maxVelocity);
                            TheTrainer->app.Plot(50, TheTrainer->OneRMExperimentLoads, TheTrainer->OneRMExperimentVelocities, 1, "Max velocity during reps", "scatter");
                        }
                        else {
                            std::cout << " -- WARNING Max velocity found is absurd " << maxVelocity << " at time " << timeAtMaxVelocity << " after analyzing lobe number " << k << " so we don't keep this lobe analysis -- " << endl;
                            TheTrainer->app.AddRobotConsoleWarning("WARNING Max velocity found is absurd " + std::to_string(maxVelocity) + " at time " + std::to_string(timeAtMaxVelocity) + " on lobe number " + std::to_string(k));
                            std::vector<double> pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                            pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                            TheTrainer->app.addPointToPlot(pointCoordinates, "maxVelocity " + std::to_string(k));

                            //Plot polynomial regression
                            PlotRegression(TimeLobe, coefficients_pol, "on lobe " + std::to_string(k), -1, false);
                        }
                    }
                    else {
                        TheTrainer->app.AddRobotConsoleWarning("WARNING The final lobe is not finished so we don't keep it");
                        std::cout << " -- WARNING The final lobe is not finished so we don't keep it -- " << endl;
                    }
                }
            }
            else {
                TheTrainer->app.AddRobotConsoleError("Error in AnalyzeData: minBound or maxBound are not in the list of X values");
                std::cout << " ---- ERROR in AnalyzeData: minBound or maxBound are not in the list of X values ---- " << endl;
                std::cout << "Are you sure that t_list is actually the abscissa list of the y list that you want to analyze ?" << endl;
            }
        }
        else {
            TheTrainer->app.AddRobotConsoleError("Error in AnalyzeData: the lists you want to analyze are empty (or equal to {0, 0, 0})");
            std::cout << " ---- ERROR : the lists you want to analyze are empty (or equal to {0, 0, 0}) ---- " << endl;
        }
        WantToAnalyzeSeriesForOneRM = false;
        std::cout << "End of AnalyzeData" << endl;
    }
    // END ANALYZING SERIES FOR ONE RM

    if (lastTimeWeAnalyzedData != t_list.back()) { //We analyze only if there is new data
        double epsilon = 0.005;
        if (velocity_list.back() > 0 + epsilon && velocity_list[velocity_list.size() - 2] <= 0 + epsilon) { //We just started a lobe
            //We tell all the observers that a new lobe has been detected
            for (auto& analyzer : seriesAnalyzers) {
                if (!analyzer.get()->closed){
                    analyzer.get()->index0_RisingPhaseOfCurrentRep = t_list.size() - 1;
                    //cout<<"We just started a rep at t = " << t_list[analyzer.get()->index0_RisingPhaseOfCurrentRep] <<endl;
                    analyzer.get()->duringARep = true;
                    analyzer.get()->numberOfReps++;
                }
            }
        }
        if (velocity_list.back() < 0 + epsilon && velocity_list[velocity_list.size() - 2] >= 0 + epsilon) { //We just finished a lobe, we are at max height
            //We tell all the observers that the max height has been reached
            for (auto& analyzer : seriesAnalyzers) {
                if (!analyzer.get()->closed && analyzer.get()->duringARep) {
                    //cout<<"We just finished a lobe. from t = " << t_list[analyzer.get()->index0_RisingPhaseOfCurrentRep] << " to t = " << t_list[t_list.size() - 1] << analyzer.get()->duringARep <<endl;
                    analyzer.get()->maxHeights.push_back(distance_list.back());
                    analyzer.get()->indexf_RisingPhaseOfCurrentRep = t_list.size() - 1;

                    std::vector<double> TimeLobe = std::vector<double>(t_list.begin() + analyzer.get()->index0_RisingPhaseOfCurrentRep, t_list.begin() + analyzer.get()->indexf_RisingPhaseOfCurrentRep);
                    std::vector<double> VelocityLobe = std::vector<double>(velocity_list.begin() + analyzer.get()->index0_RisingPhaseOfCurrentRep, velocity_list.begin() + analyzer.get()->indexf_RisingPhaseOfCurrentRep);
                    std::vector<double> DistanceLobe = std::vector<double>(distance_list.begin() + analyzer.get()->index0_RisingPhaseOfCurrentRep, distance_list.begin() + analyzer.get()->indexf_RisingPhaseOfCurrentRep);

                    //Do polynomial regression on the lobe
                    std::vector<double> coefficients_pol = polynomialRegression(TimeLobe, VelocityLobe);

                    //find the max of the polynomial regression
                    double maxVelocity = (-coefficients_pol[1] * coefficients_pol[1] / (4 * coefficients_pol[0])) + coefficients_pol[2]; //max value of the polynomial regression
                    double timeAtMaxVelocity = -coefficients_pol[1] / (2 * coefficients_pol[0]); //time at max value of the polynomial regression

                    double maxVelocityNormalized = (maxVelocity - *std::min_element(VelocityLobe.begin(), VelocityLobe.end())) / (*std::max_element(VelocityLobe.begin(), VelocityLobe.end()) - *std::min_element(VelocityLobe.begin(), VelocityLobe.end())); // We want to compare the max Velcity found with the max Velocity acquired to make sure it is not an absurd value.
                    if (maxVelocityNormalized > 1 - 0.3 && maxVelocityNormalized < 1 + 0.3) {  //If max Velocity found is inbetween min and max values acquired (with a 20% margin), we keep it. Otherwise we consider it is an absurd value so we don't keep the lobe analysis
                        if (DoingContinuous1RMExperiment) {
                            std::cout << "Max velocity is " << maxVelocity << " at time " << timeAtMaxVelocity << endl;
                            TheTrainer->app.AddRobotConsoleMessage("Max velocity of the last repetition is " + std::to_string(maxVelocity) + " at time " + std::to_string(timeAtMaxVelocity));
                        }
                    }
                    else {
                        if (DoingContinuous1RMExperiment) {
                            std::cout << " -- WARNING Max velocity found is absurd " << maxVelocity << " at time " << timeAtMaxVelocity << " after analyzing the last lobe. So we keep the acquired value -- " << endl;
                            TheTrainer->app.AddRobotConsoleWarning("WARNING Max velocity found is absurd " + std::to_string(maxVelocity) + " at time " + std::to_string(timeAtMaxVelocity) + " on the last lobe. So we keep the acquired value at " + std::to_string(*std::max_element(VelocityLobe.begin(), VelocityLobe.end())));
                        }
                        maxVelocity = *std::max_element(VelocityLobe.begin(), VelocityLobe.end());
                        timeAtMaxVelocity = TimeLobe[std::distance(VelocityLobe.begin(), std::max_element(VelocityLobe.begin(), VelocityLobe.end()))];
                    }
                    analyzer.get()->maxVelocities.push_back(maxVelocity);
                    analyzer.get()->timeWhenWeFoundMaxVelocity = t_list.back(); // The time at which we found the max velocity of the last repetition. NOT the time at which the MaxVelocity has been reached
                    std::vector<double> pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                    pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                    TheTrainer->app.addPointToPlot(pointCoordinates, "maxVelocity");

                    //Plot polynomial regression
                    PlotRegression(TimeLobe, coefficients_pol, "last repetition", -1, false);
                } 
            }
        }
        LepleyExperiment(t_list);
        ContinuousOneRMExperiment(t_list);

        lastTimeWeAnalyzedData = t_list.back();
	}
    
    int indexLastSetDataSaverAnalyzer = FindLastAnalyzerOfType<SetDataSaver>();
    if (indexLastSetDataSaverAnalyzer != -1) {
        if (TheTrainer->BotWorkingInProgress && (TheTrainer->Username != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->Username || TheTrainer->TypeOfExercice != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->TypeOfExercice || TheTrainer->botManager.TotalWeightWanted != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->TotalWeightWanted || TheTrainer->UsingLoadProfile != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->UsingLoadProfile)) {
            //We finished a set without turning the motors off!!
            BotFinishedSet(t_list);
            BotStartedSet(t_list);
        }
    }
    dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->update();

    predictRegression(); // Will predict the regression for the data that has been added to the list of data to do regression on
}