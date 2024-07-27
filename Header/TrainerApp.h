#pragma once

#define GLEW_STATIC
#include <image_stb/GL/glew.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include "imgui.h"
#include "Trapezoid.h"
#include "Point.h"
#include "Line.h"
#include <chrono>

class Trainer;


class TrainerApp {
public:

    // Variables for the data plot 
    std::vector<Line> listLinesToPlot; // List of lines to plot. TODO create a Line class to put inside. Yet it only plot horizontal lines
    std::vector<Point> listPointsToPlot ; // List of points to plot
    std::vector<bool> listOfPlotsToKeep; // List of plots that we should never forget
    std::vector<std::vector<double>> listXToPlot; // List of Xs values to plot
    std::vector<std::vector<double>> listYToPlot; // List of Ys values to plot
    std::vector<double> listIndexesToPlot; // List of indexes values to plot. 0 for the first row first column, 1.0 for the second row first column, 1.1 for the second row second column, 2 for the third row.
    std::vector<std::string> listLabelsToPlot; // List of Labels values to plot
    std::vector<std::string> listStylesToPlot; // List of Styles values to plot
    std::vector<ImVec4> listColorsToPlot; // List of Styles values to plot
    std::vector<std::string> listAnnotationsToPlot; // List of Annotations values to plot
    std::vector<std::string> listMessagesInConsole; // List of messages to show in console
    std::vector<std::string> listWarningsInConsole; // List of messages to show in console
    std::vector<std::string> listErrorsInConsole; // List of error messages to show in console
    int nbreOfDataToPlot = 60;
    bool showLegends = true; // Default to true to show legends initially
    bool ShowAnnotation = false;
    std::vector<double> subplotHeights = { 300, 300, 300 };

    // Variables for the profile plot
    bool WantToPlotProfile = true; // Default to true to show the profile plot initially
    int IndexOfVertexFocusedOn = -1;
    std::vector<double> mousePositionAtRightClick;

    //Variables for Epley experiment 
    int NumberOfReps = 0;
    int LoadDuringReps = 0;

    //Variables for Analyzing repetitions
    double minBound = 0;
    double maxBound = 1000000;
    bool HasAlreadyClickedOnPlot = false;

    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    bool ShowFirstPopup = true;

    Trapezoid trapeze;
    Trainer* TheTrainer;
    TrainerApp(Trainer* trainer);

    //void Begin();
    void ShowMessagesAndErrorInConsole();
    void AddRobotConsoleInfo(const std::string& message, std::string type);
    void AddRobotConsoleMessage(const std::string& message);
    void AddRobotConsoleWarning(const std::string& message);
    void AddRobotConsoleError(const std::string& message);
    void DeleteRobotConsoleInfo(const std::string& message, std::string type);
    void DeleteRobotConsoleMessage(const std::string& message);
    void DeleteRobotConsoleWarning(const std::string& message);
    void DeleteRobotConsoleError(const std::string& message);
    void DeleteOldestConsoleInfo();
    void DefineFirstPopup();
    void RenderUI();
    void MakeDockingPossible();
    void CreateUsersInfoWindow();
    void CreateBotModeWindow();
    void CreatePlotWindow();
    void CreateConsoleWindow();
    void UndoLastAction();
    void SubplotItemSharing();
    int FindPointFocusedOn(int j, std::vector <double> mousePosition);
    void RenderPointsAndLineInPlot(double i);
    void RenderDataInPlot(double i);
    void SetUpQueryInPlot();
    void Plot(int i, std::vector<double>& x, std::vector<double>& y, double index = 0, std::string label = "", std::string style = "", std::string annotation = "", bool shallWeKeepThePlotEvenAfterAClearPlot = true, ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    void Unplot(std::string label);
    void ClearPlot();
    int findIndexOfEmptyPlot();
    void defineLastPopup();
    void definePopupWindowOneRMEpley();
    void definePopupWindowOneRMEpleyExplanation();
    void definePopupWindowOneRM3StepsExplanation();
    void definePopupWindowOneRM3StepsIntermediateWindow();
    void DrawResizableSeparator(int i);
    void addPointToPlot(std::vector<double> coordinates, std::string label, ImVec4 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    void deletePointToPlot(std::vector<double> coordinates);
    void changePointToPlotColor(std::vector<double> coordinates, ImVec4 color);
    void addVerticalLineToPlot(double value, std::string type = "", int index = 0);
    void addHorizontalLineToPlot(double value, std::string type = "", int index = 0);
    std::vector<std::tuple<std::string, std::string>> MergeAndSortMessages(); // Merge and sort infos from messages, warnings and errors
};
