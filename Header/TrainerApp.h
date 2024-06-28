#pragma once

#define GLEW_STATIC
#include <image_stb/GL/glew.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include "imgui.h"
#include "Trapezoid.h"
#include "Point.h"

class Trainer;


class TrainerApp {
public:

    int TypeOfExerciceOptionSelected;

    // Variables for the data plot 
    std::vector<Point> listPointsToPlot ; // List of points to plot
    std::vector<bool> listOfPlotsToKeep; // List of plots that we should never forget
    std::vector<std::vector<double>> listXToPlot; // List of Xs values to plot
    std::vector<std::vector<double>> listYToPlot; // List of Ys values to plot
    std::vector<int> listIndexesToPlot; // List of indexes values to plot
    std::vector<std::string> listLabelsToPlot; // List of Labels values to plot
    std::vector<std::string> listStylesToPlot; // List of Styles values to plot
    std::vector<ImVec4> listColorsToPlot; // List of Styles values to plot
    std::vector<std::string> listAnnotationsToPlot; // List of Annotations values to plot
    std::vector<std::string> listMessagesInConsole; // List of messages to show in console
    std::vector<std::string> listErrorsInConsole; // List of error messages to show in console
    int indexOfWindowToPlotRawData = 1;
    int nbreOfDataToPlot = 60;
    bool ShowAnnotation = true;

    // Variables for the profile plot
    bool WantToPlotProfile = true;
    int IndexOfVertexFocusedOn = -1;
    std::vector<double> mousePositionAtRightClick;

    // Variables for the pop up window 
    int PopUpWindowWidth = 500;
    int PopUpWindowHeight = 500;
    int channels = 500;
    GLuint texture;
    unsigned char* data;
    D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle;

    // Variables for the study of the Kalman filter
    std::vector<double> x; // 1000 valeurs entre 0 et 100
    std::vector<double> ySmoothed;
    std::vector<double> ySmoothedKalman;
    std::vector<double> ySmoothedKalman2;
    std::vector<double> y;
    std::vector<double> y_withoutNoise;
    double yToSmooth;

    //Variables for Epley experiment 
    int NumberOfReps = 0;
    int LoadDuringReps = 0;

    //Variables for Analyzing repetitions
    double minBound = 0;
    double maxBound = 1000000;
    bool HasAlreadyClickedOnPlot = false;

    Trapezoid trapeze;
    Trainer* TheTrainer;
    TrainerApp(Trainer* trainer);

    void Begin();
    void ShowMessageAndErrorInConsole();
    void AddRobotConsoleError(const std::string& errorLabel);
    void AddRobotConsoleMessage(const std::string& message);
    void DeleteRobotConsoleError(const std::string& errorLabel);
    void DeleteRobotConsoleMessage(const std::string& message);
    void RenderUI();
    void MakeDockingPossible();
    void CreateUsersInfoWindow();
    void CreateBotModeWindow();
    void CreatePlotWindow();
    void CreateConsoleWindow();
    void SubplotItemSharing();
    void Plot(int i, std::vector<double>& x, std::vector<double>& y, int index = 0, std::string label = "", std::string style = "", std::string annotation = "", bool shallWeKeepThePlotEvenAfterAClearPlot = true, ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    void Unplot(std::string label);
    void ClearPlot();
    int findIndexOfEmptyPlot();
    void BeginKalmanExample();
    void definePopupWindowRepMeasurement();
    void definePopupWindowOneRMExperiment();
    void addPointToPlot(std::vector<double> coordinates, std::string label);
};
