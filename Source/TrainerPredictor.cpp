
#include "TrainerPredictor.h"
#include "Trainer.h"

#include <iostream>
#include <vector>
#include <cmath>
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

    //Horizontal line at y=OneRMVelocityValue
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
    
    //Conjuncture point between the regression line and the horizontal line
    std::string OneRMPointlabel = "(" + std::to_string((TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0]) + " ; " + std::to_string(TheTrainer->OneRMVelocityValue) + ")";
    std::vector<double> x_values = { (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0] };
    std::vector<double> y_values = { TheTrainer->OneRMVelocityValue };
    TheTrainer->app.Plot(index + 1, x_values, y_values, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], OneRMPointlabel, "scatter");
    //END Conjuncture point between the regression line and the horizontal line

    return (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0];
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
            cout << "F is not finite or is negative " << std::to_string(F) << " so we choose linear regression" << endl;
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
    std::string label = type + " regression " + subsublabel;
    TheTrainer->app.Plot(index, X, Y, indexOfSubPlotToPlotOn, label, "lines", equation, shallWeKeepThePlotEvenAfterAClearPlot);
}

void TrainerPredictor::predictRegression() {
    for (int i = 0; i < indexesOfAbscissaData.size(); i++)
    {

        std::vector<double> x = TheTrainer->app.listXToPlot[indexesOfAbscissaData[i]];
        std::vector<double> y = TheTrainer->app.listYToPlot[indexesOfDataToDoRegressionOn[i]];

        if (y.size() > 2) {
            if (std::to_string(y[0]) == "-nan(ind)")
            {
                TheTrainer->app.AddRobotConsoleError("Issue doing regression. Values in y axis are not defined : y[0] = " + std::to_string(y[0]));
            }
            else{
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
                    TheTrainer->app.DeleteRobotConsoleMessage("Estimated One RM is ");
                    TheTrainer->app.AddRobotConsoleMessage("Estimated One RM is " + std::to_string(OneRM) + " kg");
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
                TheTrainer->app.AddRobotConsoleMessage(line);
            }
        }
    }
}

void TrainerPredictor::AnalyzeData() {

    if (WantToAnalyzeData) {
        std::vector<double> t_list = TheTrainer->app.listXToPlot[36-1];
        std::vector<double> distance_list = TheTrainer->app.listYToPlot[36-1];
        std::vector<double> velocity_list = TheTrainer->app.listYToPlot[37-1];
        if (t_list == std::vector<double>{0.0, 0.0, 0.0} || distance_list == std::vector<double>{0.0, 0.0, 0.0} || velocity_list == std::vector<double>{0.0, 0.0, 0.0} || t_list.size() == 0 || distance_list.size() == 0 || velocity_list.size() == 0) {
            cout << "Error : the lists you want to analyze are empty (or equal to {0, 0, 0})" << endl;
        }
        else {
            cout << "Velocity list " <<  velocity_list[0] << " " << velocity_list.back() << " " << velocity_list.size() << endl;
            //Get the boundaries and the portion of the lists we are studying
            double minBound = TheTrainer->app.minBound;
            double maxBound = TheTrainer->app.maxBound;
            auto indexMin = FindIndexOfListAfterOrBeforeSpecifiedValue(t_list, minBound);
            auto indexMax = FindIndexOfListAfterOrBeforeSpecifiedValue(t_list, maxBound);
            //Make sure indexMax and indexMin are credible indexes 
            indexMax = ((indexMax) < (static_cast<decltype(indexMax)>(t_list.size()))) ? (indexMax) : (static_cast<decltype(indexMax)>(t_list.size()));
            indexMin = ((indexMin) < (static_cast<decltype(indexMin)>(t_list.size()))) ? (indexMin) : (static_cast<decltype(indexMin)>(t_list.size()));
            cout << "indexMin " << indexMin << " indexMax " << indexMax << endl;
            std::vector<double> XSubList;
            std::vector<double> DistanceSubList;
            std::vector<double> VelocitySubList;
            if (static_cast<size_t>(indexMin) < static_cast<size_t>(indexMax) && static_cast<size_t>(indexMin) < t_list.size() && static_cast<size_t>(indexMax) <= t_list.size()) {
                XSubList = std::vector<double>(t_list.begin() + indexMin, t_list.begin() + indexMax);
                DistanceSubList = std::vector<double>(distance_list.begin() + indexMin, distance_list.begin() + indexMax); //Distance in the interval
                VelocitySubList = std::vector<double>(velocity_list.begin() + indexMin, velocity_list.begin() + indexMax); //Velocity in the interval
            
                cout << "VelocitySubList " << VelocitySubList[0] << " " << VelocitySubList.back() << " " << VelocitySubList.size() << endl;
                //Understand where the lobe is in velocityData (It means where is the rising up of the load)
                int startLobe = 0;
                int finishLobe = VelocitySubList.size();
                for (int i = 1; i < VelocitySubList.size(); i++) {
                    if (VelocitySubList[i] > 0 && VelocitySubList[i - 1] <= 0) { //We just started a lobe
                        cout << "We found the start point of the lobe at " << i << " " << VelocitySubList[i - 1] << " " << VelocitySubList[i] << endl;
                        startLobe = i;
                    }
                    if (VelocitySubList[i] < 0 && VelocitySubList[i - 1] >= 0) { //We just finished a lobe
                        cout << "We found the end point of the lobe at " << i << " " << VelocitySubList[i - 1] << " " << VelocitySubList[i] << endl;
                        finishLobe = i;
                    }
                }
                std::vector<double> TimeLobe = std::vector<double>(XSubList.begin() + startLobe, XSubList.begin() + finishLobe);
                std::vector<double> VelocityLobe = std::vector<double>(VelocitySubList.begin() + startLobe, VelocitySubList.begin() + finishLobe);
                std::vector<double> DistanceLobe = std::vector<double>(DistanceSubList.begin() + startLobe, DistanceSubList.begin() + finishLobe);

                //Do polynomial regression on the lobe
                cout << "TimeLobe " << TimeLobe[0] << " " << TimeLobe.back() << " " << TimeLobe.size() << endl;
                cout << "VelocityLobe " << VelocityLobe[0] << " " << VelocityLobe.back() << " " << VelocityLobe.size() << endl;
                std::vector<double> coefficients_pol = polynomialRegression(TimeLobe, VelocityLobe);

                //find the max of the polynomial regression
                double maxVelocity = (-coefficients_pol[1] * coefficients_pol[1] / (4 * coefficients_pol[0])) + coefficients_pol[2]; //max value of the polynomial regression
                double timeAtMaxVelocity = -coefficients_pol[1] / (2 * coefficients_pol[0]); //time at max value of the polynomial regression

                cout << "Max velocity is " << maxVelocity << " at time " << timeAtMaxVelocity << endl;
                TheTrainer->app.AddRobotConsoleMessage("Max velocity of this repetition is " + std::to_string(maxVelocity) + " at time " + std::to_string(timeAtMaxVelocity));

                std::vector<double> pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                pointCoordinates = { timeAtMaxVelocity, maxVelocity };
                TheTrainer->app.addPointToPlot(pointCoordinates, "maxVelocity");

                //Plot polynomial regression
                PlotRegression(TimeLobe, coefficients_pol, "on lobe", false);

                TheTrainer->OneRMExperimentLoads.push_back(TheTrainer->botManager.TotalWeightWanted);
                TheTrainer->OneRMExperimentVelocities.push_back(maxVelocity);
                TheTrainer->app.Plot(50, TheTrainer->OneRMExperimentLoads, TheTrainer->OneRMExperimentVelocities, 1, "Max velocity during reps", "scatter");
            }
            else {
                TheTrainer->app.AddRobotConsoleError("Error in AnalyzeData: minBound or maxBound are not in the list of X values");
                cout << "Error in AnalyzeData: minBound or maxBound are not in the list of X values" << endl;
                cout << "Are you sure that t_list is actually the abscissa list of the y list that you want to analyze ?" << endl;
            }
        }
        //collect it and make an average on all the MAXes
        WantToAnalyzeData = false;
    }
    predictRegression(); // Will predict the regression for the data that has been added to the list of data to do regression on
}