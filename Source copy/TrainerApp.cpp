#include "TrainerApp.h"
#include "Trainer.h"
//#include "TrainerAnalyser.h"
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

using namespace std;

TrainerApp::TrainerApp(Trainer* trainer) : trapeze(this) {
    this->TheTrainer = trainer;
    this->listOfPlotsToKeep = std::vector<bool>(nbreOfDataToPlot, true); // We first want to forget nobody
    this->listXToPlot = std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0));
    this->listYToPlot = std::vector<std::vector<double>>(nbreOfDataToPlot, std::vector<double>(3, 0.0));
    this->listIndexesToPlot = std::vector<double>(nbreOfDataToPlot, 0);
    this->listLabelsToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->listStylesToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->listColorsToPlot = std::vector<ImVec4>(nbreOfDataToPlot, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    this->listAnnotationsToPlot = std::vector<std::string>(nbreOfDataToPlot, "");
    this->mousePositionAtRightClick = std::vector<double>(2, 0.0);
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

int TrainerApp::FindPointFocusedOn(int j, vector <double> mousePosition) {
    cout << mousePosition[0] << " " << mousePosition[1] << endl;
    for (double val : listYToPlot[j]) {
        if (abs(val - mousePosition[1]) < 0.1) {
			return std::distance(listYToPlot[j].begin(), std::find(listYToPlot[j].begin(), listYToPlot[j].end(), val));
		}
    };
    return -1;
}

std::vector <double> GetClickedPlotPoint(const std::vector<std::vector<double>>& xValueslists, const std::vector<std::vector<double>>& yValueslists, double indexOfPlot, vector<double> listIndexesToPlot) {
    ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
    double minDistance = 200000;
    std::vector <double> closestPointIndex = { -1, -1 };

    for (size_t j = 0; j < yValueslists.size(); ++j){
        if (listIndexesToPlot[j] == indexOfPlot) {
            for (size_t i = 0; i < yValueslists[j].size(); ++i) {
                double distance = std::sqrt(std::pow(mousePos.x - xValueslists[j][i], 2) + std::pow(mousePos.y - yValueslists[j][i], 2));
                if (distance < minDistance) {
                    minDistance = distance;
                    closestPointIndex = { (double)j, (double)i };
                }
            }
        }
    }
    return closestPointIndex;
}

void TrainerApp::CompareLastSetWithOneRM() {
    OneRMAnalyser* oneRMAnalyser = dynamic_cast<OneRMAnalyser*>(TheTrainer->predictor.sessionAnalysers[TheTrainer->predictor.FindLastAnalyserOfType<OneRMAnalyser>()].get());
    SetDataSaver* lastSet = dynamic_cast<SetDataSaver*>(TheTrainer->predictor.sessionAnalysers[TheTrainer->predictor.FindLastAnalyserOfType<SetDataSaver>()].get());
    
    if (lastSet->closed){
        //Plot the set in the OneRM graph to compare it with the OneRM value
        std::vector<double> XValuesInOneRMPlot(lastSet->maxVelocities.size(), lastSet->TotalWeightWanted);
        int index = TheTrainer->app.FindIndexOfEmptyPlotForLabel("Compare set with One RM");
        TheTrainer->app.Plot(index, XValuesInOneRMPlot, std::vector<double>{lastSet->maxVelocities}, 1, "Compare set with One RM", "scatter", "", false, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    
        //Plot the set in the Endurance graph to compare it with the endurance value
        std::vector<double> XValuesInEndurancePlot;
        std::vector<double> YValuesInEndurancePlot;
        for (int i = 0; i < lastSet->maxVelocities.size(); i++) {
            XValuesInEndurancePlot.push_back(100 * (lastSet->maxMaxVelocities - lastSet->maxVelocities[i]) / lastSet->maxMaxVelocities); // loss of velocity
            YValuesInEndurancePlot.push_back(100 * (i + 1) / lastSet->maxVelocities.size());        // percentage of reps done  
        }
        index = TheTrainer->app.FindIndexOfEmptyPlotForLabel("Compare set with Endurance");
        TheTrainer->app.Plot(index, XValuesInEndurancePlot, YValuesInEndurancePlot, 1.1, "Compare set with Endurance", "scatter", "", false, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

        //Print the results in the console
        TheTrainer->app.DeleteRobotConsoleMessage("The maximum velocity of last set is ");
        TheTrainer->app.DeleteRobotConsoleMessage("The load lifted is ");
        TheTrainer->app.AddRobotConsoleMessage(("The minimum velocity of last set is " + std::to_string(lastSet->minMaxVelocities) + ". The difference with the One RM velocity value is : " + std::to_string(lastSet->minMaxVelocities - oneRMAnalyser->OneRMVelocityValue)).c_str());
        TheTrainer->app.AddRobotConsoleMessage(("The load lifted is " + std::to_string(lastSet->TotalWeightWanted * 100 / oneRMAnalyser->OneRMValue) + " % of One RM load").c_str());
        TheTrainer->app.AddRobotConsoleMessage(("The load lifted is " + std::to_string(lastSet->TotalWeightWanted * 100 / oneRMAnalyser->OneRMValue) + " % of One RM load").c_str());
    }
}

int TrainerApp::FindIndexOfEmptyPlotForLabel(string label) {
    int index = -1;
    //If the plot for this regression already exists, we update it 
    for (int j = 0; j < TheTrainer->app.listLabelsToPlot.size(); ++j) {
        if (TheTrainer->app.listLabelsToPlot[j].find(label) != std::string::npos) {
            index = j;
            break;
        }
    }
    //If it does not exist, we create a new one at the first empty index
    if (index == -1) {
        for (int i = 0; i < this->listLabelsToPlot.size(); ++i) {
            if (this->listLabelsToPlot[i] == "") {
                index = i;
                break;
            }
        }
    }
    return index;
}

void TrainerApp::Plot(int i, std::vector<double>& x, std::vector<double>& y, double indexOfWindowWhereItPlots, std::string label, std::string style, std::string annotation, bool shallWeKeepThePlotEvenAfterAClearPlot, ImVec4 color) {
    //Each time this function is called, the previous plot is crushed and replaced by the new one
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
    listLinesToPlot.clear();
}

void TrainerApp::RenderPointsAndLineInPlot(double i) {
    double x_min = 100000, x_max = -100000, y_min = 100000, y_max = -100000;
    for (Line line : listLinesToPlot) {
        if (line.index == i) {
            if (dynamic_cast<HorizontalLine*>(&line)) { dynamic_cast<HorizontalLine*>(&line)->update(); }
            if (dynamic_cast<VerticalLine*>(&line)) { dynamic_cast<VerticalLine*>(&line)->update(); }
            double x_points[2] = { line.pointA[0], line.pointB[0] };
            double y_points[2] = { line.pointA[1], line.pointB[1] };
            ImPlot::PlotLine("", x_points, y_points, 2);
        }
    }
    //Single Points to plot (For example the lower and upper bounds of the 1RM experiment)
    for (auto it = listPointsToPlot.begin(); it != listPointsToPlot.end(); ) {
        auto& point = *it;
        if (point.index == i) {
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, point.color);
            ImPlot::PlotScatter("", &point.x, &point.y, 1);
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup("Delete Point");
            }
            if (ImGui::BeginPopup("Delete Point")) {
                if (ImGui::MenuItem("Delete")) {
                    it = listPointsToPlot.erase(it);
                    ImGui::CloseCurrentPopup();
                    continue;
                }
                ImGui::EndPopup();
            }
            if (point.type != "point" && point.type != "maxVelocity" &&
                point.color.x == 1.0f && point.color.y == 0.0f && point.color.z == 0.0f && point.color.w == 1.0f) {
                //We draw a vertical line where the point is 
                for (int j = 0; j < listXToPlot.size(); ++j) {
                    for (int k = 0; k < listXToPlot[j].size(); ++k) {
                        y_min = (((y_min) < (listYToPlot[j][k])) ? (y_min) : (listYToPlot[j][k]));
                        y_max = (((y_max) > (listYToPlot[j][k])) ? (y_max) : (listYToPlot[j][k]));
                    }
                }
                double x_points[2] = { point.x, point.x };
                double y_points[2] = { y_min, y_max };
                ImPlot::PlotLine("", x_points, y_points, 2);
            }
            else if (point.type == "point") {
                //TODO cout << "Faire une ligne horizontale" << endl;
            }
            ImPlot::PopStyleColor();
        }
        ++it;
    }

}

void TrainerApp::RenderDataInPlot(double i) {
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
            if (!(listColorsToPlot[j].x == 0.0f && listColorsToPlot[j].y == 0.0f && listColorsToPlot[j].z == 0.0f && listColorsToPlot[j].w == 0.0f)) {
                ImPlot::PopStyleColor();
            }
        }
        //Right click on graph to do the regression
       /* if (std::find(TheTrainer->predictor.indexesOfDataToDoRegressionOn.begin(), TheTrainer->predictor.indexesOfDataToDoRegressionOn.end(), j - 1) != TheTrainer->predictor.indexesOfDataToDoRegressionOn.end()) {
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
        }*/
    }

}

void TrainerApp::SetUpQueryInPlot() {
    //fully aesthetic function to Set Up Query (= a rectangle to zoom in a specific area of the plot)
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
}

void TrainerApp::DrawResizableSeparator(int i) {
    //fully aesthetic function to draw a separator between subplots and enable the user to resize them
    ImGui::PushID("separator_"+i);
    ImVec2 separatorPosition = ImGui::GetCursorScreenPos();
    ImGui::Separator(); // Draw a horizontal line
    ImGui::InvisibleButton("separator", ImVec2(-1, 4.0));
    if (ImGui::IsItemActive()) {
        subplotHeights[i] += ImGui::GetIO().MouseDelta.y; // Change the height of the subplot according to the mouse movement
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); // Change cursor to resize arrow
    }
    ImGui::PopID();
    ImVec2 mousePos = ImGui::GetMousePos();
    if (mousePos.x >= separatorPosition.x && mousePos.x <= separatorPosition.x + ImGui::GetContentRegionAvail().x &&
        mousePos.y >= separatorPosition.y && mousePos.y <= separatorPosition.y + ImGui::GetTextLineHeight()) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 separatorEndPos = ImVec2(separatorPosition.x + ImGui::GetContentRegionAvail().x, separatorPosition.y);
        draw_list->AddLine(separatorPosition, separatorEndPos, IM_COL32(200, 200, 200, 255), 2.0f); // Light gray color, 2.0f thickness
    }
}

void TrainerApp::AllowDeletingPointByRightClickingOnIt(int index) {
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        mousePositionAtRightClick = { ImPlot::GetPlotMousePos().x, ImPlot::GetPlotMousePos().y };
        for (int i = 0; i < listYToPlot.size(); ++i) {
            if (index == listIndexesToPlot[i]) {
                for (int j = 0; j < listYToPlot[i].size();) {
                    if (j < listYToPlot[i].size()){
                        double x = listXToPlot[i][j];
                        double y = listYToPlot[i][j];
                        ImPlotRect limits = ImPlot::GetPlotLimits();
                        if (abs(y - mousePositionAtRightClick[1]) < 0.1*(limits.Y.Max - limits.Y.Min) && abs(x - mousePositionAtRightClick[0]) < 0.1 * (limits.X.Max - limits.X.Min)) {
                            j_toRemember = j;
                            i_toRemember = i;
                            ImGui::OpenPopup("Delete Point");
                            break;
                        }
                    }
                    else {
                        break;
                    }
                    ++j;
                }
            }
        }
    }
    if (ImGui::BeginPopup("Delete Point")) {
        if (index == 1){
            if (ImGui::MenuItem("Delete")){
                auto it = TheTrainer->predictor.FindLastAnalyserOfType<OneRMAnalyser>();
                if (it != -1) {
                    OneRMAnalyser* oneRMAnalyser = dynamic_cast<OneRMAnalyser*>(TheTrainer->predictor.sessionAnalysers[it].get());
                    std::ostringstream oss;
                    oss << "deleted OneRM point {"
                        << std::fixed << std::setprecision(5)
                        << it << ", "
                        << j_toRemember << ", "
                        << oneRMAnalyser->OneRMExperimentLoads[j_toRemember] << ", "
                        << oneRMAnalyser->OneRMExperimentVelocities[j_toRemember] << ", "
                        << oneRMAnalyser->EnduranceExperimentVelocitiesLoss[j_toRemember] << ", "
                        << oneRMAnalyser->EnduranceExperimentRepetitionRatio[j_toRemember] << "}";

                    LastActionDone.push_back(oss.str());
                    oneRMAnalyser->OneRMExperimentLoads.erase(oneRMAnalyser->OneRMExperimentLoads.begin() + j_toRemember);
                    oneRMAnalyser->OneRMExperimentVelocities.erase(oneRMAnalyser->OneRMExperimentVelocities.begin() + j_toRemember);
                    oneRMAnalyser->EnduranceExperimentVelocitiesLoss.erase(oneRMAnalyser->EnduranceExperimentVelocitiesLoss.begin() + j_toRemember);
                    oneRMAnalyser->EnduranceExperimentRepetitionRatio.erase(oneRMAnalyser->EnduranceExperimentRepetitionRatio.begin() + j_toRemember);
                    oneRMAnalyser->UpdateAndPlotOneRMExperiment();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        else {
            //create a function to delete the point (I didn't need this functionality except for the One RM velocities/load graph)
        }
    }
}

void TrainerApp::SubplotItemSharing() {
    ImPlotFlags plot_flags = 0;
    if (!showLegends) { plot_flags |= ImPlotFlags_NoLegend; }

    if (ImPlot::BeginSubplots("##First row - Data", 1, 1, ImVec2(-1, subplotHeights[0]))) {
        
        ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Sort | ImPlotLegendFlags_Horizontal);
        int i = 0;
        //Plot the raw data from the Bot 
        if (ImPlot::BeginPlot("Data", "Time in seconds", "", ImVec2(-1, 0), plot_flags)) {
                  
            // Set the x range to [x1, x2] and the y range to [y1, y2]
            ImPlot::SetupAxesLimits(-15, 60, -0.5, 0.5);
            RenderPointsAndLineInPlot(i);
            RenderDataInPlot(i);
            SetUpQueryInPlot();

            //Create a Point on mouse position Ctrl+click
            if (ImGui::GetIO().KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !HasAlreadyClickedOnPlot) {
                bool lowerBoundPointIsDefined = std::any_of(listPointsToPlot.begin(), listPointsToPlot.end(), [](const auto& point) { return point.type == "lowerBound"; }); //Check if a lowerBound point is already defined
                if (!lowerBoundPointIsDefined) {
                    cout << "Create a lowerBound point" << endl;
                    minBound = ImPlot::GetPlotMousePos().x;
                    listPointsToPlot.push_back(Point(this, std::vector<double>{ImPlot::GetPlotMousePos().x, 0.0}, "lowerBound", 0));
                    HasAlreadyClickedOnPlot = true;
                    lowerBoundPointIsDefined = true;
                    LastActionDone.push_back("add lowerBound");
                }
                else{
                    if (ImPlot::GetPlotMousePos().x > listPointsToPlot.back().x) {
                        cout << "Create a upperBound point" << endl;
                        maxBound = ImPlot::GetPlotMousePos().x;
                        listPointsToPlot.push_back(Point(this, std::vector<double>{ImPlot::GetPlotMousePos().x, 0.0}, "upperBound", 0));
                        HasAlreadyClickedOnPlot = true;
                        lowerBoundPointIsDefined = false;
                        LastActionDone.push_back("add upperBound");
                    }
                    else {
                        cout << "Create a lowerBound point" << endl;
                        listPointsToPlot.erase(std::remove_if(listPointsToPlot.begin(), listPointsToPlot.end(), [](const auto& point) { return point.type == "upperBound" || point.type == "lowerBound"; }), listPointsToPlot.end()); //Remove all points used to make manual boundaries
                        minBound = ImPlot::GetPlotMousePos().x;
                        maxBound = 1000000;
                        listPointsToPlot.push_back(Point(this, std::vector<double>{ImPlot::GetPlotMousePos().x, 0.0}, "lowerBound", 0));
                        HasAlreadyClickedOnPlot = true;
                        lowerBoundPointIsDefined = true;
                        LastActionDone.push_back("add lowerBound");
                    }
                }
            }
            //END Create a Point on mouse position Ctrl+click
            ImPlot::EndPlot();
        }
        HasAlreadyClickedOnPlot = false;

        ImPlot::EndSubplots();
    }
    DrawResizableSeparator(0);

    if (TheTrainer->predictor.FindLastAnalyserOfType<OneRMAnalyser>() != -1){
        if (ImPlot::BeginSubplots("##Second Row One RM experiment", 1, 2, ImVec2(-1, subplotHeights[1]))) { // Adjust size as needed
            // First plot in the second row
            if (ImPlot::BeginPlot("One RM", "Load in kg", "Velocity", ImVec2(-1, 180), plot_flags)) { // Adjust size as needed
                ImPlot::SetupAxesLimits(-5, 60, -0.005, 0.3);
                RenderPointsAndLineInPlot(1);
                RenderDataInPlot(1);
                SetUpQueryInPlot();
                AllowDeletingPointByRightClickingOnIt(1);
                ImPlot::EndPlot();
            }
            // Second plot in the second row
            if (ImPlot::BeginPlot("Endurance", "Velocity loss in %", "Repetition in %", ImVec2(-1, 180))) { // Adjust size as needed
                ImPlot::SetupAxesLimits(-10, 100, -10, 110);
                RenderPointsAndLineInPlot(1.1);
                RenderDataInPlot(1.1);
                SetUpQueryInPlot();
                ImPlot::EndPlot();
            }
            ImPlot::EndSubplots();
        }
        DrawResizableSeparator(1);
    }

    if (TheTrainer->UsingLoadProfile) {
        if (ImPlot::BeginPlot("Profile", "Length (cm)", "Weight (kg)", ImVec2(-1, subplotHeights[2]))) {
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
        DrawResizableSeparator(2);
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
    float tabWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(tabWidth * 0.5f);
    char usernameBuffer[256] = "";
    strncpy_s(usernameBuffer, TheTrainer->Username.c_str(), sizeof(usernameBuffer) - 1);
    if (ImGui::InputText("Username", usernameBuffer, IM_ARRAYSIZE(usernameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) { // This block is executed when the user presses Enter while the input field is active
        TheTrainer->Username = std::string(usernameBuffer);
    }
    ImGui::SetNextItemWidth(tabWidth * 0.5f);
    char typeOfExercise[256] = "";
    strncpy_s(typeOfExercise, TheTrainer->TypeOfExercise.c_str(), sizeof(typeOfExercise) - 1);
    if (ImGui::InputText("Type of exercise", typeOfExercise, IM_ARRAYSIZE(typeOfExercise), ImGuiInputTextFlags_EnterReturnsTrue)) { // This block is executed when the user presses Enter while the input field is active
        TheTrainer->TypeOfExercise = std::string(typeOfExercise);
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    char showRecordLabel[256];
    snprintf(showRecordLabel, sizeof(showRecordLabel), "Show %s's records", TheTrainer->Username.c_str());
    if (ImGui::Button(showRecordLabel)) {
        ImGui::OpenPopup("Calendar");
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    TheTrainer->dataRenderer.defineCalendarPopup();
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
            double acquiredMaxHeight = TheTrainer->predictor.sessionAnalysers[0].get()->getReferencedMaxHeight();
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
        ImGui::SameLine();
        std::string text = "";
        if (TheTrainer->predictor.DoingOneRMExperiment) {
            text += "\nDoing One RM experiment";
        }
        if (TheTrainer->predictor.DoingLepleyExperiment) {
            text += "\nDoing Lepley experiment";
        }
        text.erase(text.begin());
        ImGui::Text(text.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("Close Experiment Menu");
        }
        if (ImGui::BeginPopup("Close Experiment Menu")) {
            if (TheTrainer->predictor.DoingOneRMExperiment) {
                if (ImGui::MenuItem("Stop One RM Experiment")) {
                    TheTrainer->predictor.WantStopOneRMExperiment = true;
                }
            }
            if (TheTrainer->predictor.DoingLepleyExperiment) {
                if (ImGui::MenuItem("Stop Lepley Experiment")) {
                    TheTrainer->predictor.WantStopLepleyExperiment = true;
                }
            }
            ImGui::EndPopup();
        }
        /*if (!this->TheTrainer->predictor.WantToAnalyzeSeriesForOneRM){
            ImGui::SameLine();
            if (ImGui::Button("Analyze past series for One RM experiment")) {
                this->TheTrainer->predictor.WantToAnalyzeSeriesForOneRM = true;
                this->TheTrainer->UsingLoadProfile = false;
                cout << "WANT TO Analyze repetitions" << endl;
            }
        }*/
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        if (ImGui::Button("Start experiment")) {
            ImGui::OpenPopup("Menu");
        }
        bool WantOpenstepsMethod = false;
        bool WantOpenEpley = false;
        if (ImGui::BeginPopup("Menu")) {
            if (ImGui::MenuItem("One RM experiment by 3 steps method")) {
                WantOpenstepsMethod = true;
            }
            if (ImGui::MenuItem("One RM experiment by Epley method")) {
                WantOpenEpley = true;
            }
            ImGui::EndPopup();
        }
        if (WantOpenEpley) {
            ImGui::OpenPopup("One RM by Epley method");
            WantOpenEpley = false;
        }
        if (WantOpenstepsMethod) {
            ImGui::OpenPopup("One RM by 3 steps method");
            WantOpenstepsMethod = false;
        }
        DefinePopupWindowOneRM3StepsExplanation();
        //DefinePopupWindowOneRMEpley();
        DefinePopupWindowOneRMEpleyExplanation();
    }
    if (this->TheTrainer->BotWorkingInProgress) {
        if (ImGui::Button("Stop Bot")) {
            this->TheTrainer->WantToStopBot = true;
        }
        ImGui::SameLine();
        std::string text = "";
        if (TheTrainer->predictor.DoingOneRMExperiment) {
            text += "\nDoing One RM experiment";
        }
        if (TheTrainer->predictor.DoingLepleyExperiment) {
            text += "\nDoing Lepley experiment";
        }
        text.erase(text.begin());
        ImGui::Text(text.c_str());

        /*if (!this->TheTrainer->predictor.CountingNumberOfReps){
            if (ImGui::Button("One RM by Epley")) {
                this->TheTrainer->UsingLoadProfile = false;
                listPointsToPlot.push_back(Point(this, std::vector<double>{listXToPlot[36 - 1].back(), 0.0}, "startOfEpleyExperiment", 0));
                ImGui::OpenPopup("One RM Lander and Epley Experiment");
            }
        }
        else {}//No need for a button to stop because the button "Ok" of the popup will stop the experiment
        DefinePopupWindowOneRMEpley();
        */
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
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    if(ImGui::Checkbox("Take off the clutch", &TheTrainer->botManager.WantTakeOffTheClutch)){
        if (TheTrainer->botManager.WantTakeOffTheClutch) {
            TheTrainer->botManager.TakeOffTheClutch();
        }
        else {
            TheTrainer->botManager.PutOnTheClutch();
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Checkbox("Simulation mode", &TheTrainer->simulationMode);
    TheTrainer->AquisitionFrequency = (TheTrainer->simulationMode) ? 0.03 : 0.1;
    ImGui::End();
}

void TrainerApp::CreatePlotWindow() {
    if (ImGui::Begin("Output Data plot")) {
        float tabWidth = ImGui::GetContentRegionAvail().x;
        ImGui::Checkbox("Show Legends", &showLegends);
        ImGui::SameLine();
        ImGui::Checkbox("Show Regression equation", &ShowAnnotation);
        ImGui::SameLine();
        if (ImGui::Button("Reset Profile")) { trapeze.Begin(); }
        if (TheTrainer->predictor.FindLastAnalyserOfType<OneRMAnalyser>() != -1) {
			ImGui::SameLine();
            ImGui::Checkbox("Plot last set in One RM experiment", &PlotLastSetInOneRMGraphToCompareThem);
            if (PlotLastSetInOneRMGraphToCompareThem) { CompareLastSetWithOneRM(); }
            else { Unplot("Compare set with One RM"); Unplot("Compare set with Endurance"); }
		}
        if (TheTrainer->predictor.MaxNumberRepsEstimated != -1){  // progress bar of the set 
            //ImGui::Dummy(ImVec2(5.0f, 0.0f));
            ImGui::Text("Progress of the set : ");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 0.5f, 1.0f, 1.0f)); // Blue color
            char overlay[32];
            //snprintf(overlay, sizeof(overlay), "Progress of the set (%.0f %%)", TheTrainer->predictor.PercentageOfSet);
            //ImGui::ProgressBar(static_cast<float>(TheTrainer->predictor.PercentageOfSet) / 100.0f, ImVec2(0.0f, 0.0f), overlay);
            snprintf(overlay, sizeof(overlay), "%d repetitions done", int(TheTrainer->predictor.CurrentNumberOfRepsDoneInTheSet));
            ImGui::ProgressBar(TheTrainer->predictor.CurrentNumberOfRepsDoneInTheSet / TheTrainer->predictor.MaxNumberRepsEstimated, ImVec2(tabWidth*0.3, 0.0f), overlay);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("Out of a maximum estimated at %d repetitions", int(TheTrainer->predictor.MaxNumberRepsEstimated));
            if (TheTrainer->predictor.FindLastAnalyserOfType<SetDataSaver>() != -1) {
                SetDataSaver* setSaver = dynamic_cast<SetDataSaver*>(TheTrainer->predictor.sessionAnalysers[TheTrainer->predictor.FindLastAnalyserOfType<SetDataSaver>()].get());
                if (int(TheTrainer->predictor.personnalRecordOfNumberOfRepsAtCurrentLoad) != 0) {
                    ImGui::SameLine();
                    ImGui::Text("(Personnal record : %d)", int(TheTrainer->predictor.personnalRecordOfNumberOfRepsAtCurrentLoad));
                }
			}
        }
        SubplotItemSharing();
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            for (Line& line : listLinesToPlot) {
                if (dynamic_cast<HorizontalLine*>(&line)) { dynamic_cast<HorizontalLine*>(&line)->update(); }
                if (dynamic_cast<VerticalLine*>(&line)) { dynamic_cast<VerticalLine*>(&line)->update(); }
            }
        }
    }
    ImGui::End();
}

void TrainerApp::CreateConsoleWindow() {
    ImGui::Begin("Robot output console");
    ImGui::Text("Output console");
    ImGui::End();
}

std::vector<ImVec2> TrainerApp::GetPopupPosition(int width, int height) {
    RECT rect;
    GetWindowRect(TheTrainer->hwnd, &rect);
    int mainWindowWidth = rect.right - rect.left;
    int mainWindowHeight = rect.bottom - rect.top;

    // Define the size of the popup
    ImVec2 popupSize = ImVec2(width, height); // Example size of the popup

    // Calculate the center position for the popup
    ImVec2 popupPos = ImVec2(
        rect.left + (mainWindowWidth - popupSize.x) * 0.5f,
        rect.top + (mainWindowHeight - popupSize.y) * 0.5f
    );
    return { popupPos, popupSize };
}

void TrainerApp::DefineFirstPopup() {
    std::vector<ImVec2> result = GetPopupPosition(410, 300);
    ImGui::SetNextWindowPos(result[0], ImGuiCond_Always);
    ImGui::SetNextWindowSize(result[1]);
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
        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
            ShowFirstPopup = false;
        }
        ImGui::EndPopup();
    }
}

void TrainerApp::DefineLastPopup() {
    std::vector<ImVec2> result = GetPopupPosition(410, 300);
    ImGui::SetNextWindowPos(result[0], ImGuiCond_Always);
    ImGui::SetNextWindowSize(result[1]);
    // Calculate the center position for the text
    std::string tempText1 = TheTrainer->Username + ",";
    const char* text1 = tempText1.c_str();
    const char* text2 = "Congrats for your training.";
    const char* text3 = "Do you want me to save your session in the data base ?";
    ImVec2 textSize1 = ImGui::CalcTextSize(text1);
    ImVec2 textSize2 = ImGui::CalcTextSize(text2);
    ImVec2 textSize3 = ImGui::CalcTextSize(text3);
    if (ImGui::BeginPopupModal("Saving in Data Base", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((410 - textSize1.x) * 0.5f); // to center the text
        ImGui::Text(text1);
        ImGui::SetCursorPosX((410 - textSize2.x) * 0.5f); // to center the text
        ImGui::Text(text2);
        ImGui::SetCursorPosX((410 - textSize3.x) * 0.5f); // to center the text
        ImGui::Text(text3);
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((410 - 2 * 120) * 0.5f); // to center the button
        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            TheTrainer->dataManager.SaveSessionInDataBase();
            ImGui::CloseCurrentPopup();
            ::PostQuitMessage(0);
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
            ::PostQuitMessage(0);
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

void TrainerApp::DefinePopupWindowOneRMEpleyExplanation() {
    std::vector<ImVec2> result = TheTrainer->app.GetPopupPosition(450, 300);
    ImGui::SetNextWindowPos(result[0], ImGuiCond_Always);
    ImGui::SetNextWindowSize(result[1]);
	if (ImGui::BeginPopupModal("One RM by Epley method", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The Epley method is a quick way to estimate your One RM \nbut it is not accurate at 100%.");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
		ImGui::Text("   Start the motors and hit the button 'One RM by Epley'");
		ImGui::Text("   Do as many reps as possible at a given weight");
        ImGui::Text("   Stop the motors");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Text("The results of the calculation appear in the console");
		ImGui::Text("Make sure to be exhausted at the end of the set.");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
		ImGui::SetCursorPosX((450 - 120) * 0.5f); // to center the button
		if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
			ImGui::CloseCurrentPopup();
            if (!TheTrainer->predictor.DoingLepleyExperiment) { // If we are not already doing an experiment, we can start one
                TheTrainer->predictor.WantStartLepleyExperiment = true;
            }
		}
		ImGui::EndPopup();
    }
}

void TrainerApp::DefinePopupWindowOneRM3StepsExplanation(){
    std::vector<ImVec2> result = GetPopupPosition(700, 300);
    ImGui::SetNextWindowPos(result[0], ImGuiCond_Always);
    ImGui::SetNextWindowSize(result[1]);
	if (ImGui::BeginPopupModal("One RM by 3 steps method", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("The 3 steps method is a precise way to estimate your One RM");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
		ImGui::Text("   1.  Do as many reps as possible at 40%% of your known One RM or at 40%% of your weight");
        ImGui::Text("   2.  Do as many reps as possible at 60%% of your known One RM or at 60%% of your weight");
        ImGui::Text("   3.  Do as many reps as possible at 80%% of your known One RM or at 80%% of your weight");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Text("Make sure to be exhausted at the end of every set.\nOn the first graph you have your data, \non the second, the maximum velocity of each repetition");
        ImGui::Text("You can delete a point at anytime on the second graph if you feel that it is not representative");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((700 - 120) * 0.5f); // to center the button
        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
            if (!TheTrainer->predictor.DoingOneRMExperiment) { // If we are not already doing an experiment, we can start one
                TheTrainer->predictor.WantStartOneRMExperiment = true;
            }
        }
        ImGui::EndPopup();
	}

}

void TrainerApp::DefinePopupWindowOneRM3StepsIntermediateWindow() {
    TheTrainer->predictor.DefinePopupWindowOneRM3StepsIntermediateWindow();
}

void TrainerApp::AddHorizontalLineToPlot(double yValue, std::string type, int index) {
    listLinesToPlot.push_back(HorizontalLine(this, yValue, type, index));
}

void TrainerApp::AddVerticalLineToPlot(double xValue, std::string type, int index) {
    listLinesToPlot.push_back(VerticalLine(this, xValue, type, index));
}

void TrainerApp::AddPointToPlot(vector<double> coordinates, std::string label, ImVec4 color) {
    listPointsToPlot.push_back(Point(this, coordinates, label, 0, color));
}

void TrainerApp::DeletePointToPlot(vector<double> coordinates) {
    for (int k = listPointsToPlot.size() - 1; k >= 0; --k) {
		if (listPointsToPlot[k].x == coordinates[0] && listPointsToPlot[k].y == coordinates[1]) {
			listPointsToPlot.erase(listPointsToPlot.begin() + k);
			break;
		}
    }
}

void TrainerApp::ChangePointToPlotColor(vector<double> coordinates, ImVec4 color) {
    for (int k = listPointsToPlot.size() - 1; k >= 0; --k) {
        if (listPointsToPlot[k].x == coordinates[0] && listPointsToPlot[k].y == coordinates[1]) {
            listPointsToPlot[k].color = color;
            break;
        }
    }
}

void TrainerApp::UndoLastAction(){
    if (LastActionDone.size() > 0){
        std::string action = LastActionDone[LastActionDone.size() - 1];
        if (action.find("add lowerBound") != std::string::npos || action.find("add upperBound") != std::string::npos) { //If the last action done is to add a bound (lower or upper), we delete that last bound
	        if (listPointsToPlot.size() > 0) {
                for (int k = listPointsToPlot.size() - 1; k >= 0; --k) {
                    if (listPointsToPlot[k].type == "lowerBound" || listPointsToPlot[k].type == "upperBound") {
		                listPointsToPlot.erase(listPointsToPlot.begin() + k);
                        break;
                    }
                }
	        }
        }
        if (action.find("deleted OneRM point") != std::string::npos) { //If the last action done is to delete a point in One RM experiment, we add it back to the experiment
            //Let's extract all the values saved in the action string 
            std::string prefix = "deleted OneRM point {";   //Start of the values list
            std::string suffix = "}";                       //End of the values list    
            std::vector<double> values;
            // Check if the string starts with the prefix and ends with the suffix
            if (action.find(prefix) == 0 && action.rfind(suffix) == action.size() - suffix.size()) {
                // Extract the part of the string that contains the values
                std::string valuesStr = action.substr(prefix.size(), action.size() - prefix.size() - suffix.size());

                // Use istringstream to parse the values
                std::istringstream iss(valuesStr);
                std::string value;
                while (std::getline(iss, value, ',')) {
                    values.push_back(std::stod(value));
                }
            }
            //Now let's put back those values in the experiment 
            int indexOfOneRMAnalyser = (int)values[0];
            int j_toInsert = (int)values[1];
		    OneRMAnalyser* oneRMAnalyser = dynamic_cast<OneRMAnalyser*>(TheTrainer->predictor.sessionAnalysers[indexOfOneRMAnalyser].get());
            oneRMAnalyser->OneRMExperimentLoads.insert(oneRMAnalyser->OneRMExperimentLoads.begin() + j_toInsert, values[2]);
            oneRMAnalyser->OneRMExperimentVelocities.insert(oneRMAnalyser->OneRMExperimentVelocities.begin() + j_toInsert, values[3]);
            oneRMAnalyser->EnduranceExperimentVelocitiesLoss.insert(oneRMAnalyser->EnduranceExperimentVelocitiesLoss.begin() + j_toInsert, values[4]);
            oneRMAnalyser->EnduranceExperimentRepetitionRatio.insert(oneRMAnalyser->EnduranceExperimentRepetitionRatio.begin() + j_toInsert, values[5]);
            oneRMAnalyser->UpdateAndPlotOneRMExperiment();
	    }
        LastActionDone.pop_back();
    }
}

//void TrainerApp::RenderPlotInPopup(std::vector<SetDataSaver> setsDataSaversToShow) {
//    //Used by the calendar to show the plot of the sets in a previous day
//    if (ImPlot::BeginPlot("Velocities of Set")) {
//        for (int i = 0; i < setsDataSaversToShow.size(); ++i) {
//            ImPlot::PlotLine(("Set " + std::to_string(i + 1)).c_str(), setsDataSaversToShow[i].VelocityOfSet.data(), setsDataSaversToShow[i].VelocityOfSet.size());
//        }
//        ImPlot::EndPlot();
//    }
//}

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

    if (TheTrainer->WantWindowClosed) {
        ImGui::OpenPopup("Saving in Data Base");
    }
    DefineLastPopup();
    ImGui::End();
}
