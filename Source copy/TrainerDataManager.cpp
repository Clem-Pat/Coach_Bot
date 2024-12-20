/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org
 * Please refer to the Datasheet to know more about the application
 */


#include "TrainerDataManager.h"
#include "Trainer.h"
#include "TrainerAnalyser.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include "sqlite3.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include <set> //For std::set<std::string> validOperations = { "MAX", "MIN", "SUM", "AVG" };
#include <sys/stat.h>


std::ofstream writing_file;

std::string TrainerDataManager::GetTodaysDate() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d"); //Date is in Year-month-day format
    return ss.str();
}

std::string VectorToString(const std::vector<double>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (const auto& val : vec) {
        oss << val << ", ";
    }
    // Remove the last comma and space if the vector is not empty
    if (!vec.empty()) {
        auto str = oss.str();
        oss.str("");
        oss << str.substr(0, str.length() - 2);
    }
    oss << "]";
    return oss.str();
}

std::string VectorOfVectorToString(const std::vector<std::vector<double>>& vecOfVec) {
    std::ostringstream oss;
    oss << "[";
    for (const auto& vec : vecOfVec) {
        oss << "[";
        for (const auto& val : vec) {
            oss << val << ", ";
        }
        // Remove the last comma and space if the inner vector is not empty
        if (!vec.empty()) {
            auto str = oss.str();
            oss.str("");
            oss << str.substr(0, str.length() - 2);
        }
        oss << "], ";
    }
    // Remove the last comma and space if the outer vector is not empty
    if (!vecOfVec.empty()) {
        auto str = oss.str();
        oss.str("");
        oss << str.substr(0, str.length() - 2);
    }
    oss << "]";
    return oss.str();
}

std::vector<double> StringToVector(const std::string& str) {
    std::vector<double> result;
    std::string trimmedStr = str.substr(1, str.size() - 2); // Remove the outer brackets

    std::istringstream ss(trimmedStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim whitespace from the token
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        result.push_back(std::stod(token));
    }

    return result;
}

std::vector<std::vector<double>> StringToVectorOfVector(const std::string& str) {
    std::vector<std::vector<double>> result;
    std::string trimmedStr = str.substr(1, str.size() - 2); // Remove the outer brackets

    std::istringstream ss(trimmedStr);
    std::string token;

    while (std::getline(ss, token, ']')) {
        if (token.find('[') != std::string::npos) {
            std::string innerVecStr = token.substr(token.find('[') + 1);
            std::istringstream innerSS(innerVecStr);
            std::string innerToken;
            std::vector<double> innerVec;

            while (std::getline(innerSS, innerToken, ',')) {
                // Trim whitespace from the token
                innerToken.erase(std::remove_if(innerToken.begin(), innerToken.end(), ::isspace), innerToken.end());
                if (!innerToken.empty()) {
                    innerVec.push_back(std::stod(innerToken));
                }
            }
            result.push_back(innerVec);
        }
    }

    return result;
}

std::vector<double> ConvertIntVectorToDoubleVector(const std::vector<int>& intVec) {
    std::vector<double> doubleVec;
    doubleVec.reserve(intVec.size()); // Reserve space to avoid multiple allocations

    for (int val : intVec) {
        doubleVec.push_back(static_cast<double>(val));
    }

    return doubleVec;
}

std::vector<int> ConvertDoubleVectorToIntVector(const std::vector<double>& doubleVec) {
    std::vector<int> intVec;
    intVec.reserve(doubleVec.size()); // Reserve space to avoid multiple allocations

    for (double val : doubleVec) {
        intVec.push_back(static_cast<int>(val));
    }

    return intVec;
}

void TrainerDataManager::Begin(const std::string& filename) {
    //TODO Failed to open database, sqlite3 is not linked apparently. 
    int rc = sqlite3_open((filename + ".db").c_str(), &db);

    if (rc) {
        std::cout << "Error opening SQLite3 database: " << sqlite3_errmsg(db) << std::endl;
    }
    else {
        std::cout << "Opened database successfully" << std::endl;
    }

    const char* createUserInfoTable = "CREATE TABLE IF NOT EXISTS UserInfo ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "Username TEXT NOT NULL,"
        "password TEXT NOT NULL);";

    const char* createOneRMTable = "CREATE TABLE IF NOT EXISTS OneRM ("
        "userId INTEGER NOT NULL,"
        "typeOfExercise TEXT NOT NULL,"
        "valueOfOneRM REAL NOT NULL,"
        "coefficientsOfOneRMRegression TEXT NOT NULL,"
        "coefficientsOfEndurance TEXT NOT NULL,"
        "indexesOfSetsUsedForThisOneRM TEXT NOT NULL,"
        "OneRMExperimentLoads TEXT NOT NULL,"
        "OneRMExperimentVelocities TEXT NOT NULL,"
        "date TEXT NOT NULL,"
        "FOREIGN KEY(userId) REFERENCES UserInfo(id));";

    const char* createWorkoutSessionTable = "CREATE TABLE IF NOT EXISTS WorkoutSession ("
        "userId INTEGER NOT NULL,"
        "indexOfSet INTEGER NOT NULL,"
        "typeOfExercise TEXT NOT NULL,"
        "load REAL NOT NULL,"
        "nbreOfReps REAL NOT NULL,"
        "timesOfSet TEXT NOT NULL,"
        "velocitiesOfSet TEXT NOT NULL,"
        "coordinatesPointsProfile TEXT NOT NULL,"
        "usedForOneRM TEXT NOT NULL,"
        "date TEXT NOT NULL,"
        "FOREIGN KEY(userId) REFERENCES UserInfo(id));";

    // Execute SQL statements
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createUserInfoTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create UserInfo table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }

    rc = sqlite3_exec(db, createOneRMTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create OneRM table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }

    rc = sqlite3_exec(db, createWorkoutSessionTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create WorkoutSession table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }
}

int TrainerDataManager::GetUserId(const std::string& username) {
    const char* sql = "SELECT id FROM UserInfo WHERE Username = ?";
    sqlite3_stmt* stmt = nullptr;
    int userId = -1; // Default to -1 to indicate not found

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Bind the username to the query
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        // Execute the query and check if we got a result
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the user ID from the query result
            userId = sqlite3_column_int(stmt, 0);
        }

        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return userId;
}

int TrainerDataManager::CountUsers() {
    const char* sql = "SELECT COUNT(*) FROM UserInfo;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0; // Default to 0 if no rows found or in case of error

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Execute the query
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the count from the query result
            count = sqlite3_column_int(stmt, 0);
        }
        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return count;
}

void TrainerDataManager::AddUser(const std::string& username, const std::string& password) {
    char* errMsg = nullptr;
    int id = CountUsers() + 1;
    std::string sql = "INSERT INTO UserInfo (id, Username, password) VALUES ("
        + std::to_string(id) + ", '"
        + username + "', '"
        + password + "');";

    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to insert new user into database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "New user added successfully" << std::endl;
    }
}

bool fileExists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void TrainerDataManager::SaveSetInFile() {
    //We have two ways to store Data. Either in a file or in a database. The Database is for the user to be able to retrieve the data later. The file is for the engineer to conduct experiments. Please uncomment the next few lines to make it work. 
    if (TheTrainer->ExperimentMode) {
        //Save the data in a CSV file (useful for engineers to conduct experiments
        std::string baseFilename = GetTodaysDate() + "_" + TheTrainer->Username + "_" + TheTrainer->TypeOfExercise + "_" + std::to_string(TheTrainer->botManager.TotalWeightWanted);
        std::string filename = baseFilename + ".csv";
        int counter = 1;

        // Check if the file already exists and append a digit if necessary
        while (fileExists(filename)) {
            filename = baseFilename + "_" + std::to_string(counter) + ".csv";
            counter++;
        }
        std::ofstream writing_file;
        writing_file.open(filename.c_str(), std::ios::out);

        //Writing the names of the columns
        writing_file << "Time(s), ";
        for (int i = 1; i < experimentDataNames.size(); i++) {
            if (experimentDataNames[i] != "") { writing_file << experimentDataNames[i] << ", "; }
        }
        writing_file << std::endl;

        //Filling the lines of the file
        for (int line = 0; line < experimentData[0].size(); line++) {
            for (int column = 0; column < experimentData.size(); column++) {
                if (experimentData[column].size() > 0) {
                    writing_file << experimentData[column][line] << ", ";
                }
            }
            writing_file << std::endl;
        }
        writing_file.close();
        std::cout << "Set saved in CSV file : " << filename << std::endl;
    }
}

void TrainerDataManager::AddData(int index, const double data) {
    //AddData value after value (used for Bot Simulator)
    experimentData[index].push_back(data);
    if (index != 0) {
        TheTrainer->app.Plot(index - 1, experimentData[0], experimentData[index], 0, experimentDataNames[index], "lines");
        TheTrainer->app.listOfPlotsToKeep[index - 1] = false; //We want to be able to clear this data from the plot one day.  
    }
}

void TrainerDataManager::AddData(std::vector<double>& data, std::vector<bool> WantToPlot) {
    //AddData (used for real Bot)
    if (WantToPlot.size() != data.size()) {
        WantToPlot = std::vector<bool>(data.size(), true); //If nothing is specified in WantToPlot argument it means we want to plot everything
        if (WantToPlot.size() != 1) { //We tried to sepcify what data to plot but it was not the same size as the data so there is a human error, we let the user know
            std::cout << " -- WARNING in AddData: WantToPlot.size() != data.size() so we plot every data anyway -- " << std::endl;
        }
    }
    for (int i = 0; i < data.size(); i++)
    {
        experimentData[i].push_back(data[i]);
        if (i != 0 && WantToPlot[i]) {
            TheTrainer->app.listOfPlotsToKeep[i - 1] = false; //We want to be able to clear this data from the plot one day.  
            TheTrainer->app.Plot(i - 1, experimentData[0], experimentData[i], 0, experimentDataNames[i], "scatter");
        }
    }
}

int TrainerDataManager::findIndexOfEmptyData() {
    int k = 0;
    while (k < sizeof(experimentData))
    {
        if (experimentData[k].size() == 0) {
            break;
        }
        else {
            k++;
        }
    }
    return k;
}

void TrainerDataManager::ConfigureDataName(int index, const std::string& dataName) {
    experimentDataNames[index] = dataName;
}

void TrainerDataManager::ConfigureDataNames(const std::vector<std::string>& dataNames) {
    if (!dataNames.empty()) {
        for (int i = 0; i < dataNames.size(); i++)
        {
            experimentDataNames[i] = dataNames[i];
        }
    }
}

std::string TrainerDataManager::FindInDataBase(std::string username, std::string typeOfExercise, std::string totalWeightWanted, std::string whatWeLookFor, std::string columnName) {
    // whatWeLookFor is the SQL function to use (e.g. "MAX", "MIN", "AVG", "SUM", etc.)
    // columnName is the name of the column to apply the function on (e.g. "nbreOfReps", "velocityOfSet", etc.)
    // for example : "SELECT MAX(velocityOfSet) FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = 'username') AND typeOfExercise = 'curl' AND load = 20;"
    // It can also find if there is data corresponding to specific definition. For example, if we want to find if there is a one RM experiment, we can use whatWeLookFor = "1" and columnName = "usedForOneRM"
    // for example : "SELECT * FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = 'username') AND typeOfExercise = 'curl' AND usedForOneRM = 1;"
    std::set<std::string> validOperations = { "MAX", "MIN", "SUM", "AVG" };
    std::string query = "";

    if (validOperations.find(whatWeLookFor) != validOperations.end()) {
        query = "SELECT " + whatWeLookFor + "(" + columnName + ")" + " FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND typeOfExercise = '" + typeOfExercise + "' AND load = " + totalWeightWanted + ";";
    }
    else {
        if (totalWeightWanted != "NULL") { //If we want to find sets with a specific load
            query = "SELECT * FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND typeOfExercise = '" + typeOfExercise + "' AND load = " + totalWeightWanted + " AND " + columnName + " = " + whatWeLookFor + ";";
        }
        else {  //If we want to find sets with any load
            query = "SELECT * FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND typeOfExercise = '" + typeOfExercise + "' AND " + columnName + " = " + whatWeLookFor + ";";
        }
    }
    const char* sql = query.c_str();

    sqlite3_stmt* stmt = nullptr;
    std::string result = "NULL"; // Default to NULL if no rows found or in case of error

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Execute the query
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (text) {
                result = std::string(text);
            }
            else {
                result = "NULL"; // Handle null case appropriately
            }
        }
        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query " << whatWeLookFor + "(" + columnName + ") : " << sqlite3_errmsg(db) << std::endl;
    }
    return result;
}

void TrainerDataManager::UploadLastOneRMExperimentFromDataBase(std::string username, std::string typeOfExercise) {
    if (TheTrainer->dataManager.FindInDataBase(username, typeOfExercise, "NULL", "1", "usedForOneRM") != "NULL") {
        std::vector<OneRMAnalyser> oneRMAnalysers = FindOneRMExperimentInDataBase(username, "NULL", typeOfExercise); //We get all the OneRM experiments at any date but with a specific typeOfExercise
        oneRMAnalysers.back().UpdateAndPlotOneRMExperiment(true);
        std::unique_ptr<TrainerAnalyser> analyser = std::make_unique<OneRMAnalyser>(oneRMAnalysers.back());
        TheTrainer->predictor.sessionAnalysers.push_back(std::move(analyser));
        TheTrainer->app.AddRobotConsoleMessage("One RM experiment from date " + TheTrainer->predictor.sessionAnalysers.back()->Date + " has been successfully uploaded.");
    }
    else {
        TheTrainer->app.AddRobotConsoleError("No One RM experiment found for this username and this type of exercise.");
    }
}

std::vector<SetDataSaver> TrainerDataManager::FindWorkoutSessionInDataBase(std::string username, std::string date) {
    //std::string query = "SELECT * FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND date = '" + date + "';";
    std::string query = "SELECT indexOfSet, typeOfExercise, load, nbreOfReps, timesOfSet, velocitiesOfSet, coordinatesPointsProfile, usedForOneRM, date FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = \"" + username + "\") AND date = \"" + date + "\";";
    const char* sql = query.c_str();
    sqlite3_stmt* stmt = nullptr;
    vector<SetDataSaver> setsDataSavers = {};

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Execute the query and process the results
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the values from the query result
            int indexOfSet = sqlite3_column_int(stmt, 0);
            std::string typeOfExercise(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            double load = sqlite3_column_double(stmt, 2);
            int nbreOfReps = sqlite3_column_int(stmt, 3);
            std::string timesOfSetStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            std::vector<double> timesOfSet = StringToVector(timesOfSetStr);
            std::string velocitiesOfSetStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            std::vector<double> velocitiesOfSet = StringToVector(velocitiesOfSetStr);
            std::string coordinatesPointsProfileStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            std::vector<std::vector<double>> coordinatesPointsProfile = StringToVectorOfVector(coordinatesPointsProfileStr);
            bool usedForOneRM = sqlite3_column_int(stmt, 7);
            bool UsingLoadProfile = coordinatesPointsProfile == std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0)) ? false : true;
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

            //We create a summary SetDataSaver of the set 
            SetDataSaver setAnalyser(nullptr, 0, 0);
            setAnalyser.Username = username;
            setAnalyser.id = indexOfSet;
            setAnalyser.TypeOfExercise = typeOfExercise;
            setAnalyser.TotalWeightWanted = load;
            setAnalyser.numberOfReps = nbreOfReps;
            setAnalyser.TimeOfSet = timesOfSet;
            setAnalyser.VelocityOfSet[0] = velocitiesOfSet;
            setAnalyser.profileShape = coordinatesPointsProfile;
            setAnalyser.SetUsedForOneRMExperiment = usedForOneRM;
            setAnalyser.UsingLoadProfile = UsingLoadProfile;
            setAnalyser.Date = date;
            setAnalyser.closed = true;
            setsDataSavers.push_back(setAnalyser);
        }

        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return setsDataSavers;
}

std::vector<OneRMAnalyser> TrainerDataManager::FindOneRMExperimentInDataBase(std::string username, std::string date, std::string typeOfExercise) {
    std::string query = "NULL";
    if (date != "NULL") { //We want to get all the OneRM experiments of a specific date
        query = "SELECT typeOfExercise, valueOfOneRM, coefficientsOfOneRMRegression, coefficientsOfEndurance, indexesOfSetsUsedForThisOneRM, OneRMExperimentLoads, OneRMExperimentVelocities, date FROM OneRM WHERE userId = (SELECT id FROM UserInfo WHERE Username = \"" + username + "\") AND date = \"" + date + "\";";
    }
    else { //We want to get all the OneRM experiments of a specific type of exercise 
        query = "SELECT typeOfExercise, valueOfOneRM, coefficientsOfOneRMRegression, coefficientsOfEndurance, indexesOfSetsUsedForThisOneRM, OneRMExperimentLoads, OneRMExperimentVelocities, date FROM OneRM WHERE userId = (SELECT id FROM UserInfo WHERE Username = \"" + username + "\") AND typeOfExercise = \"" + typeOfExercise + "\";";
    }
    const char* sql = query.c_str();
    sqlite3_stmt* stmt = nullptr;
    vector<OneRMAnalyser> oneRMAnalysers = {};

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Execute the query and process the results
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the values from the query result
            std::string typeOfExercise(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            double valueOfOneRM = sqlite3_column_double(stmt, 1);
            std::string coefficientsOfOneRMRegressionStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            std::vector<double> coefficientsOfOneRMRegression = StringToVector(coefficientsOfOneRMRegressionStr);
            std::string coefficientsOfEnduranceStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::vector<double> coefficientsOfEndurance = StringToVector(coefficientsOfEnduranceStr);
            std::string indexesOfSetsUsedForThisOneRMStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            std::vector<double> indexesOfSetsUsedForThisOneRM = StringToVector(indexesOfSetsUsedForThisOneRMStr);
            std::string OneRMExperimentLoadsStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            std::vector<double> OneRMExperimentLoads = StringToVector(OneRMExperimentLoadsStr);
            std::string OneRMExperimentVelocitiesStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            std::vector<double> OneRMExperimentVelocities = StringToVector(OneRMExperimentVelocitiesStr);
            std::string Date(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));

            //BEGIN We create a summary OneRMAnalyser
            OneRMAnalyser oneRMAnalyser(nullptr, 0, 0);
            if (date != "NULL") { oneRMAnalyser = OneRMAnalyser(nullptr, 0, 0); } // This is not clean. In the beginning we wanted to create empty analysers to render them in the calendar without making them polluting the TheTrainer. This is why we give them nullptr...
            else { oneRMAnalyser = OneRMAnalyser(TheTrainer, 0, 0); } //....But with "uploading the last OneRM experiment" we need to create a real analyser so it can have access to the current TheTrainer.app in order to be plotted.

            oneRMAnalyser.Username = username;
            oneRMAnalyser.TypeOfExercise = typeOfExercise;
            oneRMAnalyser.OneRMValue = valueOfOneRM;
            oneRMAnalyser.CoefficientsOneRMRegression = coefficientsOfOneRMRegression;
            oneRMAnalyser.CoefficientsEnduranceLaw = coefficientsOfEndurance;
            oneRMAnalyser.indexesOfSetDataSaver = ConvertDoubleVectorToIntVector(indexesOfSetsUsedForThisOneRM);
            oneRMAnalyser.OneRMExperimentLoads = OneRMExperimentLoads;
            oneRMAnalyser.OneRMExperimentVelocities = OneRMExperimentVelocities;
            oneRMAnalyser.Date = Date;
            oneRMAnalyser.closed = true;
            oneRMAnalyser.createEnduranceExperimentDataKnowingOneRMExperiment();
            oneRMAnalysers.push_back(oneRMAnalyser);
            //FINISH creating the summary OneRMAnalyser
        }

        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return oneRMAnalysers;
}

void TrainerDataManager::SaveSessionInDataBase() {
    std::cout << "Saving session in database" << std::endl;
    for (auto& analyser : TheTrainer->predictor.sessionAnalysers) {
        if (analyser.get()->Date == GetTodaysDate()) {
            //Saving One RM value in database
            char* errMsg = nullptr;
            int rc;
            int id;
            std::string sql;
            if (dynamic_cast<OneRMAnalyser*>(analyser.get())) {
                OneRMAnalyser* OneRManalyser = dynamic_cast<OneRMAnalyser*>(analyser.get());
                if (OneRManalyser->closed) {
                    std::cout << "Saving OneRM in database" << std::endl;
                    id = GetUserId(OneRManalyser->Username);
                    if (id == -1) { AddUser(OneRManalyser->Username); }
                    id = GetUserId(OneRManalyser->Username);
                    if (OneRManalyser->OneRMValue != -1) {
                        sql = "INSERT INTO OneRM (userId, typeOfExercise, valueOfOneRM, coefficientsOfOneRMRegression, coefficientsOfEndurance, indexesOfSetsUsedForThisOneRM, OneRMExperimentLoads, OneRMExperimentVelocities, date) VALUES (" + std::to_string(id) + ", '" + OneRManalyser->TypeOfExercise + "', " + std::to_string(OneRManalyser->OneRMValue) + ", '" + VectorToString(OneRManalyser->CoefficientsOneRMRegression) + "', '" + VectorToString(OneRManalyser->CoefficientsEnduranceLaw) + "', '" + VectorToString(ConvertIntVectorToDoubleVector(OneRManalyser->indexesOfSetDataSaver)) + "', '" + VectorToString(OneRManalyser->OneRMExperimentLoads) + "', '" + VectorToString(OneRManalyser->OneRMExperimentVelocities) + "', '" + OneRManalyser->Date + "');";
                        errMsg = nullptr;
                        rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
                        if (rc != SQLITE_OK) {
                            std::cout << "Failed to insert OneRM in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
                            sqlite3_free(errMsg);
                        }
                    }
                }
            }
            if (dynamic_cast<SetDataSaver*>(analyser.get())) {
                SetDataSaver* dataSaver = dynamic_cast<SetDataSaver*>(analyser.get());
                if (dataSaver->closed) {
                    std::cout << "Saving WorkoutSession in database" << std::endl;
                    id = GetUserId(dataSaver->Username);
                    if (id == -1) { AddUser(dataSaver->Username); }
                    id = GetUserId(dataSaver->Username);
                    if (dataSaver->numberOfReps != 0) {
                        sql = "INSERT INTO WorkoutSession (userId, indexOfSet, typeOfExercise, load, nbreOfReps, timesOfSet, velocitiesOfSet, coordinatesPointsProfile, usedForOneRM, date) VALUES (" + std::to_string(id) + ", " + std::to_string(dataSaver->id) + ", '" + dataSaver->TypeOfExercise + "', " + std::to_string(dataSaver->TotalWeightWanted) + ", " + std::to_string(dataSaver->numberOfReps) + ", '" + VectorToString(dataSaver->TimeOfSet) + "', '" + VectorToString(dataSaver->VelocityOfSet[0]) + "', '" + VectorOfVectorToString(dataSaver->profileShape) + "', '" + std::to_string(dataSaver->SetUsedForOneRMExperiment) + "', '" + dataSaver->Date + "');";
                        errMsg = nullptr;
                        rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
                        if (rc != SQLITE_OK) {
                            std::cout << "Failed to insert WorkoutSession in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
                            sqlite3_free(errMsg);
                        }
                    }
                }
            }
        }
        else {
            //Do not save the current analyser because it comes from a past workout session 
        }
    }
}

void TrainerDataManager::End() {
    sqlite3_close(db);
}