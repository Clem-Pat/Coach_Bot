#include "TrainerApp.h"
#include "Trainer.h"
#include "imgui_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <vector>
#include <map>
#include <implot/implot.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <implot/implot_internal.h>
#include "Trapezoid.h"
#include <iostream>

#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <cstdlib> // for rand

#include <windows.h>
//
//#define STB_IMAGE_IMPLEMENTATION
//#include "image_stb/stb_image.h"
//#include <imgui_impl_dx12.h>
//#include <d3d12.h>

using namespace std;

TrainerApp::TrainerApp(Trainer* trainer) : trapeze(this) {
    this->TheTrainer = trainer;
    this->listOfPlotsToKeep = std::vector<bool>(nbreOfDataToPlot, true); // We first want to forget nobody
    this->listXToPlot = std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0));
    this->listYToPlot = std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0));
    this->listIndexesToPlot = std::vector<int>(nbreOfDataToPlot, 0);
    this->listLabelsToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->listStylesToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->listColorsToPlot = std::vector<ImVec4>(nbreOfDataToPlot, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    this->listAnnotationsToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->mousePositionAtRightClick = std::vector<double>(2, 0.0);

    Begin();
}

constexpr unsigned int hashString(const char* str, int h = 0) {
    //Translates a char* into a unique int value. Useful for switch case in AddRobotConsoleInfo and DeleteRobotConsoleInfo
    return !str[h] ? 5381 : (hashString(str, h + 1) * 33) ^ str[h];
}

std::vector<double> linspaceApp(double start, double end, int nbreOfReps) {
    std::vector<double> result;
    double step = (end - start) / (nbreOfReps - 1);

    for (int j = 0; j < nbreOfReps; j++) {
        result.push_back(start + j * step);
    }

    return result;
}

std::vector<double> ImPlotPointToVector(const ImPlotPoint& point) {
    std::vector<double> vec;
    vec.push_back(point.x);
    vec.push_back(point.y);
    return vec;
}

void TrainerApp::BeginKalmanExample() {
    x = linspaceApp(0, 100, 1000);
    y = std::vector<double>(x.size());
    y_withoutNoise = std::vector<double>(x.size());

    //double noise = static_cast<double>(rand()) / RAND_MAX * 2.0 - 1.0; // noise between -1 and 1
    //ySmoothed.push_back(5 * std::sin(x[0]) + 10 * std::cos(0.1 * x[0]) + 2 * std::sin(0.01 * x[0]) + x[0] + noise);
    //y[0] = ySmoothed[0];

    y[0] = 0;
    y_withoutNoise[0] = 0;
    ySmoothed.push_back(y[0]);
    ySmoothedKalman.push_back(y[0]);
    ySmoothedKalman2.push_back(y[0]);
    double P = 1.0;
    double P2 = 1.0;

    for (int i = 1; i < x.size(); ++i) {
        double noise = static_cast<double>(rand()) / RAND_MAX * 2.0 - 1.0; // noise between -1 and 1
        y[i] = 5 * std::sin(x[i]) + 10 * std::cos(0.1 * x[i]) + 2 * std::sin(0.01 * x[i]) + x[i] + noise; // y = 5sin(x) + 10cos(0.1x) + 2sin(0.01x) + x
        y_withoutNoise[i] = 5 * std::sin(x[i]) + 10 * std::cos(0.1 * x[i]) + 2 * std::sin(0.01 * x[i]) + x[i];
        ySmoothed.push_back(TheTrainer->botManager.MovingAverageAndExponentialSmoothing(ySmoothed, y[i]));
        std::vector<double> res = TheTrainer->botManager.KalmanFilter(ySmoothedKalman, y[i], P);
        P = res[1];
        ySmoothedKalman.push_back(res[0]);
        std::vector<double> res2 = TheTrainer->botManager.KalmanFilter2(ySmoothedKalman2, y[i], P2);
        P2 = res2[1];
        ySmoothedKalman2.push_back(res2[0]);
    }

}

void TrainerApp::Begin() {
    BeginKalmanExample();
}

int TrainerApp::findIndexOfEmptyPlot() {
    for (int i = 0; i < this->listLabelsToPlot.size(); ++i) {
        if (this->listLabelsToPlot[i] == "") {
            return i;
        }
    }
    return -1;
}

void TrainerApp::Plot(int i, std::vector<double>& x, std::vector<double>& y, int indexOfWindowWhereItPlots, std::string label, std::string style, std::string annotation, bool shallWeKeepThePlotEvenAfterAClearPlot, ImVec4 color) {
    this->listOfPlotsToKeep[i] = shallWeKeepThePlotEvenAfterAClearPlot;
    this->listXToPlot[i] = x;
    this->listYToPlot[i] = y;
    this->listIndexesToPlot[i] = indexOfWindowWhereItPlots;
    this->listLabelsToPlot[i] = label;
    this->listStylesToPlot[i] = style;
    this->listAnnotationsToPlot[i] = annotation;
    this->listColorsToPlot[i] = color;
}

void TrainerApp::Unplot(std::string label) {
    for (int i = 0; i < this->listLabelsToPlot.size(); ++i) {
        if (listLabelsToPlot[i].find(label) != std::string::npos) {
            this->listXToPlot[i] = std::vector<double>(3, 0.0);
            this->listYToPlot[i] = std::vector<double>(3, 0.0);
            this->listIndexesToPlot[i] = 0;
            this->listLabelsToPlot[i] = "";
            this->listStylesToPlot[i] = "";
            this->listColorsToPlot[i] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            this->listAnnotationsToPlot[i] = "";
        }
    }
}

void TrainerApp::ClearPlot() {
    for (int i = 0; i < listOfPlotsToKeep.size(); i++) {
        bool shallWeKeepThisOne = listOfPlotsToKeep[i];
		if (!shallWeKeepThisOne) {
			this->listXToPlot[i] = std::vector<double>(3, 0.0);
			this->listYToPlot[i] = std::vector<double>(3, 0.0);
			this->listIndexesToPlot[i] = 0;
			this->listLabelsToPlot[i] = "";
			this->listStylesToPlot[i] = "";
			this->listColorsToPlot[i] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			this->listAnnotationsToPlot[i] = "";
            this->TheTrainer->dataManager.experimentData[i+1].clear();
		}
	}
    this->TheTrainer->dataManager.experimentData[0].clear();
    listPointsToPlot.clear();
}

void TrainerApp::SubplotItemSharing() {
    double x_min = 100000, x_max = -100000, y_min = 100000, y_max = -100000;
    static int curj = -1;
    rows = (TheTrainer->UsingLoadProfile) ? 3 : 2;
    if (ImPlot::BeginSubplots("##ItemSharing", rows, cols, ImVec2(-1, 200 * rows + 100))) {
        
        ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Sort | ImPlotLegendFlags_Horizontal);
        for (int i = 0; i < rows * cols; ++i) {
            

            if (i == 0 || i == 1) {
                string Abscissa;
                if (i == 0) {
                    Abscissa = "Time in seconds";
                }
                else{
                    Abscissa = "Load in kg";
                }
                //Plot the raw data from the Bot 
                if (ImPlot::BeginPlot("Data", Abscissa.c_str(), "")) {
                  
                    // Set the x range to [x1, x2] and the y range to [y1, y2]
                    //ImPlot::SetupAxesLimits(-5, 10, -10, 10);

                    //Single Points to plot (For example the lower and upper bounds of the 1RM experiment)
                    ImVec4 scatterColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // RGBA: Red color for points to plot
                    ImPlot::PushStyleColor(ImPlotCol_MarkerFill, scatterColor);
                    for (auto& point : listPointsToPlot) {
                        if (point.index == i) {
                            ImPlot::PlotScatter("", &point.x, &point.y, 1);
                            if (point.type != "point" && point.type != "maxVelocity") {
                                //We draw a vertical line where the point is 
                                for (int j = 0; j < listXToPlot.size(); ++j) {
                                    for (int k = 0; k < listXToPlot[j].size(); ++k) {
                                        y_min = (((y_min) < (listYToPlot[j][k])) ? (y_min) : (listYToPlot[j][k]));
                                        y_max = (((y_max) > (listYToPlot[j][k])) ? (y_max) : (listYToPlot[j][k]));
                                    }
                                }
                                double x_points[2] = {point.x, point.x};
                                double y_points[2] = { y_min, y_max };
                                ImPlot::PlotLine("", x_points, y_points, 2);
                            }
                            else if (point.type == "point") {
                                //TODO cout << "Faire une ligne horizontale" << endl;
							}
                        }
                    }
                    ImPlot::PopStyleColor();

                    //Data plot
                    for (int j = 0; j < listIndexesToPlot.size(); ++j) {
                        float fc = 0.01f;
                        if (i == listIndexesToPlot[j]) {
                            if (!(listColorsToPlot[j].x == 0.0f && listColorsToPlot[j].y == 0.0f && listColorsToPlot[j].z == 0.0f && listColorsToPlot[j].w == 0.0f)) {
                                ImPlot::PushStyleColor(ImPlotCol_Line, listColorsToPlot[j]);
                            }
                            if (listStylesToPlot[j] == "scatter") {
                                ImPlot::PlotScatter(listLabelsToPlot[j].c_str(), listXToPlot[j].data(), listYToPlot[j].data(), listXToPlot[j].size());
                            }
                            else if (listStylesToPlot[j] != "")
                            {
                                ImPlot::PlotLine(listLabelsToPlot[j].c_str(), listXToPlot[j].data(), listYToPlot[j].data(), listXToPlot[j].size());
                            }
                            //Show the annotation
                            if (listAnnotationsToPlot[j] != "" && ShowAnnotation) {
                                ImPlot::Annotation(listXToPlot[j][sizeof(listXToPlot[j]) - 1], listYToPlot[j][sizeof(listYToPlot[j]) - 1], ImPlot::GetLastItemColor(), ImVec2(10, 10), false, listAnnotationsToPlot[j].c_str());
                            }
                            //Allow user to drag and drop graphs from one plot to another
                            if (ImPlot::BeginDragDropSourceItem(listLabelsToPlot[j].c_str())) {
                                curj = j;
                                ImGui::SetDragDropPayload("MY_DND", nullptr, 0);
                                ImPlot::ItemIcon(ImPlot::GetLastItemColor()); ImGui::SameLine();
                                ImGui::TextUnformatted(listLabelsToPlot[j].c_str());
                                ImPlot::EndDragDropSource();
                            }
                            if (!(listColorsToPlot[j].x == 0.0f && listColorsToPlot[j].y == 0.0f && listColorsToPlot[j].z == 0.0f && listColorsToPlot[j].w == 0.0f)) {
                                ImPlot::PopStyleColor();
                            }
                        }
                        //Right click on graph to do the regression
                        if (std::find(TheTrainer->predictor.indexesOfDataToDoRegressionOn.begin(), TheTrainer->predictor.indexesOfDataToDoRegressionOn.end(), j - 1) != TheTrainer->predictor.indexesOfDataToDoRegressionOn.end()) {
                            if (ImGui::BeginPopupContextItem(listLabelsToPlot[j].c_str())) {
                                if (ImGui::MenuItem("Stop regression")) {
                                    TheTrainer->predictor.EraseGraphToDoRegressionOn(j - 1, j - 1);
                                }
                                ImGui::EndPopup();
                            }
                        }
                        else {
                            if (listLabelsToPlot[j - 1] != "") {
                                if (ImGui::BeginPopupContextItem(listLabelsToPlot[j].c_str())) {
                                    std::string str = "Do regression";
                                    char char_array[100];
                                    strcpy(char_array, str.c_str());
                                    if (ImGui::MenuItem(char_array)) {
                                        TheTrainer->predictor.AddGraphToDoRegressionOn(j - 1, j - 1);
                                    }
                                    ImGui::EndPopup();
                                }
                            }
                        }
                    }

                    if (ImPlot::BeginDragDropTargetPlot()) {
                        if (ImGui::AcceptDragDropPayload("MY_DND"))
                            listIndexesToPlot[curj] = i;
                        ImPlot::EndDragDropTarget();
                    }
                    //Query 
                    static ImVector<ImPlotRect> rects;
                    static ImPlotRect limits, select;
                    if (ImPlot::IsPlotSelected()) {
                        select = ImPlot::GetPlotSelection();
                        int cntQuery;
                        if (cntQuery > 0) {
                            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6);
                        }
                        if (ImGui::IsMouseClicked(ImPlot::GetInputMap().SelectCancel)) {
                            ImPlot::CancelPlotSelection();
                            rects.push_back(select);
                        }
                    }
                    for (int i = 0; i < rects.size(); ++i) {
                        int cntQuery;
                        if (cntQuery > 0) {
                            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6);
                        }
                        ImPlot::DragRect(i, &rects[i].X.Min, &rects[i].Y.Min, &rects[i].X.Max, &rects[i].Y.Max, ImVec4(1, 0, 1, 1));
                    }
                    limits = ImPlot::GetPlotLimits();
                    //End Query
                    
                    //Create a Point on mouse position 
                    if (ImGui::GetIO().KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !HasAlreadyClickedOnPlot) {
                        bool lowerBoundPointIsDefined = std::any_of(listPointsToPlot.begin(), listPointsToPlot.end(), [](const auto& point) { return point.type == "lowerBound"; }); //Check if a lowerBound point is already defined
                        if (!lowerBoundPointIsDefined) {
                            cout << "Create a lowerBound point" << endl;
                            minBound = ImPlot::GetPlotMousePos().x;
                            listPointsToPlot.push_back(Point(this, std::vector<double>{ImPlot::GetPlotMousePos().x, 0.0}, "lowerBound", 0));
                            HasAlreadyClickedOnPlot = true;
                            lowerBoundPointIsDefined = true;
                        }
                        else{
                            if (ImPlot::GetPlotMousePos().x > listPointsToPlot.back().x) {
                                cout << "Create a upperBound point" << endl;
                                maxBound = ImPlot::GetPlotMousePos().x;
                                listPointsToPlot.push_back(Point(this, std::vector<double>{ImPlot::GetPlotMousePos().x, 0.0}, "upperBound", 0));
                                HasAlreadyClickedOnPlot = true;
                                lowerBoundPointIsDefined = false;
                            }
                            else {
                                cout << "Create a lowerBound point" << endl;
                                listPointsToPlot.erase(std::remove_if(listPointsToPlot.begin(), listPointsToPlot.end(), [](const auto& point) { return point.type == "upperBound" || point.type == "lowerBound"; }), listPointsToPlot.end()); //Remove all points used to make manual boundaries
                                minBound = ImPlot::GetPlotMousePos().x;
                                maxBound = 1000000;
                                listPointsToPlot.push_back(Point(this, std::vector<double>{ImPlot::GetPlotMousePos().x, 0.0}, "lowerBound", 0));
                                HasAlreadyClickedOnPlot = true;
                                lowerBoundPointIsDefined = true;
                            }
                        }
                    }

                    ImPlot::EndPlot();
                }
            }

            else if (i == 5) {
                //Plot the example of Kalman filter
                if (ImPlot::BeginPlot("Smoothing", "Time in seconds", "")) {
                    /*ImPlot::PlotLine("y", x.data(), y.data(), x.size());
                    ImPlot::PlotLine("ySmoothed", x.data(), ySmoothed.data(), x.size());
                    ImPlot::PlotLine("yKalman", x.data(), ySmoothedKalman.data(), x.size());
                    ImPlot::PlotLine("yKalman2", x.data(), ySmoothedKalman2.data(), x.size());
                    ImPlot::EndPlot();*/

                    //Comparing the two methods errors regarding the real data
                    std::vector<double> difference(ySmoothedKalman.size());
                    for (size_t i = 0; i < ySmoothedKalman.size(); ++i) {
                        difference[i] = std::abs(ySmoothedKalman[i] - y_withoutNoise[i]);
                    }
                    std::vector<double> difference2(ySmoothedKalman2.size());
                    for (size_t i = 0; i < ySmoothedKalman2.size(); ++i) {
                        difference2[i] = std::abs(ySmoothedKalman2[i] - y_withoutNoise[i]);
                    }
                    ImPlot::PlotLine("Kalman", x.data(), difference.data(), x.size());
                    ImPlot::PlotLine("Kalman2", x.data(), difference2.data(), x.size());
                    ImPlot::EndPlot();
                }

            }

            if (TheTrainer->UsingLoadProfile && i == 2) {
                if (ImPlot::BeginPlot("Profile", "Length (cm)", "Weight (kg)")) {
                    std::vector<double> vertices_x;
                    std::vector<double> vertices_y;
                    for (int k = 0; k < trapeze.vertices.size(); ++k) {
                        vertices_x.push_back(trapeze.vertices[k][0]);
                        vertices_y.push_back(trapeze.vertices[k][1]);
                    }
                    ImPlot::PlotLine("Profile", vertices_x.data(), vertices_y.data(), vertices_x.size());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        ImPlotPoint mousePos = ImPlot::PixelsToPlot(ImGui::GetMousePos());
                        mousePositionAtRightClick = ImPlotPointToVector(mousePos);
                        IndexOfVertexFocusedOn = trapeze.isNearVertex(mousePositionAtRightClick);
                        ImGui::OpenPopup("VertexMenu");
                    }

                    for (int i = 0; i < trapeze.vertices.size(); ++i) {
                        ImPlot::DragPoint(i, &trapeze.vertices[i][0], &trapeze.vertices[i][1], ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
                    }


                    if (ImGui::BeginPopup("VertexMenu")) {
                        bool deleted = false;
                        if (ImGui::MenuItem("Create new point")) {
                            trapeze.createPoint(mousePositionAtRightClick);
                        }
                        if (IndexOfVertexFocusedOn != -1) {
                            if (ImGui::MenuItem("Delete point")) {
                                trapeze.deletePoint(IndexOfVertexFocusedOn);
                                deleted = true;
                            }
                            std::ostringstream stream;
                            stream.str("");
                            static char xpos[5];
                            stream << std::fixed << std::setprecision(2) << trapeze.vertices[IndexOfVertexFocusedOn][0];
                            std::string str = stream.str();
                            strncpy(xpos, str.c_str(), sizeof(xpos));
                            ImGui::PushItemWidth(50);
                            if (ImGui::InputText("X position", xpos, sizeof(xpos))) {
                                trapeze.vertices[IndexOfVertexFocusedOn][0] = std::stod(xpos);
                            }

                            stream.str(""); // Clear the stream
                            static char ypos[5];
                            stream << std::fixed << std::setprecision(2) << trapeze.vertices[IndexOfVertexFocusedOn][1];
                            str = stream.str();
                            strncpy(ypos, str.c_str(), sizeof(ypos));
                            if (ImGui::InputText("Y position", ypos, sizeof(ypos))) {
                                trapeze.vertices[IndexOfVertexFocusedOn][1] = std::stod(ypos);
                            }
                            ImGui::PopItemWidth();
                            if (deleted) {
                                IndexOfVertexFocusedOn = -1;
                            }
                        }
                        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImPlot::EndPlot();
                }
            }

        }
        HasAlreadyClickedOnPlot = false;

        ImPlot::EndSubplots();
    }
}

void TrainerApp::MakeDockingPossible() {
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
}

void TrainerApp::CreateUsersInfoWindow() {

    ImGui::Begin("Settings");

    char usernameBuffer[256] = "";
    strncpy_s(usernameBuffer, TheTrainer->Username.c_str(), sizeof(usernameBuffer) - 1);
    if (ImGui::InputText("Username", usernameBuffer, IM_ARRAYSIZE(usernameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) { // This block is executed when the user presses Enter while the input field is active
        TheTrainer->Username = std::string(usernameBuffer);
    }
    char typeOfExercice[256] = "";
    strncpy_s(typeOfExercice, TheTrainer->TypeOfExercice.c_str(), sizeof(typeOfExercice) - 1);
    if (ImGui::InputText("Type of exercice", typeOfExercice, IM_ARRAYSIZE(typeOfExercice), ImGuiInputTextFlags_EnterReturnsTrue)) { // This block is executed when the user presses Enter while the input field is active
        TheTrainer->TypeOfExercice = std::string(typeOfExercice);
    }
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    if (ImGui::SliderInt("Total weight wanted (Kg)", &TheTrainer->botManager.TemporaryTotalWeightWanted, 0, TheTrainer->botManager.MaxOutputMotorTorque)) {
        trapeze.update(TheTrainer->botManager.TemporaryTotalWeightWanted); //Make the profile change accordingly to the slide button
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) { // Now that the user has finished sliding, update the TotalWeightWanted
        TheTrainer->botManager.TotalWeightWanted = TheTrainer->botManager.TemporaryTotalWeightWanted;
    }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    if (ImGui::Checkbox("Want to use load profile ?", &TheTrainer->UsingLoadProfile)) {
        if (TheTrainer->UsingLoadProfile){
            double acquiredMaxHeight = TheTrainer->predictor.seriesAnalyzers[0].get()->getMeanMaxHeight();
            if (acquiredMaxHeight == 0) {
                AddRobotConsoleError("We do not have enough data on your repetitions. Please do some repetitions at a constant load before using the profile");
                TheTrainer->UsingLoadProfile = false;
            }
            else {
                TheTrainer->MaximumLengthPulledByUser = acquiredMaxHeight;
                trapeze.update(TheTrainer->botManager.TotalWeightWanted);
                DeleteRobotConsoleError("We do not have enough data on your repetitions. Please do some repetitions at a constant load before using the profile");
                AddRobotConsoleMessage("We analyzed your maximum height. It is " + std::to_string(acquiredMaxHeight));
            }
        }
    }
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    if (!this->TheTrainer->WantToStartBot && !this->TheTrainer->BotWorkingInProgress) {
        if (ImGui::Button("Start Bot")) {
            this->TheTrainer->WantToStartBot = true;
        }
        if (!this->TheTrainer->predictor.WantToAnalyzeSeriesForOneRM){
            ImGui::SameLine();
            if (ImGui::Button("Analyze past series for One RM experiment")) {
                this->TheTrainer->predictor.WantToAnalyzeSeriesForOneRM = true;
                this->TheTrainer->UsingLoadProfile = false;
                cout << "WANT TO Analyze repetitions" << endl;
            }
        }
    }
    if (this->TheTrainer->BotWorkingInProgress) {
        if (ImGui::Button("Stop Bot")) {
            this->TheTrainer->WantToStopBot = true;
        }
        if (!this->TheTrainer->predictor.CountingNumberOfReps){
            if (ImGui::Button("One RM by Epley")) {
                this->TheTrainer->UsingLoadProfile = false;
                listPointsToPlot.push_back(Point(this, std::vector<double>{listXToPlot[36 - 1].back(), 0.0}, "startOfEpleyExperiment", 0));
                ImGui::OpenPopup("One RM Lander and Epley Experiment");
            }
        }
        else {/*No need for a button to stop because the button "Ok" of the popup will stop the experiment*/}
        definePopupWindowOneRMEpley();

        if (!this->TheTrainer->predictor.DoingContinuous1RMExperiment){
            if (ImGui::Button("Do continuous One RM experiment")) {
                this->TheTrainer->predictor.WantContinuous1RMExperiment = true;
                this->TheTrainer->UsingLoadProfile = false;
                listPointsToPlot.push_back(Point(this, std::vector<double>{listXToPlot[36 - 1].back(), 0.0}, "startOfContinuousOneRMExperiment", 0));
            }
        }
        else if (this->TheTrainer->predictor.DoingContinuous1RMExperiment) {
            if (ImGui::Button("Stop continuous One RM experiment")) {
                this->TheTrainer->predictor.WantStopContinuous1RMExperiment = true;
                listPointsToPlot.push_back(Point(this, std::vector<double>{listXToPlot[36 - 1].back(), 0.0}, "endOfContinuousOneRMExperiment", 0));
            }
        }
    }
    if (!this->TheTrainer->BotWorkingInProgress) {
        bool WantOpenstepsMethod = false;
        bool WantOpenEpley = false;
        if (this->listXToPlot != std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0))) {
            ImGui::SameLine();
            if (ImGui::Button("Clear Plot")) {
                ClearPlot();
            }
        }
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        if (ImGui::Button("Open possible experiments")) {
            ImGui::OpenPopup("Menu");
		}

        if (ImGui::BeginPopup("Menu")) {
            if (ImGui::MenuItem("Explanation on One RM experiment by 3 steps method")) {
                WantOpenstepsMethod = true;
            }
            if (ImGui::MenuItem("Explanation on One RM experiment by Epley method")) {
                WantOpenEpley = true;
            }
            ImGui::EndPopup();
        }
        if (WantOpenEpley){
		    ImGui::OpenPopup("One RM by Epley method");
		    WantOpenEpley = false;
	    }
        if (WantOpenstepsMethod) {
            ImGui::OpenPopup("One RM by 3 steps method");
            WantOpenstepsMethod = false;
        }
        definePopupWindowOneRM3StepsExplanation();
        definePopupWindowOneRMEpleyExplanation();
    }
    ImGui::End();
}

void TrainerApp::CreateBotModeWindow() {
    ImGui::Begin("Bot Mode");
    ImGui::Text("Select the bot mode");
    int oldBotModeSelected = TheTrainer->botManager.ClutchModeSelected;
    if (ImGui::RadioButton("Option 1 : gear low speed both sides", TheTrainer->botManager.ClutchModeSelected == 0))
        TheTrainer->botManager.ClutchModeSelected = 0;
    if (ImGui::RadioButton("Option 2 : gear high speed both sides", TheTrainer->botManager.ClutchModeSelected == 1))
        TheTrainer->botManager.ClutchModeSelected = 1;
    if (ImGui::RadioButton("Option 3 : gear low speed left side", TheTrainer->botManager.ClutchModeSelected == 2))
        TheTrainer->botManager.ClutchModeSelected = 2;
    if (ImGui::RadioButton("Option 4 : gear low speed right side", TheTrainer->botManager.ClutchModeSelected == 3))
        TheTrainer->botManager.ClutchModeSelected = 3;
    if (ImGui::RadioButton("Option 5 : gear high speed right side", TheTrainer->botManager.ClutchModeSelected == 4))
        TheTrainer->botManager.ClutchModeSelected = 4;
    if (ImGui::RadioButton("Option 6 : gear high speed left side", TheTrainer->botManager.ClutchModeSelected == 5))
        TheTrainer->botManager.ClutchModeSelected = 5;
    if (oldBotModeSelected != TheTrainer->botManager.ClutchModeSelected) {
        TheTrainer->botManager.ChangeClutchOption();
    }
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    if(ImGui::Checkbox("Take off the clutch", &TheTrainer->botManager.WantTakeOffTheClutch)){
        if (TheTrainer->botManager.WantTakeOffTheClutch) {
            TheTrainer->botManager.TakeOffTheClutch();
        }
        else {
            TheTrainer->botManager.PutOnTheClutch();
        }
    }
    ImGui::End();
}

void TrainerApp::CreatePlotWindow() {
    if (ImGui::Begin("Output Data plot")) {
        ImGui::Checkbox("Show Regression equation", &ShowAnnotation);
        ImGui::SameLine();
        if (ImGui::Button("Reset Profile")) { trapeze.Begin(); }
        TrainerApp::SubplotItemSharing();
    }
    ImGui::End();
}

void TrainerApp::CreateConsoleWindow() {
    ImGui::Begin("Robot output console");
    ImGui::Text("Output console");
    ImGui::End();
}

void TrainerApp::DefineFirstPopup() {
    ImGui::SetNextWindowSize(ImVec2(410, 300));
    // Calculate the center position for the text
    const char* text1 = "Hello, I am your personnal trainer.";
    const char* text2 = "To calibrate the system and to make your muscles warm up,";
    const char* text3 = "please do some repetitions with a low load.";
    ImVec2 textSize1 = ImGui::CalcTextSize(text1);
    ImVec2 textSize2 = ImGui::CalcTextSize(text2);
    ImVec2 textSize3 = ImGui::CalcTextSize(text3);
    if (ImGui::BeginPopupModal("Calibration notice", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((410 - textSize1.x) * 0.5f); // to center the text
        ImGui::Text(text1);
        ImGui::SetCursorPosX((410 - textSize2.x) * 0.5f); // to center the text
        ImGui::Text(text2);
        ImGui::SetCursorPosX((410 - textSize3.x) * 0.5f); // to center the text
        ImGui::Text(text3);
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((410 - 120) * 0.5f); // to center the button
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            ShowFirstPopup = false;
        }
        ImGui::EndPopup();
    }
}

void TrainerApp::AddRobotConsoleInfo(const std::string& infoLabel, std::string type) {
    //type = "message", "warning", "error"

    time_t rawtime = time(nullptr);
    struct tm* timeinfo = localtime(&rawtime);
    char buffer[9];
    strftime(buffer, 9, "%H:%M:%S", timeinfo);
    std::string strTime(buffer);
    std::string messageLabelToShow = strTime + " " + infoLabel;

    //There are to many messages in the console, we erase the oldest one
    if (listErrorsInConsole.size() + listMessagesInConsole.size() >= 20) {
        DeleteOldestConsoleInfo();
    }

    std::vector<std::string>* targetList = nullptr; // Pointer to the target list wether if it is an error, a warning or a message
    switch (hashString(type.c_str())) {
    case hashString("error"):
        targetList = &this->listErrorsInConsole;
        break;
    case hashString("warning"):
        targetList = &this->listWarningsInConsole;
        break;
    case hashString("message"):
        targetList = &this->listMessagesInConsole;
        break;
    default:
        break;
    }

    if (targetList != nullptr) {
        DeleteRobotConsoleInfo(infoLabel, type);
        targetList->push_back(messageLabelToShow);
    }
    else {
        cout << "ERROR : cannot add message '" << infoLabel << "' because we don't know the type of the message" << endl;
    }
}

void TrainerApp::AddRobotConsoleMessage(const std::string& messageLabel) {
    AddRobotConsoleInfo(messageLabel, "message");
}

void TrainerApp::AddRobotConsoleWarning(const std::string& messageLabel) {
    AddRobotConsoleInfo(messageLabel, "warning");
}

void TrainerApp::AddRobotConsoleError(const std::string& messageLabel) {
	AddRobotConsoleInfo(messageLabel, "error");
}

void TrainerApp::DeleteOldestConsoleInfo() {
    auto combinedMessages = MergeAndSortMessages();

	if (combinedMessages.size() != 0) {
        if (std::get<1>(combinedMessages[0]) == "error") { listErrorsInConsole.erase(listErrorsInConsole.begin()); }
        else if (std::get<1>(combinedMessages[0]) == "warning") { listWarningsInConsole.erase(listWarningsInConsole.begin()); }
        else if (std::get<1>(combinedMessages[0]) == "message") { listMessagesInConsole.erase(listMessagesInConsole.begin()); }
        else { cout << "-- WARNING Impossible to erase the oldest message because the type '" << std::get<1>(combinedMessages[0]) << "' is unknown for message : " << std::get<0>(combinedMessages[0]) << endl; }
	}
    else {
        cout << "-- WARNING Impossible to erase the oldest message because the list of messages is empty" << endl;
    }
}

void TrainerApp::DeleteRobotConsoleInfo(const std::string& infoLabel, std::string type) {
    std::vector<std::string>* targetList = nullptr; // Pointer to the target list

    switch (hashString(type.c_str())) {
    case hashString("error"):
        targetList = &this->listErrorsInConsole;
        break;
    case hashString("warning"):
        targetList = &this->listWarningsInConsole;
        break;
    case hashString("message"):
        targetList = &this->listMessagesInConsole;
        break;
    default:
        break;
    }
    if (targetList != nullptr) {
	    for (auto it = targetList->begin(); it != targetList->end(); ) {
		    if (it->find(infoLabel) != std::string::npos) {
			    it = targetList->erase(it);
		    }
		    else {
			    ++it;
		    }
        }
    }
    else {
        cout << "ERROR : cannot erase message '" << infoLabel << "' because we don't know the type of the message" << endl;
    }
}

void TrainerApp::DeleteRobotConsoleMessage(const std::string& messageLabel) {
    DeleteRobotConsoleInfo(messageLabel, "message");
}

void TrainerApp::DeleteRobotConsoleWarning(const std::string& messageLabel) {
    DeleteRobotConsoleInfo(messageLabel, "warning");
}

void TrainerApp::DeleteRobotConsoleError(const std::string& messageLabel) {
    DeleteRobotConsoleInfo(messageLabel, "error");
}

std::vector<std::tuple<std::string, std::string>> TrainerApp::MergeAndSortMessages() {
    std::vector<std::tuple<std::string, std::string>> combinedMessages;

    // Merge with tag
    for (const auto& msg : listErrorsInConsole) {
        combinedMessages.emplace_back(msg, "error"); // Add the tag "error" to the message
    }
    for (const auto& msg : listWarningsInConsole) {
        combinedMessages.emplace_back(msg, "warning");  // Add the tag "warning" to the message
    }
    for (const auto& msg : listMessagesInConsole) {
        combinedMessages.emplace_back(msg, "message"); // Add the tag "message" to the message
    }

    // Sort based on the first 8 characters of the message
    std::sort(combinedMessages.begin(), combinedMessages.end(),
        [](const std::tuple<std::string, std::string>& a, const std::tuple<std::string, std::string>& b) {
            return std::get<0>(a).substr(0, 8) < std::get<0>(b).substr(0, 8);
        });

    return combinedMessages;
}

void TrainerApp::ShowMessagesAndErrorInConsole() {
    //Show all the messages from listMessagesInConsole, listWarningsInConsole and listErrorsInConsole in the console ordered by time
    auto combinedMessages = MergeAndSortMessages();
    for (const auto& message : combinedMessages) {
        if (ImGui::Begin("Robot output console")) {
			if (std::get<1>(message) == "error") {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color
			}
			else if (std::get<1>(message) == "warning") {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f)); // Orange color
			}
			else if (std::get<1>(message) == "message") {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // green color
			}
			ImGui::Text("%s", std::get<0>(message).c_str());
			ImGui::PopStyleColor(); // Reset to default color
			if (ImGui::BeginPopupContextItem(std::get<0>(message).c_str())) {
				if (ImGui::MenuItem("Copy")) {
                    cout << "want to copy " << std::get<0>(message) << endl;
					ImGui::SetClipboardText(std::get<0>(message).c_str());
				}
                if (ImGui::MenuItem("Delete")) {
                    DeleteRobotConsoleInfo(std::get<0>(message), std::get<1>(message));
                }
				ImGui::EndPopup();
			}
			ImGui::End();
		}
    }
}

void TrainerApp::definePopupWindowOneRMEpley() {
    ImGui::SetNextWindowSize(ImVec2(400, 300));
    LoadDuringReps = TheTrainer->botManager.TotalWeightWanted;
    if (ImGui::BeginPopupModal("One RM Lander and Epley Experiment", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Please do as many reps as possible and enter the ammount in the text box");
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::PushItemWidth(100);
        ImGui::InputInt("Load during the reps", &LoadDuringReps);
        ImGui::InputInt("Number of reps", &NumberOfReps);
        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        ImGui::Text(("One RM according to Lander : " + std::to_string(TheTrainer->predictor.Lander(LoadDuringReps, NumberOfReps))).c_str());
        ImGui::Text(("One RM according to Epley : " + std::to_string(TheTrainer->predictor.Epley(LoadDuringReps, NumberOfReps))).c_str());
        ImGui::Text(("One RM according to Mayhew : " + std::to_string(TheTrainer->predictor.Mayhew(LoadDuringReps, NumberOfReps))).c_str());

        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            listPointsToPlot.push_back(Point(this, std::vector<double>{listXToPlot[36 - 1].back(), 0.0}, "endOfEpleyExperiment", 0));
            TheTrainer->predictor.WantStopCountNumberOfReps = true;
            DeleteRobotConsoleMessage("One RM according to Lander : ");
            DeleteRobotConsoleMessage("One RM according to Epley : ");
            DeleteRobotConsoleMessage("One RM according to Mayhew : ");
            AddRobotConsoleMessage("One RM according to Lander : " + std::to_string(TheTrainer->predictor.Lander(LoadDuringReps, NumberOfReps)));
            AddRobotConsoleMessage("One RM according to Epley : " + std::to_string(TheTrainer->predictor.Epley(LoadDuringReps, NumberOfReps)));
            AddRobotConsoleMessage("One RM according to Mayhew : " + std::to_string(TheTrainer->predictor.Mayhew(LoadDuringReps, NumberOfReps)));
        }
        ImGui::EndPopup();
    }
}

void TrainerApp::definePopupWindowOneRMEpleyExplanation() {
    ImGui::SetNextWindowSize(ImVec2(450, 300));
	if (ImGui::BeginPopupModal("One RM by Epley method", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        cout << "popup opened" << endl;
		ImGui::Text("The Epley method is a simple way to estimate your One RM");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
		ImGui::Text("   Start the motors and hit the button 'One RM by Epley'");
		ImGui::Text("   Do as many reps as possible at a given weight");
        ImGui::Text("   Hit the button OK when you are done");
        ImGui::Text("   Stop the motors");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Text("The result of the calculation appears in the console");
		ImGui::Text("Make sure to be exhausted at the end of the set.");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
		ImGui::SetCursorPosX((450 - 120) * 0.5f); // to center the button
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
    }
}

void TrainerApp::definePopupWindowOneRM3StepsExplanation(){
	ImGui::SetNextWindowSize(ImVec2(700, 300));
	if (ImGui::BeginPopupModal("One RM by 3 steps method", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("The 3 steps method is a precise way to estimate your One RM");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Text("   Start the motors and hit the button 'One RM by 3 steps'");
		ImGui::Text("   1.  Do as many reps as possible at 40%% of your known One RM or at 40%% of your weight");
        ImGui::Text("   2.  Do as many reps as possible at 60%% of your known One RM or at 60%% of your weight");
        ImGui::Text("   3.  Do as many reps as possible at 80%% of your known One RM or at 80%% of your weight");
        ImGui::Text("   Stop the motors");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Text("Make sure to be exhausted at the end of every set.\nOn the first graph you have your data, \non the second, the maximum velocity of each repetition");
        ImGui::Text("You can delete a point at anytime on the second graph if you feel that it is not representative");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((700 - 120) * 0.5f); // to center the button
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
	}

}

void TrainerApp::addPointToPlot(vector<double> coordinates, std::string label) {
    listPointsToPlot.push_back(Point(this, coordinates, label, 0));
}

void TrainerApp::UndoLastAction(){
	if (listPointsToPlot.size() > 0) {
        for (int k = listPointsToPlot.size() - 1; k >= 0; --k) {
            if (listPointsToPlot[k].type == "lowerBound" || listPointsToPlot[k].type == "upperBound") {
		        listPointsToPlot.erase(listPointsToPlot.begin() + k);
                break;
            }
        }
	}
}   

void TrainerApp::RenderUI()
{
    MakeDockingPossible();

    CreateUsersInfoWindow();
    CreateBotModeWindow();
    CreatePlotWindow();
    CreateConsoleWindow();
    ShowMessagesAndErrorInConsole();

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)) && ImGui::GetIO().KeyCtrl) {
        UndoLastAction();
    }

    if (ShowFirstPopup){
        if ((std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)).count() < 15) {
            ImGui::OpenPopup("Calibration notice");
        }
        else {
            ShowFirstPopup = false;
            ImGui::CloseCurrentPopup();
        }
        DefineFirstPopup();
    }
    ImGui::End();
}
