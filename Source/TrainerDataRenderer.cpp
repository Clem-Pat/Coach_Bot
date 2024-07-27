#include "TrainerDataRenderer.h"
#include "Trainer.h" // Ensure this header file is included
#include "imgui_internal.h"
#include <sqlite3.h>
#include <ctime>
#include <vector>
#include <string>
#include <iostream>

std::vector<double> TrainerDataRenderer::getMonthActivity(int month, int year) {
    //Just a function to get the number of the days with workouts
    //Useful to put green the days with workouts in the calendar

    sqlite3* db = TheTrainer->dataManager.db;
    std::vector<double> daysWithWorkouts;

    // Prepare the SQL query
    std::string sql = "SELECT strftime('%d', WorkoutSession.date) AS day FROM WorkoutSession "
        "JOIN UserInfo ON WorkoutSession.userId = UserInfo.id "
        "WHERE SUBSTR(WorkoutSession.date, 6, 2) = ? AND SUBSTR(WorkoutSession.date, 1, 4) = ? AND UserInfo.Username = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        // Bind the month, year, and username parameters
        std::string monthStr = (month < 10 ? "0" + std::to_string(month) : std::to_string(month));
        sqlite3_bind_text(stmt, 1, monthStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, std::to_string(year).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, TheTrainer->Username.c_str(), -1, SQLITE_STATIC);

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
    ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x - 400) * 0.5f, (ImGui::GetIO().DisplaySize.y - 300) * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(540, 400));
    if (ImGui::BeginPopupModal("Workout Session", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        const char* months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
        if (day == 1) {
            ImGui::Text(("This is the workout session of " + TheTrainer->Username + " on day " + std::to_string(day) + "st of " + months[selectedMonth-1] + " " + std::to_string(selectedYear)).c_str());
        }
        if (day == 2) {
            ImGui::Text(("This is the workout session of " + TheTrainer->Username + " on day " + std::to_string(day) + "nd of " + months[selectedMonth-1] + " " + std::to_string(selectedYear)).c_str());
        }
        if (day == 3) {
			ImGui::Text(("This is the workout session of " + TheTrainer->Username + " on day " + std::to_string(day) + "rd of " + months[selectedMonth-1] + " " + std::to_string(selectedYear)).c_str());
		}
		else {
			ImGui::Text(("This is the workout session of " + TheTrainer->Username + " on day " + std::to_string(day) + "th of " + months[selectedMonth-1] + " " + std::to_string(selectedYear)).c_str());
		}
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((540 - 120) * 0.5f);
        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void TrainerDataRenderer::defineCalendarPopup() {
    ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x - 400) * 0.5f, (ImGui::GetIO().DisplaySize.y - 300) * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(540, 400));
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
        if (ImGui::Button(months[selectedMonth-1])) {
            ImGui::OpenPopup("Month Selection");
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

        std::vector<double> daysWithWorkouts = getMonthActivity(selectedMonth, selectedYear);
        // Create a matrix of buttons for the days
        int columns = 7; // 7 days in a week
        for (int day = 1; day <= daysInMonth; ++day) {
            if (day % columns != 1) {
                ImGui::SameLine();
            }
            if (std::find(daysWithWorkouts.begin(), daysWithWorkouts.end(), day) != daysWithWorkouts.end()){
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

        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((540 - 120) * 0.5f);
        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}