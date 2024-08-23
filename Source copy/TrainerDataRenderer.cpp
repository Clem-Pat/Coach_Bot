/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org
 * Please refer to the Datasheet to know more about the application
 */


#include "TrainerDataRenderer.h"
#include "Trainer.h"
#include "imgui_internal.h"
#include <sqlite3.h>
#include <ctime>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <implot/implot.h>
#include <ctime>    // to know the first day of the month. useful in the calendar

int getFirstDayOfWeek(int year, int month) {
    // Function to calculate the day of the week for the first day of the month. Useful to place the day button in the right place in the calendar
    std::tm time_in = { 0, 0, 0, 1, month - 1, year - 1900 }; // 1st day of the month
    std::time_t time_temp = std::mktime(&time_in);
    const std::tm* time_out = std::localtime(&time_temp);
    return time_out->tm_wday; // Day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
}

std::vector<double> TrainerDataRenderer::getMonthActivity(int month, int year) {
    //Just a function to get the number of the days with workouts
    //Useful to put green the days with workouts in the calendar

    sqlite3* db = TheTrainer->dataManager.db;
    std::vector<double> daysWithWorkouts;

    // Prepare the SQL query
    std::string sql = "SELECT substr(WorkoutSession.date, 9, 2) AS day FROM WorkoutSession "
        "WHERE userId = (SELECT id FROM UserInfo WHERE Username = ?) "
        "AND substr(WorkoutSession.date, 6, 2) = ? "
        "AND substr(WorkoutSession.date, 1, 4) = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        // Bind the username, month, and year parameters
        std::string username = TheTrainer->Username;
        std::string monthStr = (month < 10 ? "0" + std::to_string(month) : std::to_string(month)); // Ensure month is two digits
        std::string yearStr = std::to_string(year);

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, monthStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, yearStr.c_str(), -1, SQLITE_STATIC);

        // Execute the query and collect the results
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int day = std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            daysWithWorkouts.push_back(day);
        }

        // Finalize the statement
        sqlite3_finalize(stmt);
    }
    else {
        // Handle SQL preparation error
        std::cerr << "Failed to prepare SQL statement: " << sqlite3_errmsg(db) << std::endl;
    }
    return daysWithWorkouts;
}

void TrainerDataRenderer::defineWorkoutSessionPopup(int selectedYear, int selectedMonth, int day) {
    std::string date = std::to_string(selectedYear) + "-" + (selectedMonth < 10 ? "0" + std::to_string(selectedMonth) : std::to_string(selectedMonth)) + "-" + (day < 10 ? "0" + std::to_string(day) : std::to_string(day));
    std::vector<ImVec2> result = TheTrainer->app.GetPopupPosition(1000, 900);
    ImGui::SetNextWindowPos(result[0], ImGuiCond_Always);
    ImGui::SetNextWindowSize(result[1]);
    if (ImGui::BeginPopupModal("Workout Session", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        vector<SetDataSaver> setsDataSavers = TheTrainer->dataManager.FindWorkoutSessionInDataBase(TheTrainer->Username, date);
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        const char* months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
        std::string markOfTheDay = "";
        if (day % 10 == 1) { markOfTheDay = "st"; }
        if (day % 10 == 2) { markOfTheDay = "nd"; }
        if (day % 10 == 3) { markOfTheDay = "rd"; }
        else { markOfTheDay = "th"; }
        ImGui::Text(("This is the workout session of " + TheTrainer->Username + " on day " + std::to_string(day) + markOfTheDay + " of " + months[selectedMonth - 1] + " " + std::to_string(selectedYear)).c_str());
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        if (setsDataSavers.size() == 0) {
            ImGui::Text("No workout session recorded for this day.");
        }
        else {
            ImGui::Text("Type of Exercise : ");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            std::vector<std::string> typesOfExercisesAlreadyShown = {};
            for (int i = 0; i < setsDataSavers.size(); ++i) {                   // Display the different types of exercises
                if (std::find(typesOfExercisesAlreadyShown.begin(), typesOfExercisesAlreadyShown.end(), setsDataSavers[i].TypeOfExercise) == typesOfExercisesAlreadyShown.end()) { // If the type of exercise has not been shown yet
                    typesOfExercisesAlreadyShown.push_back(setsDataSavers[i].TypeOfExercise);
                    int count = std::count_if(setsDataSavers.begin(), setsDataSavers.end(), [&](const SetDataSaver& sds) { return sds.TypeOfExercise == setsDataSavers[i].TypeOfExercise; });

                    if (setsDataSavers[i].TypeOfExercise == WantOpenSetsOfTypeExercise) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(61 / 255.0f, 133 / 255.0f, 224 / 255.0f, 1.0f)); // Sky blue color when selected
                        ImGui::Button((setsDataSavers[i].TypeOfExercise + " (x" + std::to_string(count) + ")").c_str(), ImVec2(80, 30));
                        if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
                        ImGui::PopStyleColor();
                    }
                    else {
                        if (ImGui::Button((setsDataSavers[i].TypeOfExercise + " (x" + std::to_string(count) + ")").c_str(), ImVec2(80, 30))) { WantOpenSetsOfTypeExercise = setsDataSavers[i].TypeOfExercise; }
                        if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
                    }
                    if (i != setsDataSavers.size() - 1) { ImGui::SameLine(); }
                }
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Text("At load : ");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            std::vector<int> typesOfLoadAlreadyShown = {};
            bool AOneRMExperimentHasBeenDoneForExercice = false;
            for (int i = 0; i < setsDataSavers.size(); ++i) {
                if (setsDataSavers[i].TypeOfExercise == WantOpenSetsOfTypeExercise) {
                    if (std::find(typesOfLoadAlreadyShown.begin(), typesOfLoadAlreadyShown.end(), setsDataSavers[i].TotalWeightWanted) == typesOfLoadAlreadyShown.end()) { // If the type of exercise has not been shown yet
                        typesOfLoadAlreadyShown.push_back(setsDataSavers[i].TotalWeightWanted);
                        int count = std::count_if(setsDataSavers.begin(), setsDataSavers.end(), [&](const SetDataSaver& sds) { return sds.TypeOfExercise == setsDataSavers[i].TypeOfExercise && sds.TotalWeightWanted == setsDataSavers[i].TotalWeightWanted; });

                        if (setsDataSavers[i].TotalWeightWanted == WantOpenSetsOfLoad) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(61 / 255.0f, 133 / 255.0f, 224 / 255.0f, 1.0f)); // Sky blue color when selected
                            ImGui::Button((std::to_string((int)setsDataSavers[i].TotalWeightWanted) + " (x" + std::to_string(count) + ")").c_str(), ImVec2(80, 30));
                            if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
                            ImGui::PopStyleColor();
                        }
                        else {
                            if (ImGui::Button((std::to_string((int)setsDataSavers[i].TotalWeightWanted) + " (x" + std::to_string(count) + ")").c_str(), ImVec2(80, 30))) {
                                WantOpenSetsOfLoad = setsDataSavers[i].TotalWeightWanted;
                                WantOpenOneRM = false;
                            }
                            if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
                        }
                        if (i != setsDataSavers.size() - 1) { ImGui::SameLine(); }
                    }
                }
                if (setsDataSavers[i].SetUsedForOneRMExperiment) { AOneRMExperimentHasBeenDoneForExercice = true; }
            }
            if (WantOpenSetsOfTypeExercise != "" && AOneRMExperimentHasBeenDoneForExercice) {
                ImGui::SameLine(0.0f, 20.0f);
                if (WantOpenOneRM) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(61 / 255.0f, 133 / 255.0f, 224 / 255.0f, 1.0f)); // Sky blue color when selected
                    ImGui::Button("One RM Experiment", ImVec2(150, 30));
                    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
                    ImGui::PopStyleColor();
                }
                else {
                    if (ImGui::Button("One RM Experiment", ImVec2(150, 30))) {
                        WantOpenOneRM = true;
                        WantOpenSetsOfLoad = 0;
                    }
                    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
                }
            }

            if (!WantOpenOneRM) {
                std::vector<SetDataSaver> setsDataSaversToShow;
                std::copy_if(setsDataSavers.begin(), setsDataSavers.end(), std::back_inserter(setsDataSaversToShow), [&](const SetDataSaver& sds) { return sds.TypeOfExercise == WantOpenSetsOfTypeExercise && sds.TotalWeightWanted == WantOpenSetsOfLoad; });
                if (WantOpenSetsOfTypeExercise == "") { ImGui::Dummy(ImVec2(0.0f, 35.0f)); }
                else { ImGui::Dummy(ImVec2(0.0f, 5.0f)); }
                if (ImPlot::BeginPlot("Workout Data")) {
                    int j = 1;
                    for (const auto& sds : setsDataSaversToShow) {
                        std::vector<double> xData;
                        for (size_t i = 1; i <= sds.VelocityOfSet[0].size(); ++i) {
                            xData.push_back(static_cast<double>(i));
                        }
                        //ImPlot::PlotLine("Distance", xData.data(), sds.HeightOfSet[0].data(), xData.size());
                        std::string plotTitle = "Velocity of set " + std::to_string(j);
                        ImPlot::PlotLine(plotTitle.c_str(), xData.data(), sds.VelocityOfSet[0].data(), xData.size());
                        //ImPlot::PlotLine("Acceleration", xData.data(), sds.AccelerationOfSet[0].data(), xData.size());
                        j++;
                    }
                    ImPlot::EndPlot();
                }
                bool AtLeastOneOfTheSetUsedLoadProfile = std::any_of(setsDataSaversToShow.begin(), setsDataSaversToShow.end(),
                    [](const SetDataSaver& sds) { return sds.UsingLoadProfile; });
                if (AtLeastOneOfTheSetUsedLoadProfile) {
                    if (ImPlot::BeginPlot("Load Profile")) {
                        int j = 1;
                        for (const auto& sds : setsDataSaversToShow) {
                            std::vector<double> xList;
                            std::vector<double> yList;
                            for (const auto& point : sds.profileShape) {
                                xList.push_back(point[0]);
                                yList.push_back(point[1]);
                            }
                            std::string plotTitle = "Load Profile of set " + std::to_string(j);
                            ImPlot::PlotLine(plotTitle.c_str(), xList.data(), yList.data(), xList.size());
                            j++;
                        }
                        ImPlot::EndPlot();
                    }
                }
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                for (auto it = setsDataSaversToShow.begin(); it != setsDataSaversToShow.end(); ++it) {
                    const auto& sds = *it;
                    ImGui::Text(("Set " + std::to_string(std::distance(setsDataSaversToShow.begin(), it) + 1) + " : ").c_str());
                    ImGui::Text(("Number of repetitions : " + std::to_string(sds.numberOfReps) + ".").c_str());
                    ImGui::SameLine(0.0f, 20.0f);
                    ImGui::Text(("Set used for One RM experiment : " + std::string(sds.SetUsedForOneRMExperiment ? "yes." : "no.")).c_str());
                    ImGui::SameLine(0.0f, 20.0f);
                    ImGui::Text(("Using Load Profile : " + std::string(sds.UsingLoadProfile ? "yes." : "no.")).c_str());
                }

                ImGui::Dummy(ImVec2(0.0f, 5.0f));
            }
            if (WantOpenOneRM) {
                vector<OneRMAnalyser> oneRMAnalysers = TheTrainer->dataManager.FindOneRMExperimentInDataBase(TheTrainer->Username, date);
                OneRMAnalyser oneRMAnalyser = oneRMAnalysers[0];
                //oneRMAnalyser.createEnduranceExperimentDataKnowingOneRMExperiment();
                if (ImPlot::BeginPlot("One RM Experiment", "Load in kg", "Velocity")) {
                    ImPlot::PlotScatter("Max velocities during reps", oneRMAnalyser.OneRMExperimentLoads.data(), oneRMAnalyser.OneRMExperimentVelocities.data(), oneRMAnalyser.OneRMExperimentLoads.size());
                    //Plot regression :
                    std::vector<double> X(100); std::generate(X.begin(), X.end(), [n = 0, step = oneRMAnalyser.OneRMValue / 99.0]() mutable { return n++ * step; });
                    std::vector<double> Y(100); std::transform(X.begin(), X.end(), Y.begin(), [&](double x) { return oneRMAnalyser.CoefficientsOneRMRegression[0] * x + oneRMAnalyser.CoefficientsOneRMRegression[1]; });
                    ImPlot::PlotLine("Linear Regression", X.data(), Y.data(), X.size());
                    ImPlot::PlotLine("One RM Velocity", X.data(), std::vector<double>(X.size(), oneRMAnalyser.OneRMVelocityValue).data(), X.size()); //Plot horizontal line at the OneRMVelocityValue
                    ImPlot::PlotScatter(("One RM Value (" + std::to_string(oneRMAnalyser.OneRMValue) + ", " + std::to_string(oneRMAnalyser.OneRMVelocityValue) + ")").c_str(), &oneRMAnalyser.OneRMValue, &oneRMAnalyser.OneRMVelocityValue, 1); //Plot point at (OneRMValue, OneRMVelocityValue)
                    ImPlot::EndPlot();
                }
                //ImGui::SameLine();
                if (ImPlot::BeginPlot("Endurance", "Velocity loss in %", "Repetition in %")) {
                    ImPlot::PlotScatter("Endurance experiment", oneRMAnalyser.EnduranceExperimentVelocitiesLoss.data(), oneRMAnalyser.EnduranceExperimentRepetitionRatio.data(), oneRMAnalyser.EnduranceExperimentRepetitionRatio.size());
                    //Plot regression :
                    std::vector<double> X(100); std::generate(X.begin(), X.end(), [n = 0]() mutable { return n++ * 100.0 / 99.0; });
                    std::vector<double> Y(100); std::transform(X.begin(), X.end(), Y.begin(), [&](double x) { return oneRMAnalyser.CoefficientsEnduranceLaw[0] * x * x + oneRMAnalyser.CoefficientsEnduranceLaw[1] * x + oneRMAnalyser.CoefficientsEnduranceLaw[2]; });
                    ImPlot::PlotLine("Polynomial Regression", X.data(), Y.data(), X.size());
                    ImPlot::EndPlot();
                }
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Text(("One RM value : " + std::to_string(oneRMAnalyser.OneRMValue)).c_str());
            }
        }
        //ImGui::SameLine();
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((1000 - 120) * 0.5f);
        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
            WantOpenSetsOfTypeExercise = "";
            WantOpenSetsOfLoad = 0;
            WantOpenOneRM = false;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImGui::EndPopup();
    }
}

void TrainerDataRenderer::defineCalendarPopup() {
    std::vector<ImVec2> result = TheTrainer->app.GetPopupPosition(540, 400);
    ImGui::SetNextWindowPos(result[0], ImGuiCond_Always);
    ImGui::SetNextWindowSize(result[1]);
    if (ImGui::BeginPopupModal("Calendar", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::Text(("This is the calendar of the different workout session of " + TheTrainer->Username).c_str());
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        // Get the current date
        std::time_t t = std::time(nullptr);
        std::tm* now = std::localtime(&t);

        // Get the number of days in the current month
        if (selectedYear == 0) selectedYear = now->tm_year + 1900;
        if (selectedMonth == 0) selectedMonth = now->tm_mon + 1;

        // Array of month names
        const char* months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
        // Button to open month selection menu
        if (ImGui::Button(months[selectedMonth - 1])) {
            ImGui::OpenPopup("Month Selection");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        // Month selection menu
        if (ImGui::BeginPopup("Month Selection")) {
            for (int i = 0; i < 12; ++i) {
                if (ImGui::MenuItem(months[i])) {
                    selectedMonth = i + 1;
                }
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        // Button to open year selection menu
        if (ImGui::Button(std::to_string(selectedYear).c_str())) {
            ImGui::OpenPopup("Year Selection");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        // Year selection menu
        if (ImGui::BeginPopup("Year Selection")) {
            for (int i = 2020; i <= now->tm_year + 1900; ++i) {
                if (ImGui::MenuItem(std::to_string(i).c_str())) {
                    selectedYear = i;
                }
            }
            ImGui::EndPopup();
        }
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        int daysInMonth = 31;
        if (selectedMonth == 4 || selectedMonth == 6 || selectedMonth == 9 || selectedMonth == 11) {
            daysInMonth = 30;
        }
        else if (selectedMonth == 2) {
            bool isLeapYear = (selectedYear % 4 == 0 && selectedYear % 100 != 0) || (selectedYear % 400 == 0);
            daysInMonth = isLeapYear ? 29 : 28;
        }
        std::vector<double> daysWithWorkouts = getMonthActivity(selectedMonth, selectedYear); // Calculate the day of the week for the first day of the month
        int firstDayOfWeek = getFirstDayOfWeek(selectedYear, selectedMonth); // Create a matrix of buttons for the days
        int columns = 7; // 7 days in a week
        // Add spaces before the first day of the month
        for (int i = 0; i < firstDayOfWeek; ++i) {
            ImGui::Dummy(ImVec2(40, 40));
            if (i != columns - 1) {
                ImGui::SameLine();
            }
        }
        // Add buttons for each day of the month
        for (int day = 1; day <= daysInMonth; ++day) {
            if ((day + firstDayOfWeek - 1) % columns != 0) {
                ImGui::SameLine();
            }
            if (std::find(daysWithWorkouts.begin(), daysWithWorkouts.end(), day) != daysWithWorkouts.end()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.7f, 0.4f, 1.0f)); // Green color
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.8f, 0.5f, 1.0f)); // Lighter green when hovered
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.6f, 0.3f, 1.0f)); // Darker green when active
            }

            if (ImGui::Button(std::to_string(day).c_str(), ImVec2(40, 40))) {
                selectedDay = day;
                ImGui::OpenPopup("Workout Session");
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            if (std::find(daysWithWorkouts.begin(), daysWithWorkouts.end(), day) != daysWithWorkouts.end()) {
                ImGui::PopStyleColor(3);
            }
        }
        defineWorkoutSessionPopup(selectedYear, selectedMonth, selectedDay);

        // Close the popup
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((540 - 120) * 0.5f);
        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImGui::EndPopup();
    }
}

void TrainerDataRenderer::Plot(vector<double> X, vector<double> Y, std::string label, std::string type) {
    if (type == "lines") {
        ImPlot::PlotLine(label.c_str(), X.data(), Y.data(), X.size());
    }
    else if (type == "scatter") {
        ImPlot::PlotScatter(label.c_str(), X.data(), Y.data(), X.size());
    }
}