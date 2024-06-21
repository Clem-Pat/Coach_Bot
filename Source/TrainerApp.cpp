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
//#include <iostream>
#include <sstream>
#include <iomanip>
#include <implot/implot_internal.h>
#include "Trapezoid.h"
#include <iostream>

#include <vector>
#include <numeric>
#include <cmath>
#include <cstdlib> // for rand

#include <windows.h>

//#define GLEW_STATIC
//#include <image_stb/GL/glew.h> 
#define STB_IMAGE_IMPLEMENTATION
#include "image_stb/stb_image.h"
#include <imgui_impl_dx12.h>
#include <d3d12.h>

using namespace std;

TrainerApp::TrainerApp(Trainer* trainer) : trapeze(this){
    this->TheTrainer = trainer;
    this->listXToPlot = std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0));
    this->listYToPlot = std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0));
    this->listIndexesToPlot = std::vector<int>(nbreOfDataToPlot, 0);
    this->listLabelsToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->listStylesToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->listColorsToPlot = std::vector<ImVec4>(nbreOfDataToPlot, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    this->listAnnotationsToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->TypeOfExerciceOptionSelected = 0;
    this->mousePositionAtRightClick = std::vector<double>(2, 0.0);

    Begin();
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

void TrainerApp::Plot(int i, std::vector<double>& x, std::vector<double>& y, int indexOfWindowWhereItPlots, std::string label, std::string style, std::string annotation, ImVec4 color) {
    this->listXToPlot[i] = x;
    this->listYToPlot[i] = y;
    this->listIndexesToPlot[i] = indexOfWindowWhereItPlots;
    this->listLabelsToPlot[i] = label;
    this->listStylesToPlot[i] = style;
    this->listColorsToPlot[i] = color;
    this->listAnnotationsToPlot[i] = annotation;
}

void TrainerApp::SubplotItemSharing() {
	
	static int rows = 2;
	static int cols = 1;
	static int curj = -1;
    if (this->TheTrainer->Want1RMExperiment) {
        rows = 3;
        indexOfWindowToPlotRawData = 2;
    }
    else {
        rows = 2;
        indexOfWindowToPlotRawData = 0;
    }
    if (ImPlot::BeginSubplots("##ItemSharing", rows, cols, ImVec2(-1, 200 * rows + 100))) {
        ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Sort | ImPlotLegendFlags_Horizontal);
        for (int i = 0; i < rows * cols; ++i) {

            if ((!this->TheTrainer->Want1RMExperiment && (i == 0)) || (this->TheTrainer->Want1RMExperiment && (i == 0 || i == 1 || i == 2))) {
                string Abscissa;
                if (this->TheTrainer->Want1RMExperiment && i == 0) {
                    Abscissa = "Load in kg";
                }
                else
                {
                    Abscissa = "Time in seconds";
                }
                //Plot the raw data from the Bot 
                if (ImPlot::BeginPlot("Data", Abscissa.c_str(), "")) {
                    // Set the x range to [x1, x2] and the y range to [y1, y2]
                    ImPlot::SetupAxesLimits(-5,10,-10,10);
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

                            if (std::find(TheTrainer->predictor.indexesOfDataToDoRegressionOn.begin(), TheTrainer->predictor.indexesOfDataToDoRegressionOn.end(), j-1) != TheTrainer->predictor.indexesOfDataToDoRegressionOn.end()) {
                                if (ImGui::BeginPopupContextItem(listLabelsToPlot[j].c_str())) {
                                    if (ImGui::MenuItem("Stop regression")) {
                                        TheTrainer->predictor.EraseGraphToDoRegressionOn(j-1, j-1);
                                    }
                                    ImGui::EndPopup();
                                }
                            }
                            else {
                                if (listLabelsToPlot[j - 1] != "") {
                                    if (ImGui::BeginPopupContextItem(listLabelsToPlot[j].c_str())) {

                                        //std::string str = "Do regression on " + listLabelsToPlot[j - 1];
                                        std::string str = "Do regression";
                                        char char_array[100];
                                        strcpy(char_array, str.c_str());
                                        if (ImGui::MenuItem(char_array)) {
                                            TheTrainer->predictor.AddGraphToDoRegressionOn(j-1, j-1);
                                        }
                                        ImGui::EndPopup();
                                    }
                                }
                            }
                            if (listAnnotationsToPlot[j] != "" && ShowAnnotation){
                                ImPlot::Annotation(listXToPlot[j][sizeof(listXToPlot[j]) - 1], listYToPlot[j][sizeof(listYToPlot[j]) - 1], ImPlot::GetLastItemColor(), ImVec2(10, 10), false, listAnnotationsToPlot[j].c_str());
                            }

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
                        int cnt;
                        if (cnt > 0) {
                            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6);
                        }
                        if (ImGui::IsMouseClicked(ImPlot::GetInputMap().SelectCancel)) {
                            ImPlot::CancelPlotSelection();
                            rects.push_back(select);
                        }
                    }
                    for (int i = 0; i < rects.size(); ++i) {
                        int cnt;
                        if (cnt > 0) {
                            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6);
                        }
                        ImPlot::DragRect(i, &rects[i].X.Min, &rects[i].Y.Min, &rects[i].X.Max, &rects[i].Y.Max, ImVec4(1, 0, 1, 1));
                    }
                    limits = ImPlot::GetPlotLimits();
                    //End Query
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

            else if (!this->TheTrainer->Want1RMExperiment && WantToPlotProfile && i == 1) {
                if (WantToPlotProfile){
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
        }
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
    ImGui::InputText("Username", this->TheTrainer->username, sizeof(this->TheTrainer->username));

    if (ImGui::RadioButton("Squats training", TypeOfExerciceOptionSelected == 0))
        TypeOfExerciceOptionSelected = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Bench press training", TypeOfExerciceOptionSelected == 1))
        TypeOfExerciceOptionSelected = 1;

    if (TypeOfExerciceOptionSelected==0){this->TheTrainer->typeOfExercice = "Squats";}
    else if (TypeOfExerciceOptionSelected == 1){this->TheTrainer->typeOfExercice = "Bench Press";}
        
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    if (ImGui::SliderInt("Input Motor Torque (Kg)", &TheTrainer->botManager.InputMotorTorque, 0, TheTrainer->botManager.MaxOutputMotorTorque)) {
        trapeze.update();
    }
    ImGui::Spacing();
    if (ImGui::Button("Initialize the rep measurements")) {
        TheTrainer->botManager.WantRecordDistances = true;
        this->TheTrainer->WantToStartBot = true;
        ImGui::OpenPopup("Initialize the rep measurements");
    }
    definePopupWindowRepMeasurement();

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Checkbox("Want Acquisition ?", &TheTrainer->AcquisitionAsked);
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    if (!this->TheTrainer->WantToStartBot && !this->TheTrainer->BotWorkingInProgress) {
        if (ImGui::Button("Start Bot")) {
            this->TheTrainer->WantToStartBot = true;
            this->TheTrainer->UsingLoadProfile = true;
            this->TheTrainer->Want1RMExperiment = false;
        }
        if (ImGui::Button("Start 1RM Experiment")) {
            this->TheTrainer->WantToStartBot = true;
            this->TheTrainer->UsingLoadProfile = false;
            this->TheTrainer->Want1RMExperiment = true;
            //indexOfWindowToPlotRawData = 5;
        }
    }
    if (this->TheTrainer->BotWorkingInProgress) {
        if (this->TheTrainer->Want1RMExperiment && !this->TheTrainer->dataManager.WantMeasureRep) {
            if (ImGui::Button("Start the next rep")) {
                this->TheTrainer->dataManager.WantMeasureRep = true;
            }
            ImGui::SameLine();
        }
        if (this->TheTrainer->Want1RMExperiment && this->TheTrainer->dataManager.WantMeasureRep) {
            if (ImGui::Button("Finish the rep")) {
                this->TheTrainer->dataManager.WantMeasureRep = false;
                int index = 23; //arbitrary index, we just need an empty spot to plot the data
                Plot(index, this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep[0], this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep[1], 1, "Distance during Rep", "scatter");
                Plot(index+1, this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep[0], this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep[2], 1, "Velocity during Rep", "scatter");

                TheTrainer->OneRMExperimentLoads.push_back(TheTrainer->botManager.InputMotorTorque);
                TheTrainer->OneRMExperimentVelocities.push_back(std::accumulate(this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep[2].begin(), this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep[2].end(), 0.0) / TheTrainer->dataManager.TimeDistanceAndVelocityInRep[2].size());
                Plot(index + 2, TheTrainer->OneRMExperimentLoads, TheTrainer->OneRMExperimentVelocities, 0, "Mean velocity during reps", "scatter");
                this->TheTrainer->dataManager.TimeDistanceAndVelocityInRep = std::vector<std::vector<double>>(3, std::vector<double>());;
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Stop Bot")) {
            this->TheTrainer->WantToStopBot = true; 
        }
        if (this->TheTrainer->Want1RMExperiment) {
            if (ImGui::Button("Do Lander and Epley experiment")) {
                this->TheTrainer->dataManager.WantMeasureRep = false;
                ImGui::OpenPopup("One RM Experiment");
            }
            definePopupWindowOneRMExperiment();

        }

    }
    if (!this->TheTrainer->BotWorkingInProgress) {
        if (this->listXToPlot != std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0))) {
            ImGui::SameLine();
            if (ImGui::Button("Clear Plot")) {
                this->listXToPlot = std::vector<std::vector<double>>(listXToPlot.size(), std::vector<double>(listXToPlot[0].size(), 0.0));
                this->listYToPlot = std::vector<std::vector<double>>(listYToPlot.size(), std::vector<double>(listYToPlot[0].size(), 0.0));
                this->listIndexesToPlot = std::vector<int>(listIndexesToPlot.size(), 0);
                this->listLabelsToPlot = std::vector<std::string>(listLabelsToPlot.size(), "");
                this->listStylesToPlot = std::vector<std::string>(listStylesToPlot.size(), "");
                this->listColorsToPlot = std::vector<ImVec4>(listColorsToPlot.size(), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                this->listAnnotationsToPlot = std::vector<std::string>(listAnnotationsToPlot.size(), "");
                //this->TheTrainer->dataManager.experimentData.clear();
            }
        }
    }

    ImGui::End();
}

void TrainerApp::CreateBotModeWindow() {
    ImGui::Begin("Bot Mode");
    ImGui::Text("Select the bot mode");
    int oldBotModeSelected = TheTrainer->botManager.BotModeSelected;
    if (ImGui::RadioButton("Option 1 : gear low speed both sides", TheTrainer->botManager.BotModeSelected == 0))
        TheTrainer->botManager.BotModeSelected = 0;
    if (ImGui::RadioButton("Option 2 : gear high speed both sides", TheTrainer->botManager.BotModeSelected == 1))
        TheTrainer->botManager.BotModeSelected = 1;
    if (ImGui::RadioButton("Option 3 : gear low speed left side", TheTrainer->botManager.BotModeSelected == 2))
        TheTrainer->botManager.BotModeSelected = 2;
    if (ImGui::RadioButton("Option 4 : gear low speed right side", TheTrainer->botManager.BotModeSelected == 3))
        TheTrainer->botManager.BotModeSelected = 3;
    if (ImGui::RadioButton("Option 5 : gear high speed right side", TheTrainer->botManager.BotModeSelected == 4))
        TheTrainer->botManager.BotModeSelected = 4;
    if (ImGui::RadioButton("Option 6 : gear high speed left side", TheTrainer->botManager.BotModeSelected == 5))
        TheTrainer->botManager.BotModeSelected = 5;
    if (oldBotModeSelected != TheTrainer->botManager.BotModeSelected){
        TheTrainer->botManager.ChangeBotModeOption();
    }
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    static bool takeOffTheClutch = false;
    ImGui::Checkbox("Take off the clutch", &takeOffTheClutch);
    if (takeOffTheClutch) {
        TheTrainer->botManager.TakeOffTheClutch();
    }
    else {
        TheTrainer->botManager.PutOnTheClutch();
    }
    ImGui::End();
}

void TrainerApp::CreatePlotWindow() {
    ImGui::Begin("Output Data plot");
    ImGui::Checkbox("Show Regression equation", &ShowAnnotation);
    TrainerApp::SubplotItemSharing();
    ImGui::End();
}

void TrainerApp::CreateConsoleWindow() {
    ImGui::Begin("Robot output console");
    ImGui::Text("Output console");
    ImGui::End();
}

void TrainerApp::AddRobotConsoleError(const std::string& errorLabel) {
    time_t rawtime = time(nullptr);
    struct tm* timeinfo = localtime(&rawtime);
    char buffer[9];
    strftime(buffer, 9, "%H:%M:%S", timeinfo);
    std::string strTime(buffer);
    std::string errorLabelToShow = strTime + " " + errorLabel;

    if (listErrorsInConsole.size() + listMessagesInConsole.size() >= 20) {
        listErrorsInConsole.erase(listErrorsInConsole.begin());
    }

    for (auto it = listErrorsInConsole.begin(); it != listErrorsInConsole.end(); ) {
        if (it->find(errorLabel) != std::string::npos) {
            it = listErrorsInConsole.erase(it);
        }
        else {
            ++it;
        }
    }

    this->listErrorsInConsole.push_back(errorLabelToShow);
}

void TrainerApp::AddRobotConsoleMessage(const std::string& messageLabel) {
    time_t rawtime = time(nullptr);
    struct tm* timeinfo = localtime(&rawtime);
    char buffer[9];
    strftime(buffer, 9, "%H:%M:%S", timeinfo);
    std::string strTime(buffer);
    std::string messageLabelToShow = strTime + " " + messageLabel;

    if (listErrorsInConsole.size() + listMessagesInConsole.size() >= 20) {
        listMessagesInConsole.erase(listMessagesInConsole.begin());
    }
    for (auto it = listMessagesInConsole.begin(); it != listMessagesInConsole.end(); ) {
        if (it->find(messageLabel) != std::string::npos) {
            it = listMessagesInConsole.erase(it);
        }
        else {
            ++it;
        }
    }
    this->listMessagesInConsole.push_back(messageLabelToShow);
}

void TrainerApp::DeleteRobotConsoleError(const std::string& errorLabel) {
    for (auto it = listErrorsInConsole.begin(); it != listErrorsInConsole.end(); ) {
        if (it->find(errorLabel) != std::string::npos) {
            it = listErrorsInConsole.erase(it);
        }
        else {
            ++it;
        }
    }
}

void TrainerApp::DeleteRobotConsoleMessage(const std::string& messageLabel) {
    for (auto it = listMessagesInConsole.begin(); it != listMessagesInConsole.end(); ) {
        if (it->find(messageLabel) != std::string::npos) {
            it = listMessagesInConsole.erase(it);
        }
        else {
            ++it;
        }
    }
}

void TrainerApp::ShowMessageAndErrorInConsole() {
    for (const auto& error : this->listErrorsInConsole) {
        if (ImGui::Begin("Robot output console")) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color
            //ImGui::Selectable(error.c_str(), false);
            ImGui::Text("%s", error.c_str());
            if (ImGui::BeginPopupContextItem(error.c_str())) {
                if (ImGui::MenuItem("Copy")) {
                    ImGui::SetClipboardText(error.c_str());
                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(); // Reset to default color
            ImGui::End();
        }
    }
    for (const auto& message : this->listMessagesInConsole) {
        if (ImGui::Begin("Robot output console")) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // green color
            //ImGui::Selectable(message.c_str(), false);
            ImGui::Text("%s", message.c_str());
            if (ImGui::BeginPopupContextItem(message.c_str())) {
                if (ImGui::MenuItem("Copy")) {
                    ImGui::SetClipboardText(listAnnotationsToPlot[0].c_str());
                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(); // Reset to default color
            ImGui::End();
        }
    }

}

void TrainerApp::definePopupWindowRepMeasurement() {
    ImGui::SetNextWindowSize(ImVec2(400, 300));
    if (ImGui::BeginPopupModal("Initialize the rep measurements", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Please do a rep, I will analyze your movement.\n");
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Separator();
        //ImGui::Image(texture, ImVec2(PopUpWindowWidth, PopUpWindowHeight));
        //ImGui::Image((ImTextureID)my_texture_srv_gpu_handle.ptr, ImVec2((float)PopUpWindowWidth, (float)PopUpWindowHeight));
        if (TheTrainer->botManager.distancesMeasured.size() > 1) {
            ImGui::Text(("minimum height of the bar : " + std::to_string(std::min_element(TheTrainer->botManager.distancesMeasured.begin(), TheTrainer->botManager.distancesMeasured.end())[0])).c_str());
            ImGui::Text(("Current height of the bar : " + std::to_string(TheTrainer->botManager.distancesMeasured[TheTrainer->botManager.distancesMeasured.size() - 1])).c_str());
            ImGui::Text(("maximum height of the bar : " + std::to_string(std::max_element(TheTrainer->botManager.distancesMeasured.begin(), TheTrainer->botManager.distancesMeasured.end())[0])).c_str());
        }
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        if (ImGui::Button("I finished the rep")) {
            TheTrainer->botManager.WantRecordDistances = false;
            TheTrainer->botManager.distancesMeasured.clear();
            this->TheTrainer->WantToStartBot = false;
        }
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            TheTrainer->botManager.WantRecordDistances = false; 
            TheTrainer->botManager.distancesMeasured.clear();
            this->TheTrainer->WantToStartBot = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void TrainerApp::definePopupWindowOneRMExperiment() {
    ImGui::SetNextWindowSize(ImVec2(400, 300));
   
    if (ImGui::BeginPopupModal("One RM Experiment", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
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

void TrainerApp::RenderUI()
{
    MakeDockingPossible();

    CreateUsersInfoWindow();
    CreateBotModeWindow();
    CreatePlotWindow();
    CreateConsoleWindow();

    ShowMessageAndErrorInConsole();

    ImGui::End();
}
