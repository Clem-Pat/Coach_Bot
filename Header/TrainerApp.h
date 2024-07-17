#pragma once

#define GLEW_STATIC
#include <image_stb/GL/glew.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include "imgui.h"
#include "Trapezoid.h"
#include "Point.h"
#include <chrono>

class Trainer;


class TrainerApp {
public:

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
    std::vector<std::string> listWarningsInConsole; // List of messages to show in console
    std::vector<std::string> listErrorsInConsole; // List of error messages to show in console
    int indexOfWindowToPlotRawData = 0;
    int nbreOfDataToPlot = 60;
    bool ShowAnnotation = false;

    // Variables for the profile plot
    int rows = 2;
    int cols = 1;
    bool WantToPlotProfile = true;
    int IndexOfVertexFocusedOn = -1;
    std::vector<double> mousePositionAtRightClick;
    std::vector<bool> WantPlotResize = std::vector<bool>(rows*cols, false);

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

    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    bool ShowFirstPopup = true;

    Trapezoid trapeze;
    Trainer* TheTrainer;
    TrainerApp(Trainer* trainer);

    void Begin();
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
    void Plot(int i, std::vector<double>& x, std::vector<double>& y, int index = 0, std::string label = "", std::string style = "", std::string annotation = "", bool shallWeKeepThePlotEvenAfterAClearPlot = true, ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    void Unplot(std::string label);
    void ClearPlot();
    int findIndexOfEmptyPlot();
    void BeginKalmanExample();
    void definePopupWindowOneRMEpley();
    void definePopupWindowOneRMEpleyExplanation();
    void definePopupWindowOneRM3StepsExplanation();
    void addPointToPlot(std::vector<double> coordinates, std::string label);
    std::vector<std::tuple<std::string, std::string>> MergeAndSortMessages(); // Merge and sort infos from messages, warnings and errors
};
