
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

void eraseTheProblematicReps(SetDataSaver* lastDataSaver) {
    for (int j = 0; j < lastDataSaver->setsWithProblemsToKeep.size(); j++) {
        if (!lastDataSaver->setsWithProblemsToKeep[j]) {
            int index = lastDataSaver->IndexesOfRepsWithProblems[j];
            lastDataSaver->TimeOfSet.erase(lastDataSaver->TimeOfSet.begin() + lastDataSaver->indexesEnteringARep[index], lastDataSaver->TimeOfSet.begin() + lastDataSaver->indexesExitingARep[index]);
            for (int i = 0; i < 2; i++) {
                lastDataSaver->HeightOfSet[i].erase(lastDataSaver->HeightOfSet[i].begin() + lastDataSaver->indexesEnteringARep[index], lastDataSaver->HeightOfSet[i].begin() + lastDataSaver->indexesExitingARep[index]);
                lastDataSaver->VelocityOfSet[i].erase(lastDataSaver->VelocityOfSet[i].begin() + lastDataSaver->indexesEnteringARep[index], lastDataSaver->VelocityOfSet[i].begin() + lastDataSaver->indexesExitingARep[index]);
                lastDataSaver->AccelerationOfSet[i].erase(lastDataSaver->AccelerationOfSet[i].begin() + lastDataSaver->indexesEnteringARep[index], lastDataSaver->AccelerationOfSet[i].begin() + lastDataSaver->indexesExitingARep[index]);
            }
            lastDataSaver->maxHeights.erase(lastDataSaver->maxHeights.begin() + index);
            lastDataSaver->maxVelocities.erase(lastDataSaver->maxVelocities.begin() + index);
            lastDataSaver->numberOfReps--;
            lastDataSaver->indexesEnteringARep.erase(lastDataSaver->indexesEnteringARep.begin() + index);
            lastDataSaver->indexesExitingARep.erase(lastDataSaver->indexesExitingARep.begin() + index);
            lastDataSaver->closeAnalyzer(lastDataSaver->TimeOfSet.back());
        }
    }
}

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

std::vector<int> detectPeaksBoudaries(const std::vector<int>& data, int windowSize, int threshold = INT_MIN) {
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

std::vector<std::vector<double>> findPeaksBoundaries(std::vector<double>& data, double offsetValue) {
    //returns the indexes of the beginning of the peaks and the indexes of the end of the peaks as soon as data is over the offsetValue
    std::vector<double> indexesOfBeginOfPeaks = std::vector<double>();
    std::vector<double> indexesOfEndOfPeaks = std::vector<double>();
    bool hasValueAboveOffset = std::any_of(data.begin(), data.end(), [offsetValue](double value) { return value > offsetValue; });
    if (hasValueAboveOffset) {
        if (data[0] > offsetValue) { indexesOfBeginOfPeaks.push_back(0); }
        for (int i = 0; i < data.size() - 1; i++) {
            if (data[i] <= offsetValue && data[i + 1] > offsetValue) {
                indexesOfBeginOfPeaks.push_back(i);
            }
            if (data[i] > offsetValue && data[i + 1] <= offsetValue) {
                indexesOfEndOfPeaks.push_back(i);
            }
        }
        if (data.back() > offsetValue) { indexesOfEndOfPeaks.push_back(data.size() - 1); }
        return { indexesOfBeginOfPeaks , indexesOfEndOfPeaks };
    }
    else {
        //cout<< "ERROR : No value above the offsetValue" << endl;
        return { indexesOfBeginOfPeaks , indexesOfEndOfPeaks };
    }
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

template<typename AnalyzerType>
int TrainerPredictor::FindAnalyzerByItsIdOfType(int id) {
	for (int i = 0; i < seriesAnalyzers.size(); i++) {
		if (dynamic_cast<AnalyzerType*>(seriesAnalyzers[i].get()) != nullptr && seriesAnalyzers[i].get()->id == id) {
			return i;
		}
	}
	return -1; // Return -1 if no AnalyzerType analyzer is found
}
//Useful functions to validate or not the different methods that exist. We chose the most relevant and different ones.  

void TrainerPredictor::definePopupWindowOneRM3StepsIntermediateWindow() {
    
    ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x - 750) * 0.5f, (ImGui::GetIO().DisplaySize.y - 400) * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(750, 400));
    if (ImGui::BeginPopupModal("One RM by 3 steps method (intermediate window)", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        OneRMAnalyzer* OneRManalyzer = dynamic_cast<OneRMAnalyzer*>(seriesAnalyzers[FindLastAnalyzerOfType<OneRMAnalyzer>()].get());
        if (FindLastAnalyzerOfType<SetDataSaver>() != -1) {
            SetDataSaver* lastDataSaver = dynamic_cast<SetDataSaver*>(seriesAnalyzers[FindAnalyzerByItsIdOfType<SetDataSaver>(OneRManalyzer->indexesOfSetDataSaver.back())].get());
            if (lastDataSaver->setsWithProblemsToKeep.size() == 0) { lastDataSaver->setsWithProblemsToKeep = std::vector<bool>(lastDataSaver->problemsInSet.size(), true); }
            ImGui::Text("You finished the set number %d at %.2f kg", OneRManalyzer->indexesOfSetDataSaver.size(), lastDataSaver->TotalWeightWanted);
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::Text("   Number of repetitions : %d", lastDataSaver->numberOfReps);
            ImGui::Text("   Highest maximum velocity : %.2f at repetition number %d", lastDataSaver->maxMaxVelocities, lastDataSaver->indexOfRepOfMaxMaxVelocities + 1);
            ImGui::Text("   Lowest maximum velocity : %.2f at repetition number %d", lastDataSaver->minMaxVelocities, lastDataSaver->indexOfRepOfMinMaxVelocities + 1);
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            if (lastDataSaver->problemsInSet.size() > 0) {
                for (int j = 0; j < lastDataSaver->problemsInSet.size(); j++) {
                    ImGui::Text((lastDataSaver->problemsInSet[j] + " Do you want to keep this rep ? ").c_str());
                    ImGui::SameLine();
                    std::string buttonText = lastDataSaver->setsWithProblemsToKeep[j] ? "Yes" : "No"; // Toggle button text based on the flag
                    if (ImGui::Button(buttonText.c_str(), ImVec2(50, 0))) {
                        lastDataSaver->setsWithProblemsToKeep[j] = !lastDataSaver->setsWithProblemsToKeep[j]; // Toggle the flag when the button is clicked
                    }
                }
                ImGui::Dummy(ImVec2(0.0f, 20.0f));
            }
            ImGui::Text("If you like this set, if you were really exhausted at the end of the set, you can click on 'Validate the set'");
            ImGui::Text("Else, click on 'Do the set again'");
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::SetCursorPosX((750 - 2 * 120) * 0.5f); // to center the button
            if (OneRManalyzer->indexesOfSetDataSaver.size() >= 3) { ImGui::SetCursorPosX((750 - 140 - 2 * 120) * 0.5f); }
            if (ImGui::Button("Do the set again", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                OneRManalyzer->indexesOfSetDataSaver.pop_back();
            }
            ImGui::SameLine();
            if (ImGui::Button("Validate the set", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                ImGui::CloseCurrentPopup();
                eraseTheProblematicReps(lastDataSaver);
            }
            if (OneRManalyzer->indexesOfSetDataSaver.size() >= 3) {
                ImGui::SameLine();
                if (ImGui::Button(" Validate the set\nEnd the experiment", ImVec2(140, 30))) {
                    ImGui::CloseCurrentPopup();
                    eraseTheProblematicReps(lastDataSaver);
                    WantStopOneRMExperiment = true;
                }
            }
        }
        ImGui::EndPopup();
    }
}

void TrainerPredictor::PrintOutTheSetDataForFutureSimulations() {
    //Only used to manually create .txt files to use the data to simulate a set in the future
    SetDataSaver* dataSaver = dynamic_cast<SetDataSaver*>(seriesAnalyzers[FindLastAnalyzerOfType<SetDataSaver>()].get());
    cout << "SET DATA FOR " << dataSaver->TotalWeightWanted << " kg" << endl;
    cout << " " << endl;
    cout << "Time of the set : " << endl;
    cout << "{";
    for (double time : dataSaver->TimeOfSet) { cout << time << ", "; } cout << "}" << endl;
    cout << " " << endl;
    cout << "Height of the set : " << endl;
    cout << "{";
    for (double height : dataSaver->HeightOfSet[0]) { cout << height << ", "; } cout << "}" << endl;
    cout << " " << endl;
    cout << "Velocity of the set : " << endl;
    cout << "{";
    for (double height : dataSaver->VelocityOfSet[0]) { cout << height << ", "; } cout << "}" << endl;
    cout << " " << endl;
    cout << "Acceleration of the set : " << endl;
    cout << "{";
    for (double height : dataSaver->AccelerationOfSet[0]) { cout << height << ", "; } cout << "}" << endl;
    cout << " " << endl;
}

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
    if (TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>() != -1) {
        OneRMAnalyzer* LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(TheTrainer->predictor.seriesAnalyzers[TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>()].get());

        //Begin Horizontal line at y=OneRMVelocityValue
        std::vector<double> XOneRM = linspace(-5, X.back() + 5, 10 * X.size());
        std::vector<double> YOneRM;
        for (double xOneRM : XOneRM) {
            YOneRM.push_back(LastOneRManalyzer->OneRMVelocityValue);
        }

        string OneRMLabel = "y = " + std::to_string(LastOneRManalyzer->OneRMVelocityValue);
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

        if (regressionCoefficients[0] < 0 && (LastOneRManalyzer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0] > 0){
            //Conjuncture point between the regression line and the horizontal line
            std::string OneRMPointlabel = "(" + std::to_string((LastOneRManalyzer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0]) + " ; " + std::to_string(LastOneRManalyzer->OneRMVelocityValue) + ")";
            std::vector<double> x_values = { (LastOneRManalyzer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0] };
            std::vector<double> y_values = { LastOneRManalyzer->OneRMVelocityValue };
            TheTrainer->app.Plot(index + 1, x_values, y_values, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], OneRMPointlabel, "scatter"); // Plot the point of the conjuncture
            //END Conjuncture point between the regression line and the horizontal line
            return (LastOneRManalyzer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0];
        }
        else {
            TheTrainer->app.DeleteRobotConsoleMessage("Estimated One RM is ");
            TheTrainer->app.AddRobotConsoleError("The regression line is not decreasing so we cannot find the One RM");
            //cout << " ---- ERROR The regression line is not decreasing so we cannot find the One RM ---- " << endl;
            return -1;
        }
    }
}

double TrainerPredictor::FindPersonnalRecord() {
    double personnalRecordAllTime = 0;
    if (TheTrainer->predictor.FindLastAnalyzerOfType<SetDataSaver>() != -1) {
        SetDataSaver* setSaver = dynamic_cast<SetDataSaver*>(TheTrainer->predictor.seriesAnalyzers[TheTrainer->predictor.FindLastAnalyzerOfType<SetDataSaver>()].get());
        std::string personnalRecordInDBString = TheTrainer->dataManager.FindInDataBase(setSaver->Username, setSaver->TypeOfExercice, std::to_string(setSaver->TotalWeightWanted), "MAX", "nbreOfReps");
        vector<double> allNumberOfRepsAtThisLoad = {};
        for (int i = 0; i < seriesAnalyzers.size() - 1; i++) {
            if (seriesAnalyzers[i].get()->name == "SetDataSaver") {
                SetDataSaver* analyzer = dynamic_cast<SetDataSaver*>(seriesAnalyzers[i].get());
                if (analyzer->Username == setSaver->Username && analyzer->TypeOfExercice == setSaver->TypeOfExercice && analyzer->TotalWeightWanted == setSaver->TotalWeightWanted) {
                    allNumberOfRepsAtThisLoad.push_back(analyzer->numberOfReps);
                }
            }
        }
        auto personnalRecordTodayIt = std::max_element(allNumberOfRepsAtThisLoad.begin(), allNumberOfRepsAtThisLoad.end());
        double personnalRecordToday = 0;
        if (personnalRecordTodayIt != allNumberOfRepsAtThisLoad.end()) { // Ensure the iterator is not the end iterator
            personnalRecordToday = *personnalRecordTodayIt;
        }
        double personnalRecordInDB = 0;
        if (personnalRecordInDBString != "NULL") { personnalRecordInDB = std::stod(personnalRecordInDBString); }
        personnalRecordAllTime = max(personnalRecordToday, personnalRecordInDB);
        cout << "Personnal record at " << setSaver->TotalWeightWanted << "kg : " << personnalRecordAllTime << " reps." << std::endl;
    }
    return personnalRecordAllTime;
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

    return { coeffs[0], coeffs[1], coeffs[2] }; //a, b, c from the equation y = ax^2 + bx + c
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
    //subsublabel is the specified text to put in the label. Not necessary to put one when i is specified
    std::vector<double> X = linspace(Xvalues[0], Xvalues[Xvalues.size() - 1], 10 * Xvalues.size());
    if (TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]] == "Max velocity during reps" && TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>() != -1) { //If the regression is for the Max velocity during reps, we extend the X values to the OneRM value so we can see the regression line until the secant point at OneRM value
        OneRMAnalyzer* LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(TheTrainer->predictor.seriesAnalyzers[TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>()].get());
        X = linspace(Xvalues[0], LastOneRManalyzer->OneRMValue, 10 * Xvalues.size());
    }
    std::vector<double> Y;
    std::string type = "";
    std::string equation = "";
    double indexOfSubPlotToPlotOn = 0;

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
                    if (TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>() != -1) {
                        OneRMAnalyzer* LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(TheTrainer->predictor.seriesAnalyzers[TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>()].get());
                        if (OneRM != -1 && OneRM != LastOneRManalyzer->OneRMValue) {
                            TheTrainer->app.DeleteRobotConsoleError(" so we cannot find the One RM ");
                            TheTrainer->app.DeleteRobotConsoleMessage("Estimated One RM is ");
                            TheTrainer->app.AddRobotConsoleMessage("Estimated One RM is " + std::to_string(OneRM) + " kg");
                            LastOneRManalyzer->OneRMValue = OneRM;
                        }
                    }
                }

                if (isPolynomial) {
                    //We tell user that polynomial regression fits better
                    line = "Polynomial regression fits better on " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                    PlotRegression(x, coefficients_pol, "", i, TheTrainer->app.listOfPlotsToKeep[indexesOfDataToDoRegressionOn[i]]);
                    // We update the Endurance Law that should be a polynomial regression...
                    if (TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]] == "Endurance experiment") { 
                        OneRMAnalyzer* LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(TheTrainer->predictor.seriesAnalyzers[TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>()].get());
                        if (coefficients_pol != LastOneRManalyzer->CoefficientsEnduranceLaw) {
                            LastOneRManalyzer->CoefficientsEnduranceLaw = coefficients_pol;
                        }
                    }
                }
                else {
                    //We tell user that linear regression fits better
                    line = "Linear regression fits better on " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                    PlotRegression(x, coefficients_lin, "", i, TheTrainer->app.listOfPlotsToKeep[indexesOfDataToDoRegressionOn[i]]);
                    // ... but if the algorithm finds that a linear regression fits better for the endurance experiment, we update the Endurance Law with this polynomial regression : 0*x*x + a*x + b
                    if (TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]] == "Endurance experiment") {
                        OneRMAnalyzer* LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(TheTrainer->predictor.seriesAnalyzers[TheTrainer->predictor.FindLastAnalyzerOfType<OneRMAnalyzer>()].get());
                        if (LastOneRManalyzer->CoefficientsEnduranceLaw != std::vector<double>{ 0, coefficients_lin[0], coefficients_lin[1] }) {
                            LastOneRManalyzer->CoefficientsEnduranceLaw = std::vector<double>{ 0, coefficients_lin[0], coefficients_lin[1] };
                        }
                    }
                }
                //We add the message to the console
                std::string lineString = line;
                std::string subLine = lineString.substr(11);
                TheTrainer->app.DeleteRobotConsoleMessage(subLine);
                //TheTrainer->app.AddRobotConsoleMessage(line);         //We do not want to add this message to the console because it is not useful for the user
                //cout << line << endl;
            }
            else {
                TheTrainer->app.AddRobotConsoleError("Issue doing regression. Values in y axis are not defined : y[0] = " + std::to_string(y[0]));
            }
        }
    }
}

void TrainerPredictor::BotStartedSet(vector<double> t_list) {
    cout << "START SET" << endl;
    //Create a new SetDataSaver
    int index = CountAnalyzersOfType<SetDataSaver>();
    seriesAnalyzers.push_back(std::make_unique<SetDataSaver>(TheTrainer, index, t_list.back()));
    dynamic_cast<SetDataSaver*>(seriesAnalyzers.back().get())->SetUsedForOneRMExperiment = (DoingOneRMExperiment || WantStartOneRMExperiment); //update the InitialAnalyzer
    dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->numberOfSets++;
    //update the OneRMAnalyzer
    if (FindLastAnalyzerOfType<OneRMAnalyzer>() != -1) {
        OneRMAnalyzer* LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(seriesAnalyzers[FindLastAnalyzerOfType<OneRMAnalyzer>()].get());
        if (!LastOneRManalyzer->closed){ LastOneRManalyzer->indexesOfSetDataSaver.push_back(index); }
    }
    personnalRecordOfNumberOfRepsAtCurrentLoad = FindPersonnalRecord();
    TheTrainer->app.DeleteRobotConsoleWarning("The rep number ");
}

void TrainerPredictor::BotFinishedSet(vector<double> t_list) {
    cout << "STOP SET" << endl;
    AnalyzeLastSet(t_list);
    //PrintOutTheSetDataForFutureSimulations();
    dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->update(true);
    for (auto& analyzer : seriesAnalyzers) {
        if (!analyzer.get()->closed) {
            analyzer.get()->numberOfSets++;
            analyzer.get()->lastSetEndTime = t_list.back();
        }
    }
}

void TrainerPredictor::AnalyzeLastSet(vector<double> t_list) {
    int indexLastSetDataSaverAnalyzer = FindLastAnalyzerOfType<SetDataSaver>();
    if (indexLastSetDataSaverAnalyzer != -1) {
        SetDataSaver* setAnalyzer = dynamic_cast<SetDataSaver*>(seriesAnalyzers[indexLastSetDataSaverAnalyzer].get());
        if (!setAnalyzer->closed){
            auto maxHeightElementIt = std::max_element(setAnalyzer->HeightOfSet[0].begin(), setAnalyzer->HeightOfSet[0].end());
            double maxHeightOfSet = 0;
            if (maxHeightElementIt != setAnalyzer->HeightOfSet[0].end()) { // Ensure the iterator is not the end iterator
                maxHeightOfSet = *maxHeightElementIt; // Dereference the iterator to get the value
                double peeksValue = maxHeightOfSet - 0.2 * std::abs(maxHeightOfSet);
                TheTrainer->app.addHorizontalLineToPlot(peeksValue);
                std::vector<std::vector<double>> result = findPeaksBoundaries(setAnalyzer->HeightOfSet[0], peeksValue);
                if (result[0].size() > 0) { //We have done more than one rep we can proceed analyzing
                    std::vector<double> indexesOfBeginPeak = result[0];
                    std::vector<double> indexesOfEndPeak = result[1];
                    std::vector<double> indexesOfEndCounterReps = result[0];
                    std::vector<double> indexesOfBeginCounterReps = result[1];
                    if (indexesOfEndCounterReps.size()>1) {
                        indexesOfEndCounterReps.erase(indexesOfEndCounterReps.begin());
                    }
                    if (indexesOfEndCounterReps.back() <= indexesOfBeginCounterReps.back()) {
                        indexesOfBeginCounterReps.pop_back();
                    }
                    std::vector<double> minHeightsOfCounterReps = std::vector<double>();
                    for (int i = 0; i < indexesOfBeginCounterReps.size(); i++) {
                        std::vector<double> heightsOfCounterReps = std::vector<double>(setAnalyzer->HeightOfSet[0].begin() + indexesOfBeginCounterReps[i], setAnalyzer->HeightOfSet[0].begin() + indexesOfEndCounterReps[i]);
                        minHeightsOfCounterReps.push_back(*std::min_element(heightsOfCounterReps.begin(), heightsOfCounterReps.end()));
                    }
                    //double meanMinHeightsOfCounterReps = std::accumulate(minHeightsOfCounterReps.begin(), minHeightsOfCounterReps.end(), 0.0) / minHeightsOfCounterReps.size();
                    auto maxMinHeightsOfCounterRepsElementIt = std::max_element(minHeightsOfCounterReps.begin(), minHeightsOfCounterReps.end());
                    double maxMinHeightsOfCounterReps = 0;
                    if (maxMinHeightsOfCounterRepsElementIt != minHeightsOfCounterReps.end()) { // Ensure the iterator is not the end iterator
                        maxMinHeightsOfCounterReps = *maxMinHeightsOfCounterRepsElementIt; // Dereference the iterator to get the value
                    }
                    peeksValue = maxMinHeightsOfCounterReps + 0.1 * std::abs(maxHeightOfSet);
                    TheTrainer->app.addHorizontalLineToPlot(peeksValue);
                    result = findPeaksBoundaries(setAnalyzer->HeightOfSet[0], peeksValue);
                    if (result[0].size() != 0 && result[1].size() != 0){
                        setAnalyzer->indexesEnteringARep = result[0];
                        setAnalyzer->indexesExitingARep = result[1];
                        for (int i = 0; i < setAnalyzer->indexesEnteringARep.size(); i++) {
                            //TheTrainer->app.addPointToPlot({ t_list[setAnalyzer->indexesEnteringARep[i]], 0 }, "");

                            std::vector<double> heightOfRep;
                            std::vector<double> velocityOfRep;
                            std::vector<double> accelerationOfRep;
                            for (int j = setAnalyzer->indexesEnteringARep[i]; j < setAnalyzer->indexesExitingARep[i]; j++) {
                                heightOfRep.push_back(setAnalyzer->HeightOfSet[0][j]);
                                velocityOfRep.push_back(setAnalyzer->VelocityOfSet[0][j]);
                                accelerationOfRep.push_back(setAnalyzer->AccelerationOfSet[0][j]);
                            }
                            if (heightOfRep.size() != 0 && velocityOfRep.size() != 0 && accelerationOfRep.size() != 0) {

                                //Get the max Height of the rep
                                auto maxHeightOfRepElementIt = std::max_element(heightOfRep.begin(), heightOfRep.end());
                                double maxHeightOfRep = 0;
                                if (maxHeightOfRepElementIt != heightOfRep.end()) { // Ensure the iterator is not the end iterator
                                    maxHeightOfRep = *maxHeightOfRepElementIt; // Dereference the iterator to get the value
                                }
                                setAnalyzer->maxHeights.push_back(maxHeightOfRep);
                                if (maxHeightOfRep < maxHeightOfSet - 0.3 * std::abs(maxHeightOfSet)) {
                                    TheTrainer->app.AddRobotConsoleWarning("The rep number " + std::to_string(i + 1) + " is not going high enough");
                                    setAnalyzer->problemsInSet.push_back("The rep number " + std::to_string(i + 1) + " is not going high enough");
                                    setAnalyzer->IndexesOfRepsWithProblems.push_back(i);
                                }
                                //Get the max Velocity of the rep
                                int maxVelocityOfRepIndex = std::distance(velocityOfRep.begin(), std::max_element(velocityOfRep.begin(), velocityOfRep.end()));
                                double maxVelocityOfRep = velocityOfRep[maxVelocityOfRepIndex];
                                setAnalyzer->maxVelocities.push_back(maxVelocityOfRep);
                                TheTrainer->app.addPointToPlot({ setAnalyzer->TimeOfSet[setAnalyzer->indexesEnteringARep[i] + maxVelocityOfRepIndex], maxVelocityOfRep }, "maxVelocity");
                                //Get the max acceleration of the rep
                                auto maxAccelerationIt = std::max_element(accelerationOfRep.begin(), accelerationOfRep.end());
                                double maxAccelerationOfRep = 0;
                                if (maxAccelerationIt != accelerationOfRep.end()) { // Ensure the iterator is not the end iterator
                                    maxAccelerationOfRep = *maxAccelerationIt; // Dereference the iterator to get the value
                                }
                                result = findPeaksBoundaries(accelerationOfRep, 0 + 0.3 * maxAccelerationOfRep);
                                if (result[0].size() > 1) {
                                    TheTrainer->app.AddRobotConsoleWarning("The rep number " + std::to_string(i + 1) + " is not smooth enough");
                                    setAnalyzer->problemsInSet.push_back("The rep number " + std::to_string(i + 1) + " is not smooth enough");
                                    setAnalyzer->IndexesOfRepsWithProblems.push_back(i);
                                }
                            }
                            else {
                                cout << "ERROR while analyzing the last set : the heightsOfRep or the velocityOfReps or the accelerationOfRep is an empty list for rep number " << i+1 << endl;
                            }
                        }
                        if (setAnalyzer->maxVelocities.size() != 0) {
                            dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->minMaxVelocitiesOfSets.push_back(*std::min_element(setAnalyzer->maxVelocities.begin(), setAnalyzer->maxVelocities.end()));
                        }
                        setAnalyzer->closeAnalyzer(t_list.back());
                    }
                    else {
                        cout << "ERROR while analyzing the last set : no peak has been found" << endl;
                    }
                }
                else {
                    cout << " ERROR while analyzing the last set : Only one rep has been done. We cannot analyze it" << endl;
                }
            }
            else {
                cout << "ERROR while analyzing the last set : There is no height maximum. Either the set has not been done or the SetAnalyzer has not been updated through the set" << endl;
            }
        }
        else {
            cout << "ERROR while analyzing the last set : Last analyzer has been closed before we could analyze the set" << endl;
        }
    }
    else {
        cout << "No Set Analyzer has been found" << endl;
    }

}

void TrainerPredictor::ContinuousRepCounter(std::vector<double> t_list, std::vector<double> distance_list, std::vector<double>velocity_list) {
    //The continuous analysis is not perfectly reliable but it allows to use the endurance law
    double epsilon = 0.005;
    OneRMAnalyzer* LastOneRManalyzer;
    int indexLastOneRMAnalyzer = FindLastAnalyzerOfType<OneRMAnalyzer>();
    if (indexLastOneRMAnalyzer != -1) {
		LastOneRManalyzer = dynamic_cast<OneRMAnalyzer*>(seriesAnalyzers[indexLastOneRMAnalyzer].get());
    }
    SetDataSaver* setAnalyzer;
    int indexLastSetDataSaverAnalyzer = FindLastAnalyzerOfType<SetDataSaver>();
    if (indexLastSetDataSaverAnalyzer != -1) {
        if (!seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->closed) {
            setAnalyzer = dynamic_cast<SetDataSaver*>(seriesAnalyzers[indexLastSetDataSaverAnalyzer].get());
        }
    }
    if (indexLastSetDataSaverAnalyzer != -1) {
        if (velocity_list.back() > 0 + epsilon && velocity_list[velocity_list.size() - 2] <= 0 + epsilon) { //We just started a lobe
            setAnalyzer->index0_RisingPhaseOfCurrentRep = t_list.size() - 1;
            setAnalyzer->continuousMinHeights.push_back(distance_list.back()); // We are at min Height of the rep
            setAnalyzer->continuousNumberOfReps++;
        }
        if (velocity_list.back() < 0 + epsilon && velocity_list[velocity_list.size() - 2] >= 0 + epsilon) { //We just finished a lobe, we are at max height
            setAnalyzer->continuouseTimeOfTopRep.push_back(t_list.back());
            TheTrainer->app.addPointToPlot({ t_list.back(), 0 }, "maxHeight");
            setAnalyzer->continuousMaxHeights.push_back(distance_list.back());
            setAnalyzer->indexf_RisingPhaseOfCurrentRep = t_list.size() - 1;

            std::vector<double> TimeLobe = std::vector<double>(t_list.begin() + setAnalyzer->index0_RisingPhaseOfCurrentRep, t_list.begin() + setAnalyzer->indexf_RisingPhaseOfCurrentRep);
            std::vector<double> VelocityLobe = std::vector<double>(velocity_list.begin() + setAnalyzer->index0_RisingPhaseOfCurrentRep, velocity_list.begin() + setAnalyzer->indexf_RisingPhaseOfCurrentRep);

            double maxVelocityLobe = *std::max_element(VelocityLobe.begin(), VelocityLobe.end());
            double timeAtMaxVelocity = TimeLobe[std::distance(VelocityLobe.begin(), std::max_element(VelocityLobe.begin(), VelocityLobe.end()))];
            setAnalyzer->continuousMaxVelocities.push_back(maxVelocityLobe);
            double MaxContinuousMaxVelocities = *std::max_element(setAnalyzer->continuousMaxVelocities.begin(), setAnalyzer->continuousMaxVelocities.end());
            double Velocity_loss = 100 * (MaxContinuousMaxVelocities - maxVelocityLobe) / MaxContinuousMaxVelocities;
            // Remove values from both vectors based on the condition
            if (indexLastOneRMAnalyzer != -1) {
                if (LastOneRManalyzer->closed){
                    PercentageOfSet = LastOneRManalyzer->GetNumberOfRepAtSpecificVloss(Velocity_loss); //According to former student Natsuo Tojo, "PercentageOfSet" is written "Nper" in his work and is the proportion of the number of reps that we usually do at the same velocity loss (Endurance experiment must have been done)
                    MaxNumberRepsEstimated = setAnalyzer->continuousNumberOfReps / (PercentageOfSet * 1/100); //According to former student Natsuo Tojo, Endurance is the proportion of the number of reps that we already did. This means we can predict the number of reps that we have done
                    CurrentNumberOfRepsDoneInTheSet = setAnalyzer->continuousNumberOfReps;
                    TheTrainer->app.DeleteRobotConsoleMessage(", maximal number of repetitions estimated is ");
                    TheTrainer->app.AddRobotConsoleMessage("At rep " + std::to_string(setAnalyzer->continuousNumberOfReps) + ", maximal number of repetitions estimated is " + std::to_string(MaxNumberRepsEstimated) + ". At Nper = " + std::to_string(PercentageOfSet) + ", V_loss = " + std::to_string(Velocity_loss));
                }
            }
            double meanMaxHeight = std::accumulate(setAnalyzer->continuousMaxHeights.begin(), setAnalyzer->continuousMaxHeights.end(), 0.0) / setAnalyzer->continuousMaxHeights.size();
            double meanMinHeight = std::accumulate(setAnalyzer->continuousMinHeights.begin(), setAnalyzer->continuousMinHeights.end(), 0.0) / setAnalyzer->continuousMinHeights.size();
            int j = 0;
            for (size_t i = 0; i < setAnalyzer->continuousMaxHeights.size(); ) {
                if (setAnalyzer->continuousMaxHeights[i] < 0.5 * meanMaxHeight) { // We delete the rep because we misthought it wa a rep. 
                    TheTrainer->app.DeleteRobotConsoleMessage("At rep " + std::to_string(j + 1) + ", maximal number of repetitions estimated is ");
                    //TheTrainer->app.deletePointToPlot({ setAnalyzer->continuouseTimeOfTopRep[i], 0 });
                    TheTrainer->app.changePointToPlotColor({ setAnalyzer->continuouseTimeOfTopRep[i], 0 }, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
                    setAnalyzer->continuousMaxHeights.erase(setAnalyzer->continuousMaxHeights.begin() + i);
                    setAnalyzer->continuousMinHeights.erase(setAnalyzer->continuousMinHeights.begin() + i);
                    setAnalyzer->continuousMaxVelocities.erase(setAnalyzer->continuousMaxVelocities.begin() + i);
                    setAnalyzer->continuouseTimeOfTopRep.erase(setAnalyzer->continuouseTimeOfTopRep.begin() + i);
                    setAnalyzer->continuousNumberOfReps--;
                }
                else {
                    ++i;
                }
                j++;
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

void TrainerPredictor::OneRMExperiment(vector<double> t_list) {
    if (WantStartOneRMExperiment) {
        cout << "START ONE RM EXPERIMENT" << endl;
        int index = CountAnalyzersOfType<OneRMAnalyzer>();
        seriesAnalyzers.push_back(std::make_unique<OneRMAnalyzer>(TheTrainer, index, t_list.back()));
        WantStartOneRMExperiment = false;
        DoingOneRMExperiment = true;
    }
    if (DoingOneRMExperiment) {
        for (auto& analyzer : seriesAnalyzers) {
            if (!analyzer.get()->closed && analyzer.get()->name == "OneRMAnalyzer"){
                analyzer.get()->DoMission();
                if (std::abs(analyzer.get()->lastSetEndTime - t_list.back()) <= TheTrainer->AquisitionFrequency) {
                    ImGui::OpenPopup("One RM by 3 steps method (intermediate window)");
                    analyzer.get()->lastSetEndTime = -1; // C'est pas tres propre mais bon...
				}
                definePopupWindowOneRM3StepsIntermediateWindow();
            }
        }
    }
    if (WantStopOneRMExperiment) {
        cout << "STOP ONE RM EXPERIMENT" << endl;
        for (auto& analyzer : seriesAnalyzers) {
            if (!analyzer.get()->closed && analyzer.get()->name == "OneRMAnalyzer") {
                analyzer.get()->DoMission();
                analyzer.get()->closeAnalyzer(t_list.back());
            }
        }
        DoingOneRMExperiment = false;
        WantStopOneRMExperiment = false;
    }
}

void TrainerPredictor::AnalyzeData() {
    std::vector<double> t_list = TheTrainer->app.listXToPlot[36-1];
    std::vector<double> distance_list = TheTrainer->app.listYToPlot[36-1];
    std::vector<double> velocity_list = TheTrainer->app.listYToPlot[37 - 1];
    std::vector<double> acceleration_list = TheTrainer->app.listYToPlot[38 - 1];

    // CONTINUOUS ANALYSIS
    if (lastTimeWeAnalyzedData != t_list.back()) { //We analyze only if there is new data
        //Update the DataSavers data
        int indexLastSetDataSaverAnalyzer = FindLastAnalyzerOfType<SetDataSaver>();
        if (indexLastSetDataSaverAnalyzer != -1) {
            if (!seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->closed) {
                seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->DoMission();
            }
        }
        ContinuousRepCounter(t_list, distance_list, velocity_list);
        lastTimeWeAnalyzedData = t_list.back();
    }
    // END CONTINUOUS ANALYSIS

    LepleyExperiment(t_list);
    OneRMExperiment(t_list);

    //Get if we finished the set without turning off the motors (change of exercice type, change of username, change of load, etc)
    int indexLastSetDataSaverAnalyzer = FindLastAnalyzerOfType<SetDataSaver>();
    if (indexLastSetDataSaverAnalyzer != -1) {
        if (TheTrainer->BotWorkingInProgress && (TheTrainer->Username != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->Username || TheTrainer->TypeOfExercice != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->TypeOfExercice || TheTrainer->botManager.TotalWeightWanted != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->TotalWeightWanted || TheTrainer->UsingLoadProfile != seriesAnalyzers[indexLastSetDataSaverAnalyzer].get()->UsingLoadProfile)) {
            //We finished a set without turning the motors off!!
            BotFinishedSet(t_list);
            BotStartedSet(t_list);
        }
    }

    //Update the initial analyzer
    dynamic_cast<InitialAnalyzer*>(seriesAnalyzers[0].get())->update();

    predictRegression(); // Will predict the regression for the data that has been added to the list of data to do regression on
}
