#include "TrainerBotManager.h"
#include "Trainer.h"

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <string>
#include <iostream>
#include <time.h>
#include <chrono> // For std::chrono::seconds
#include <thread> // For std::this_thread::sleep_for
#include <math.h>
#include <numeric> //For accumulate
#include <algorithm> //For max
#include <MMsystem.h>
#include <iomanip>
#pragma comment(lib,"winmm.lib")
#include <io.h>
using namespace std;
#include <fstream>
#include <vector>

#include "CCnt.h" // Contec control 
#include "Caio.h" // Contec control 

#define PI          3.141592  
#define Rp			51	         // �M�A�w�b�h�̌����� Reduction ratio of gear
#define n			1.0          // �`�B����
#define Kt			0.192	     // �g���N�萔[Nm/A]
#define Jm          0.803482246  // �������[�����g[kg m2]
#define Cm          1.17065      // �S���W��[Nm/(rad/s)]
#define num         10           // �ړ����ς̕W�{���@ 
#define e           2.7182818284 // ���R�ΐ��̒�
#define g           9.81         // �W�{�̒� gravitationnal field
#define R           0.1          // Radius of the pulley is at 10 cm
//std::ofstream writing_file;   //Old file writing method
extern std::ofstream writing_file;

void TrainerBotManager::Begin() {
    TheTrainer->dataManager.Begin("data.csv");
    TheTrainer->dataManager.ConfigureDataNames({"ts", "ang_v_0", "ang_v_1", "ang_a_0", "ang_a_1", "distance_0", "distance_1", "estimate_power_kg[0]", "estimate_power_kg[1]", "estimate_power_both_kg", "estimate_power_N[0]", "estimate_power_N[1]", "estimate_power_both_N", "dennryu_Left", "dennryu_Right", "Loadcell_Left", "Loadcell_Right" });
}

void TrainerBotManager::ChangeClutchOption() {
    switch (ClutchModeSelected) {
    case 0://  �ᑬ�M������ gear low speed both sides
        Ret = AioOutputDoByte(Id, 0, 0x03); //Outputs (Bit 0, 1, 2, 3 are 0011) to port 0.
        Rg = 2;
        break;
    case 1: // �����M������ gear high speed both sides
        Ret = AioOutputDoByte(Id, 0, 0x0C); //Outputs (Bit 0, 1, 2, 3 are 1100) to port 0.
        Rg = 1;
        break;
    case 2://  �ᑬ�M������ gear low speed left side
        Ret = AioOutputDoByte(Id, 0, 0x02); //Outputs (Bit 0, 1, 2, 3 are 0010) to port 0.
        Rg = 2;
        break;
    case 3://  �ᑬ�M���E�� gear low speed right side
        Ret = AioOutputDoByte(Id, 0, 0x01); //Outputs (Bit 0, 1, 2, 3 are 0001) to port 0.
        Rg = 2;
        break;
    case 4: // �����M���E�� gear high speed right side
        Ret = AioOutputDoByte(Id, 0, 0x04); //Outputs (Bit 0, 1, 2, 3 are 0100) to port 0.
        Rg = 1;
        break;
    case 5: // �����M������ gear high speed left side
        Ret = AioOutputDoByte(Id, 0, 0x08); //Outputs (Bit 0, 1, 2, 3 are 1000) to port 0.
        Rg = 1;
        break;
    case 6: // �S�Œ� everything corrected ???
        Ret = AioOutputDoByte(Id, 0, 0x00); //Outputs (Bit 0, 1, 2, 3 are 0000) to port 0.
        Rg = 2;
        break;
    case 7: // �S���� errase all ???
        Ret = AioOutputDoByte(Id, 0, 0x0f); //Outputs (Bit 0, 1, 2, 3 are 1111) to port 0.
        Rg = 1;
        break;
    default:
        break;
    }
    char line[30];
    sprintf(line, "Clutch Mode Changed to option %d", ClutchModeSelected+1);
    TheTrainer->app.DeleteRobotConsoleMessage("Clutch Mode Changed to option");
    TheTrainer->app.AddRobotConsoleMessage(line);
}

void TrainerBotManager::TakeOffTheClutch() {
    Ret = AioOutputDoByte(Id, 0, 0x00); //Outputs (Bit 0, 1, 2, 3 are 0000) to port 0.
    WantTakeOffTheClutch = true;
}

void TrainerBotManager::PutOnTheClutch() {
    ChangeClutchOption();
}

double TrainerBotManager::MovingAverageAndExponentialSmoothing(std::vector<double>& oldSmoothedValues, double ValueToSmooth) {
    /*Filter for 1D data to smooth the data measured
    * Makes an average on the few last points and applies weights on each points to consider the latest points way more than the older ones  
    */
    int window_size = (4 < oldSmoothedValues.size()) ? 4 : oldSmoothedValues.size(); //find the min between 4 and the size of the values
    double alpha = 0.5;
    double SmoothedValue = alpha * ValueToSmooth;
    double sum = alpha;

    if (oldSmoothedValues.size() > window_size + 1) {

        for (int i = 0; i < window_size; i++)
        {
            SmoothedValue += (1 - alpha) * std::exp(-i) * oldSmoothedValues[oldSmoothedValues.size() - i - 1]; //Exponential smoothing SmoothedValue = alpha * ValueAcquiredWeNeedToSmooth + Sum_i ( (1 - alpha) * exp(-i) * oldSmoothedValues[end - i] )
            sum += (1 - alpha) * std::exp(-i);
        }
        
        SmoothedValue = SmoothedValue / sum;
        
    }
    return SmoothedValue;
}

std::vector<double> TrainerBotManager::KalmanFilter2(std::vector<double>&oldSmoothedValues, double ValueToSmooth, double P) {
    /*Kalman filter for 1D data to smooth the data measured
    * Uses a linear prediction and the known previous value to smooth the data
    * https://towardsdatascience.com/kalman-filter-an-algorithm-for-making-sense-from-the-insights-of-various-sensors-fused-together-ddf67597f35e
    */

    double Q = 0.001; // Covariance du bruit de processus 0.001
    double Rcov = 0.01;  // Covariance du bruit de mesure 0.01
    //static double P = 1;  // Covariance d'erreur initiale
    double alpha = 0.7; // How much we trust the predicted value
    double K; // Gain de Kalman

    if (!oldSmoothedValues.empty()) {

        if (oldSmoothedValues.size() > 2) {
            // Time update
            double old_value = oldSmoothedValues[oldSmoothedValues.size() - 1];
            double slope = old_value - oldSmoothedValues[oldSmoothedValues.size() - 2];
            double intersect = old_value - slope;
            double predicted_value = slope * 2 + intersect;
            P = P + Q;

            // Mise à jour
            K = P / (P + Rcov);
            double smoothed_value = (alpha * predicted_value + (1 - alpha) * old_value) + K * (ValueToSmooth - (alpha* predicted_value + (1-alpha) * old_value));
            P = (1 - K) * P;

            return { smoothed_value, P };
        }
        else {
            return { ValueToSmooth, P };
        }
    }
    else {
	    return { ValueToSmooth, P };
	}
}

std::vector<double> TrainerBotManager::KalmanFilter(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P) {
    /*Kalman filter for 1D data to smooth the data measured
    */

    double Q = 0.001; // Covariance du bruit de processus
    double Rcov = 0.01;  // Covariance du bruit de mesure
    //static double P = 1;  // Covariance d'erreur initiale
    double K; // Gain de Kalman

    if (oldSmoothedValues.size() > 2) {
        // Time update
        double old_value = oldSmoothedValues[oldSmoothedValues.size() - 1];
        P = P + Q;

        // Mise à jour
        K = P / (P + Rcov);
        double smoothed_value = old_value + K * (ValueToSmooth - old_value);
        P = (1 - K) * P;

        return { smoothed_value, P };
    }
    else {
        return { ValueToSmooth, P };
    }
}

void TrainerBotManager::BotSetup() {
    // Using tutorial on Contec website 
    // https://www.contec.com/jp/support/guides-tutorials/daq-control/tutorial-analog-io/aio-swcs_swcpp_swvb_swpy/

    //Data init
    TheTrainer->dataManager.ConfigureDataName(34, "Motor Torque left");
    TheTrainer->dataManager.ConfigureDataName(35, "Motor Torque right");
    TheTrainer->dataManager.ConfigureDataName(36, "Distance Right");
    TheTrainer->dataManager.ConfigureDataName(37, "Angle Velocity Right");

    //COUNTER
    Ret = CntInit("CNT000", &IdCNT);
    Ret = CntStartCount(IdCNT, ChNo, 2); //Start counting on channel 0 and 1. 2 is the number of channels !!

    //INTERFACE BOARD
    Ret = AioInit("AIO000", &Id);
    ChangeClutchOption();

    /*The range is only changed by performing the function.
        - 10 to 10V : AiRange = 0 || PM10
        - 5 to 5V : AiRange = 1 || PM5
        - 2.5 to 2.5V : AiRange = 2 || PM25
        0 to 10V : AiRange = 50 || P10
        0 to 5V : AiRange = 51 || P5
        0 to 2.5V : AiRange = 53 || P25
        Initial value : AiRange = 0.
        Cannot set to each channel.*/
    //Set the Motors Analog Output pin range to -10V to 10V
    Ret = AioSetAoRange(Id, 00, PM10);
    Ret = AioSetAoRange(Id, 01, PM10);
    MotorStop();
     
    // Set the potentiometer pins range to -10 to 10V
    Ret = AioSetAiRange(Id, 00, P10);    //Potentiometer Left  A44 Ground A40
    Ret = AioSetAiRange(Id, 01, P10);    //Potentiometer Right A42 Ground A39

    // Set the clutch pins range to 0 to 1V
    //Ret = AioSetAoRange(Id, 0, P1);       // Set range from 0 to 1V
    //Ret = AioSetAoRange(Id, 1, P1);       // Set range from 0 to 1V
    //Ret = AioSetAoRange(Id, 2, P1);       // Set range from 0 to 1V
    //Ret = AioSetAoRange(Id, 3, P1);       // Set range from 0 to 1V

    AioSingleAiEx(Id, 00, &PotentiometerOrigin[0]); // Setting origin Potentiometer Left
    AioSingleAiEx(Id, 01, &PotentiometerOrigin[1]); // Setting origin Potentiometer Right
    ListPotentiometerRawVoltageValue = { {PotentiometerOrigin[0]} , {PotentiometerOrigin[1]} };
    cout << "Bot initiated" << endl;
    BotIsInitiated = true;
}

void TrainerBotManager::BotStart() {
    if (!BotIsInitiated) {
        BotSetup();
    }
    cout<<"Bot Start"<<endl;
    WantTakeOffTheClutch = false;
    TheTrainer->app.ClearPlot();
    ExperimentT0 = std::chrono::steady_clock::now();
	Ret = CntReadCount(IdCNT, ChNo, 2, CntDat);
    pre_t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    for (int i = 0; i < 2; i++){
		initialCounterValue[i] = CntDat[i];
        PreAngleRelativeValueUsingCounter[i] = 2 * PI * (CntDat[i] - initialCounterValue[i]) / (500 * Rp * Rg); // 500 is the precision of the counter (counting only upper fronts)
        ListAngleVelocity[i].push_back(0);
    }
}

void TrainerBotManager::BotAquisition() {
    t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    double dt = t - pre_t;

    float PotentiometerRawVoltageValue[2]; // Raw value of the potentiometer
    float PotentiometerRelativeValue[2]; // Relative value of the potentiometer
    

    for (int i = 0; i < 2; i++) {  //0 for left 1 for right
        int vector = (i == 0) ? 1 : -1; // 1 for left and -1 for right 
        short channel = (i == 0) ? 00 : 01; // 00 for left and 01 for right

        //MEASURING AND ANALYZING POTENTIOMETERS OUTPUT
        Ret = AioSingleAiEx(Id, channel, &PotentiometerRawVoltageValue[i]);
        PotentiometerRawVoltageValue[i] = KalmanFilter2(ListPotentiometerRawVoltageValue[i], PotentiometerRawVoltageValue[i], 1)[0];
        PotentiometerRelativeValue[i] = vector * (PotentiometerRawVoltageValue[i] - PotentiometerOrigin[i]);

        // There is a jump in Potentiometer measure because we turned the potentiometer out of the range...
        if (PotentiometerRelativeValue[i] - PrePotentiometerRelativeValue[i] > 2.5) { //... Too far to the right (The potentiometer made a full turn to the right) ...
            PotentiometerRoundCounter[i]--;
        }
        else if (PotentiometerRelativeValue[i] - PrePotentiometerRelativeValue[i] < -2.5) { //... Too far to the left (The potentiometer made a full turn to the left)
            PotentiometerRoundCounter[i]++;
        }

        AngleRelativeValueUsingPotentiometer[i] = (PotentiometerRelativeValue[i] * 360 / 4) + PotentiometerRoundCounter[i] * 360; // 340 degrees for 4V according to the datasheet
        DistanceRelativeValueUsingPotentiometer[i] = AngleRelativeValueUsingPotentiometer[i] * 0.1 + 65; // Pulley radius is 10cm. The bar on the holder is at 65cm from the floor approximately
        PrePotentiometerRelativeValue[i] = PotentiometerRelativeValue[i];
        ListPotentiometerRawVoltageValue[i].push_back(PotentiometerRawVoltageValue[i]);
        //END ANALYZING POTENTIOMETERS OUTPUT

        //MEASURING AND ANALYZING COUNTERS OUTPUT
        Ret = CntReadCount(IdCNT, ChNo, 2, CntDat);

        AngleRelativeValueUsingCounter[i] = 2 * PI * (CntDat[i] - initialCounterValue[i]) / (500 * Rp * Rg); // 500 is the precision of the counter (counting only upper fronts)
        deltaTheta[i] = AngleRelativeValueUsingCounter[i] - PreAngleRelativeValueUsingCounter[i];
        DistanceRelativeValueUsingCounter[i] = vector * ((float)Rg / 2) * 0.1 * AngleRelativeValueUsingCounter[i]; // Pulley radius is 10cm. Rg is the gear ratio
            //Calculating velocity with a kalman filter
        std::vector<double> results = KalmanFilter2(ListAngleVelocity[i], vector * deltaTheta[i] / dt, KalmanPValueForVelocity[i]);
        AngleVelocity[i] = results[0];
        KalmanPValueForVelocity[i] = results[1];
        AngleAcceleration[i] = vector * (AngleVelocity[i] - ListAngleVelocity[i].back()) / (dt);
        //END ANALYZING COUNTERS OUTPUT


        //To do at the end
        ListAngleVelocity[i].push_back(AngleVelocity[i]);
        PreAngleRelativeValueUsingCounter[i] = AngleRelativeValueUsingCounter[i];
        //Make all the pre_something = something
    }

    //MOTOR COMMAND
    BotCommand();

}

void TrainerBotManager::BotCommand() {
    double TorqueWanted[2] = { (TotalWeightWanted * g * R / 2) - 1, (TotalWeightWanted * g * R / 2) - 1 }; //in Nm. "-1" is the friction torque

    if (t >= 6) {
        // The following law has been determined by former student Natsuo Tojo
        double f_ang_v_0 = (AngleVelocity[0] >= 0) ? AngleVelocity[0] : 0;
        double f_ang_v_1 = (AngleVelocity[1] >= 0) ? AngleVelocity[1] : 0;
        double f_ang_a_0 = (AngleAcceleration[0] >= 0) ? AngleAcceleration[0] : 0;
        double f_ang_a_1 = (AngleAcceleration[1] >= 0) ? AngleAcceleration[1] : 0;

        if ((AngleVelocity[0] <= 0 && (deltaTheta[0] < 0 && deltaTheta[0] >= -1.2)) || (AngleVelocity[1] <= 0 && (deltaTheta[1] < 0 && deltaTheta[1] >= -1.2))) {
            MotorTorque[0] = TorqueWanted[0] + 5 + f_ang_v_1 * Cm + f_ang_a_1 * Jm; //5 is the torque needed to start the motor
            MotorTorque[1] = TorqueWanted[1] + 3 + f_ang_v_1 * Cm + f_ang_a_1 * Jm; //3 is the torque needed to start the motor
        }
        else if ((AngleVelocity[0] <= 0 && (deltaTheta[0] <= 0.4 && deltaTheta[0] >= 0)) || (AngleVelocity[1] <= 0 && (deltaTheta[1] <= 0.5 && deltaTheta[1] >= 0))) {
            MotorTorque[0] = TorqueWanted[0] + 8 + f_ang_v_1 * Cm + f_ang_a_1 * Jm; //8 is the torque needed to start the motor
            MotorTorque[1] = TorqueWanted[1] + 6 + f_ang_v_1 * Cm + f_ang_a_1 * Jm; //6 is the torque needed to start the motor
        }
        else {
            MotorTorque[0] = TorqueWanted[0];
            MotorTorque[1] = TorqueWanted[1];
        }
    }
    else if (6 > t && t >= 5) {
        MotorTorque[0] = TorqueWanted[0];
        MotorTorque[1] = TorqueWanted[1];
    }
    else { // if t < 5 time of response
        MotorTorque[0] = TorqueWanted[0] / (1 + pow(e, -0.8 * (t - 2))); //Gain 0.8 biais 5  
        MotorTorque[1] = TorqueWanted[0] / (1 + pow(e, -0.8 * (t - 1))); //Gain 0.8 biais 5 to match acceleration from left side and right side 
    }

    if (Rg == 2) {
        AnalogOutput[0] = (int)(115.89 * MotorTorque[0]) + 32768; // �� ���[�^�d��[V]
        AnalogOutput[1] = -(int)(125.97 * MotorTorque[1]) + 32768; // �� ���[�^�d��[V]
    }
    else if (Rg == 1) {
        AnalogOutput[0] = (int)(60.416 * (MotorTorque[0] - 5)) + 32768; // �� ���[�^�d��[V]����
        AnalogOutput[1] = -(int)(62.789 * (MotorTorque[1] - 5)) + 32768; // �� ���[�^�d��[V]����
    }

    Ret = AioSingleAo(Id, 0, AnalogOutput[0]);
    Ret = AioSingleAo(Id, 1, AnalogOutput[1]);

    BotSaveData();
}

void TrainerBotManager::BotSaveData() {
    TheTrainer->dataManager.AddData(0, t); // Time
    TheTrainer->dataManager.AddData(34, MotorTorque[0]); // Relative Motor Torque left
    TheTrainer->dataManager.AddData(35, MotorTorque[1]); // Relative Motor Torque right
    TheTrainer->dataManager.AddData(36, DistanceRelativeValueUsingCounter[1]); // Relative Distance Right
    TheTrainer->dataManager.AddData(37, AngleVelocity[1]); // Relative Angle Velocity Right
}

void TrainerBotManager::MotorStop() {
    Ret = AioSingleAo(Id, 0, 32768); //left motor at zero
    Ret = AioSingleAo(Id, 1, 32768); //right motor at zero
    TheTrainer->dataManager.AddData(34, 0); // Relative Motor Torque left
    TheTrainer->dataManager.AddData(35, 0); // Relative Motor Torque right
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void TrainerBotManager::BotStop() {
    cout << "Bot Stop" << endl;
    MotorStop();
    TakeOffTheClutch();
}

void TrainerBotManager::BotKill() {
    BotStop();
    Ret = AioExit(Id);					    // Disable access to the AIO device
    Ret = CntExit(IdCNT);                   // Disable access to the CNT device      
    BotIsInitiated = false;
}