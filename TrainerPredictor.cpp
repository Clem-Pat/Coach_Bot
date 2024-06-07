
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
    std::vector<double> x_values = { (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0] };
    std::vector<double> y_values = { TheTrainer->OneRMVelocityValue };
    TheTrainer->app.Plot(index + 1, x_values, y_values, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], "..", "scatter");
    return (TheTrainer->OneRMVelocityValue - regressionCoefficients[1]) / regressionCoefficients[0];
}

//Calculating F test as defined in the article https://delladata.fr/regression-polynomiale/
//Using Weierstrass theorem principle
bool TrainerPredictor::calculateFTest(std::vector<double>& x, std::vector<double>& y, vector<double>& linearCoefficients, vector<double>& polynomialCoefficients) {

    //Computing RSS1
    int n = x.size();
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
        char line[100];
        snprintf(line, sizeof(line), "F is not finite or is negative %.3f so we choose linear regression", F);
        std::string lineString = line;
        std::string subLine = lineString.substr(0, 15);
        TheTrainer->app.DeleteRobotConsoleError(subLine);
        TheTrainer->app.AddRobotConsoleError(line);
        return false;
    }
    else {
        boost::math::fisher_f f_dist(3 - 2, n - 3);
        double p_value = 1 - boost::math::cdf(f_dist, F);
        return { p_value < 0.05 };
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

    return { a, b};
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

void TrainerPredictor::predictRegression() {
    for (int i = 0; i < indexesOfAbscissaData.size(); i++)
    {
        TheTrainer->app.AddRobotConsoleMessage("Doing Regression on " + TheTrainer->app.listLabelsToPlot[indexesOfAbscissaData[i]]);
        //std::vector<double> x = TheTrainer->dataManager.experimentData[indexesOfAbscissaData[i]];
        //std::vector<double> y = TheTrainer->dataManager.experimentData[indexesOfDataToDoRegressionOn[i]];

        std::vector<double> x = TheTrainer->app.listXToPlot[indexesOfAbscissaData[i]];
        std::vector<double> y = TheTrainer->app.listYToPlot[indexesOfDataToDoRegressionOn[i]];

        if (y.size() > 3) {
            std::vector<double> coefficients_pol = polynomialRegression(x, y);
            std::vector<double> coefficients_lin = linearRegression(x, y);

            std::vector<double> X = linspace(x[0], x[x.size() - 1], 10*x.size());
            std::vector<double> Y;
            std::vector<double> Y2;
            bool isPolynomial = calculateFTest(x, y, coefficients_lin, coefficients_pol);

            if (TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]] == "Velocity during reps") {
                while (coefficients_lin[0] * X.back() + coefficients_lin[1] > 0 && X.size() < 1000) {
                    X.push_back(X.back() + 1); // Increment the last value in X by 1kg
                }
                double OneRM = FindOneRM(X, coefficients_lin, i);
                TheTrainer->app.AddRobotConsoleMessage("Estimated One RM is " + std::to_string(OneRM) + " kg");
            }

            if (isPolynomial) {
                //We tell user that polynomial regression fits better
                std::string line = "Polynomial regression fits better on " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                std::string lineString = line;
                std::string subLine = lineString.substr(11);
                TheTrainer->app.DeleteRobotConsoleMessage(subLine);
                TheTrainer->app.AddRobotConsoleMessage(line);
                //Just to make annotation on the graph with the equation
                for (double xi : X) {
                    double yi2 = coefficients_pol[0] * xi * xi + coefficients_pol[1] * xi + coefficients_pol[2];
                    Y2.push_back(yi2);
                }
                std::string equation = "y = " + std::to_string(coefficients_pol[0]) + "x^2 " +
                    (coefficients_pol[1] < 0 ? "- " : "+ ") + std::to_string(abs(coefficients_pol[1])) + "x " +
                    (coefficients_pol[2] < 0 ? "- " : "+ ") + std::to_string(abs(coefficients_pol[2]));

                //Find the proper index to register the data to plot
                std::string sublabel = "regression " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
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
                std::string label = "Polynomial regression " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                TheTrainer->app.Plot(index, X, Y2, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], label, "lines", equation);
            }
            else {
                //We tell user that linear regression fits better
                std::string line = "Linear regression fits better on " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                std::string lineString = line;
                std::string subLine = lineString.substr(11);
                TheTrainer->app.DeleteRobotConsoleMessage(subLine);
                TheTrainer->app.AddRobotConsoleMessage(line);
                //Just to make annotation on the graph with the equation
                for (double xi : X) {
                    double yi = coefficients_lin[0] * xi + coefficients_lin[1];
                    Y.push_back(yi);
                }
                std::string equation = "y = " + std::to_string(coefficients_lin[0]) + "x" +
                    (coefficients_lin[1] < 0 ? "- " : "+ ") + std::to_string(abs(coefficients_lin[1]));

                //Find the proper index to register the data to plot
                std::string sublabel = "regression " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
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

                std::string label = "Linear regression " + TheTrainer->app.listLabelsToPlot[indexesOfDataToDoRegressionOn[i]];
                TheTrainer->app.Plot(index, X, Y, TheTrainer->app.listIndexesToPlot[indexesOfDataToDoRegressionOn[i]], label, "lines", equation);
            }
        }
    }
}
