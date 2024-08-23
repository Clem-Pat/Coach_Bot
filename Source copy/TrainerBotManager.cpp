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

//CONTEC BOARDS CONTROL
#include "CCnt.h" // Contec control 
#include "Caio.h" // Contec control 

//LPC AND PEX BOARDS CONTROL
#include "GPC3300\include\Fbida.h"
#include "GPC3100\include\Fbiad.h"
#include "GPC6204\Include\FbiPenc.h"
#include "GPC6320\include\IFHScnt.h"
#include "GPC6320\include\IFUcnt.h"


//std::ofstream writing_file;   //Old file writing method
extern std::ofstream writing_file;

//We cannot include the GPCxxxx.h files in the header file because of the circular dependencies problem. This is why we put them here and declare those variables here. This is not clean. TODO : find a way to put them in header file
HANDLE      hDeviceHandleDa;
DASMPLCHREQ DaSmplChReq[2];
WORD        DawData[2];
HANDLE      hDeviceHandleAd;
ADSMPLCHREQ AdSmplChReq[8];
WORD        AdwData[8]; // 16bit ̕   
HANDLE      hDeviceHandlePls;
LARGE_INTEGER freq;
DWORD	OutData;
DWORD preValue[2] = { 0x50000,0x50000 };
DWORD Value[2];
double pre_ang_v[2];
double pre_distance[2];
double ang_a_ave[2][10] = { 0 };
double theta_deg[2] = { 0 };
double theta_diff[2] = { 0 };

void TrainerBotManager::Begin() {
    TheTrainer->dataManager.Begin("data.csv");
    //TheTrainer->dataManager.ConfigureDataNames({ "ts", "ang_v_0", "ang_v_1", "ang_a_0", "ang_a_1", "distance_0", "distance_1", "estimate_power_kg[0]", "estimate_power_kg[1]", "estimate_power_both_kg", "estimate_power_N[0]", "estimate_power_N[1]", "estimate_power_both_N", "dennryu_Left", "dennryu_Right", "Loadcell_Left", "Loadcell_Right" });
}

void TrainerBotManager::ChangeClutchOption() {
    if (UsingContecAIO) {
        if (ClutchModeSelected != OldClutchModeSelected) {
            switch (ClutchModeSelected) {
            case 0://   ᑬ M       gear low speed both sides
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x03); } //Outputs (Bit 0, 1, 2, 3 are 0011) to port 0.
                Rg = 2;
                break;
            case 1: //      M       gear high speed both sides
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x0C); } //Outputs (Bit 0, 1, 2, 3 are 1100) to port 0.
                Rg = 1;
                break;
            case 2://   ᑬ M       gear low speed left side
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x02); } //Outputs (Bit 0, 1, 2, 3 are 0010) to port 0.]
                Rg = 2;
                break;
            case 3://   ᑬ M   E   gear low speed right side
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x01); } //Outputs (Bit 0, 1, 2, 3 are 0001) to port 0.
                Rg = 2;
                break;
            case 4: //      M   E   gear high speed right side
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x04); } //Outputs (Bit 0, 1, 2, 3 are 0100) to port 0.
                Rg = 1;
                break;
            case 5: //      M       gear high speed left side
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x08); } //Outputs (Bit 0, 1, 2, 3 are 1000) to port 0.
                Rg = 1;
                break;
            case 6: //  S Œ  everything corrected ???
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x00); } //Outputs (Bit 0, 1, 2, 3 are 0000) to port 0.
                Rg = 2;
                break;
            case 7: //  S     errase all ???
                if (UsingContecAIO) { Ret = AioOutputDoByte(Id, 0, 0x0f); } //Outputs (Bit 0, 1, 2, 3 are 1111) to port 0.
                Rg = 1;
                break;
            default:
                break;
            }
            char line[30];
            sprintf(line, "Clutch Mode Changed to option %d", ClutchModeSelected + 1);
            TheTrainer->app.DeleteRobotConsoleMessage("Clutch Mode Changed to option");
            TheTrainer->app.AddRobotConsoleMessage(line);
            OldClutchModeSelected = ClutchModeSelected;
        }
    }
}

void TrainerBotManager::TakeOffTheClutch() {
    if (UsingContecAIO) Ret = AioOutputDoByte(Id, 0, 0x00); //Outputs (Bit 0, 1, 2, 3 are 0000) to port 0.
    WantTakeOffTheClutch = true;
}

void TrainerBotManager::PutOnTheClutch() {
    OldClutchModeSelected = -1;
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

std::vector<double> TrainerBotManager::KalmanFilter2(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P) {
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

            // Updating the value
            K = P / (P + Rcov);
            double smoothed_value = (alpha * predicted_value + (1 - alpha) * old_value) + K * (ValueToSmooth - (alpha * predicted_value + (1 - alpha) * old_value));
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

void TrainerBotManager::InitializeBotWithLPCAndPEXBoards() {
    //IF NO CONTEC BOARD IS FOUND, WE LOOK FOR THEIR EQUIVALENT (LPC for Counter AND PEX for AIO)
    if (!UsingContecCounter) { //If no CONTEC counter board is found, we look for LPC board to count the number of turns of the encoders
        bool LPCFailure = false;
        hDeviceHandlePls = UcntOpen((LPCTSTR)"IFUCNT1");
        if (hDeviceHandlePls == INVALID_HANDLE_VALUE) {
            std::cout << "IFUCNT1 failure" << std::endl;
            LPCFailure = true;
        }
        int nChannel;
        unsigned long dwChSel;
        unsigned long dwCountMode;
        unsigned long dwLoadMode;
        unsigned long dwLatchMode;
        unsigned long dwLoadData;
        unsigned long dwCounter[4];
        // Configure the pulse counter mode.
        // When the phase-shifted pulse count mode and phase Z are enabled, the counter clear and no counter latch.
        dwCountMode = IFUCNT_COUNT_PHASE_4 | IFUCNT_DIR_NORMAL;
        dwLoadMode = 0x00;
        dwLatchMode = 0x00;
        int nRet;
        nRet = UcntSetPulseCountMode(hDeviceHandlePls, 1, dwCountMode, dwLoadMode, dwLatchMode);
        nRet = UcntSetPulseCountMode(hDeviceHandlePls, 2, dwCountMode, dwLoadMode, dwLatchMode);
        //      l ݒ 
        dwCounter[0] = 10000000;
        dwCounter[1] = 10000000;
        nRet = UcntSetCounter(hDeviceHandlePls, 0x03, dwCounter);

        if (LPCFailure) {
            std::cout << "No LPC board found. This means no interface board has been found to count the number of turns of the encoders" << endl;
            TheTrainer->app.AddRobotConsoleError("No counter board found");
        }
        else {
            UsingLPC = true;
            std::cout << "LPC board found" << endl;
        }
    }
    if (!UsingContecAIO) { //If no CONTEC AIO board is found, we look for PEX board to control the motors
        bool PEXFailure = false;
        //DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);
        //int nRet = AdOutputDO(hDeviceHandleAd, 0x00);
        hDeviceHandleDa = DaOpen((LPCTSTR)"FBIDA1");
        if (hDeviceHandleDa == INVALID_HANDLE_VALUE) {
            std::cout << "FBIDA1 failure" << endl;
            PEXFailure = true;
        }
        // channle 0 settings
        DaSmplChReq[0].ulChNo = 1;
        DaSmplChReq[0].ulRange = DA_10V;
        // channle 1 settings
        DaSmplChReq[1].ulChNo = 2;
        DaSmplChReq[1].ulRange = DA_10V;
        hDeviceHandleAd = AdOpen((LPCTSTR)"FBIAD1");
        if (hDeviceHandleAd == INVALID_HANDLE_VALUE) {
            std::cout << "FBIAD1" << endl;
            PEXFailure = true;
        }
        //     Ch    ͂̂  ߂ɃX L     N   b N  ύX
        ADBMSMPLREQ AdBmSmplConfig;
        ADBOARDSPEC AdBoardSpec;
        AdBmGetSamplingConfig(hDeviceHandleAd, &AdBmSmplConfig);
        int nRet = AdGetDeviceInfo(hDeviceHandleAd, &AdBoardSpec);
        //  V   O   G   h7 `     l   A }10V     W ŃT   v     O
        AdBmSmplConfig.fScanFreq = 14000;  // fSmplFreq * ulChCount <= fScanFreq 
        AdBmSetSamplingConfig(hDeviceHandleAd, &AdBmSmplConfig);
        //  T   v     O      ݒ 
        AdSmplChReq[0].ulChNo = 1;
        AdSmplChReq[0].ulRange = AD_10V;
        AdSmplChReq[1].ulChNo = 2;
        AdSmplChReq[1].ulRange = AD_10V;
        AdSmplChReq[2].ulChNo = 3;
        AdSmplChReq[2].ulRange = AD_10V;
        AdSmplChReq[3].ulChNo = 4;
        AdSmplChReq[3].ulRange = AD_10V;
        AdSmplChReq[4].ulChNo = 5;
        AdSmplChReq[4].ulRange = AD_10V;
        AdSmplChReq[5].ulChNo = 6;
        AdSmplChReq[5].ulRange = AD_10V;
        AdSmplChReq[6].ulChNo = 7;
        AdSmplChReq[6].ulRange = AD_10V;
        AdSmplChReq[7].ulChNo = 8;
        AdSmplChReq[7].ulRange = AD_10V;

        if (PEXFailure) {
            std::cout << "No PEX board found. This means no interface board has been found to control the motors" << endl;
            TheTrainer->app.AddRobotConsoleError("No interface board found to control the motors");
        }
        else {
            Rg = 1;     //There is no clutch in the new robot so Rg is set as 1
            Rp = 15;    //reduction ratio at the end of the motor is 15
            R = 0.05;   //radius of the pulley on the new robot is 5cm
            OutData = 0x22;
            AdOutputDO(hDeviceHandleAd, OutData);

            DawData[0] = 32768;
            DawData[1] = 32768;
            DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);

            UsingPEX = true;
            std::cout << "PEX board found" << endl;
        }
    }
    if (UsingLPC || UsingPEX) {
        QueryPerformanceFrequency(&freq);
    }
}

void TrainerBotManager::InitializeBotWithCONTECBoards() {
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
    if (UsingContecAIO) {
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
        std::cout << "Bot initiated with AIO Contec" << endl;
    }
}

void TrainerBotManager::BotSetup() {
    // Using tutorial on Contec website 
    // https://www.contec.com/jp/support/guides-tutorials/daq-control/tutorial-analog-io/aio-swcs_swcpp_swvb_swpy/

    //Data init
    //TheTrainer->dataManager.ConfigureDataName(32, "CounterValue left");
    //TheTrainer->dataManager.ConfigureDataName(33, "CounterValue right");
    //TheTrainer->dataManager.ConfigureDataName(34, "Motor Torque left");
    //TheTrainer->dataManager.ConfigureDataName(35, "Motor Torque right");
    TheTrainer->dataManager.ConfigureDataName(36, "Distance Left");
    TheTrainer->dataManager.ConfigureDataName(37, "Angle Velocity Left");
    TheTrainer->dataManager.ConfigureDataName(38, "Angle Acceleration Left");
    TheTrainer->dataManager.ConfigureDataName(39, "Distance Right");
    TheTrainer->dataManager.ConfigureDataName(40, "Angle Velocity Right");
    TheTrainer->dataManager.ConfigureDataName(41, "Angle Acceleration Right");

    //FIND CONTEC BOARDS
    //CONTEC COUNTER
    Ret = CntInit("CNT000", &IdCNT);
    if (Ret == 0) {
        UsingContecCounter = true;
        Ret = CntStartCount(IdCNT, ChNo, 2); //Start counting on channel 0 and 1. "2" is the number of channels !!
    }
    //CONTEC INTERFACE BOARD
    Ret = AioInit("AIO000", &Id);
    if (Ret == 0) {
        UsingContecAIO = true;
        ChangeClutchOption(); //Set the clutch to the default option
    }
    if (!UsingContecAIO || !UsingContecCounter) { //If one of the CONTEC boards is missing, we look for their equivalent (LPC for Counter AND PEX for AIO)
        InitializeBotWithLPCAndPEXBoards();
    }
    if (UsingContecAIO || UsingContecCounter) {  //If we have at least one CONTEC board, we initialize it
        InitializeBotWithCONTECBoards();
    }

    if (UsingContecAIO || UsingPEX) { //If we have one of the interface board, we can start the bot
        //TODO : Implement the PEX board
        std::cout << "Bot initiated with: \n"
            << (UsingContecAIO ? "    - ContecAIO\n" : "")
            << (UsingPEX ? "    - PEX\n" : "")
            << (UsingContecCounter ? "    - ContecCounter\n" : "")
            << (UsingLPC ? "    - LPC\n" : "")
            << std::endl;
        BotIsInitiated = true;
    }
    else {
        std::cout << "AIO interface board is missing (please connect either Contec or PEX)" << std::endl;
        BotIsInitiated = false;
    }
}

void TrainerBotManager::BotStart() {
    if (!BotIsInitiated) {
        BotSetup();
    }
    std::cout << "Bot Start" << endl;
    WantTakeOffTheClutch = false;
    if (UsingContecAIO) PutOnTheClutch();
    TheTrainer->app.ClearPlot();
    ExperimentT0 = std::chrono::steady_clock::now();
    pre_t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    if (UsingContecCounter) {
        Ret = CntReadCount(IdCNT, ChNo, 2, CntDat);
    }
    unsigned long Value2;
    if (UsingLPC) {
        Ret = UcntStartCount(hDeviceHandlePls, 0x03, IFUCNT_CMD_START);
        UcntReadCounter(hDeviceHandlePls, 0x03, Value);
        for (int i = 0; i < 2; i++) { CntDat[i] = (double)Value[i]; }
    }
    for (int i = 0; i < 2; i++) {
        initialCounterValue[i] = CntDat[i];
        CounterValue[i] = CntDat[i];
        PreCounterValue[i] = CntDat[i];
        PreAngleRelativeValueUsingCounter[i] = 2 * PI * (CntDat[i] - initialCounterValue[i]) / (500 * Rp * Rg); // 500 is the precision of the counter (counting only upper fronts)
        ListAngleVelocity[i].push_back(0);
    }
    if (UsingPEX) {
        MotorACOn();
        MotorStart();
    }
}

void TrainerBotManager::MotorACOn() {
    OutData |= 0x08;
    OutData |= 0x80;
    AdOutputDO(hDeviceHandleAd, OutData);		// OUT4,8出力
    Sleep(2000);
    OutData &= ~0x08;
    OutData &= ~0x80;
    AdOutputDO(hDeviceHandleAd, OutData);
}

void TrainerBotManager::MotorStart() {
    if (UsingPEX) {
        OutData |= 0x04;
        OutData |= 0x40;
        AdOutputDO(hDeviceHandleAd, OutData);
    }
}

void TrainerBotManager::BotAquisitionUsingPEX() {
    //This function is a way to remember the method of former student Natsuo Tojo. It is not used in the current version of the robot
    t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    double dt = t - pre_t;

    if (UsingLPC) {
        UcntReadCounter(hDeviceHandlePls, 0x03, Value);
        CounterValue[0] = Value[0];
        CounterValue[1] = Value[1];
    }
    AdInputAD(hDeviceHandleAd, 8, AD_INPUT_DIFF, &AdSmplChReq[0], AdwData);


    // The following method is made by former student Natsuo Tojo. It might be a good idea but I (Clement Patrizio) could not make it work so I used my own method.
    for (int i = 0; i < 2; i++) {
        double vector = (i == 0) ? 1 : -1;//   ]     W  

        double delta_count[2] = { 0 };
        double delta_theta[2] = { 0 };
        // ړ    ς̏     
        for (int j = num - 1; j > 0; j--) ang_a_ave[i][j] = ang_a_ave[i][j - 1];// ߋ num 񕪂̒l   L ^

        // J E   g l 擾
        delta_count[i] = double(CounterValue[i]) - double(PreCounterValue[i]);
        //theta ̕ω    =  i  L ̃J E   g    4   {      ̃J E   g   j  2PI/ i M A w b h     䁖 N   b ` M A      )
        delta_theta[i] = 2.0 * PI * (delta_count[i] / 24000) / Rp; //Resolution of sensor is 24000 ticks per round
        
        AngleRelativeValueUsingCounter[i] = PreAngleRelativeValueUsingCounter[i] + vector * delta_theta[i];
        DistanceRelativeValueUsingCounter[i] = AngleRelativeValueUsingCounter[i] * R;

        //  p   x Z o [rad/s] 
        AngleVelocity[i] = vector * delta_theta[i] / (dt);
        //  p     x Z o[rad/s2]
        ang_a_ave[i][0] = (AngleVelocity[i] - preAngleVelocity[i]) / (dt);
        float a_ave = 0;
        for (int j = 0; j < num; j++) a_ave += ang_a_ave[i][j]; //   a     ߂  AngleAcceleration is the average of the last num values of ang_a_ave (moving average). TODO : Use KalmanFilter2 instead of moving average
        AngleAcceleration[i] = (float)a_ave / num;                     // W {   Ŋ    ĕ  ς    ߂ 
        pre_ang_v[i] = AngleVelocity[i];
        PreCounterValue[i] = CounterValue[i];
        PreAngleRelativeValueUsingCounter[i] = AngleRelativeValueUsingCounter[i];
        preAngleVelocity[i] = AngleVelocity[i];
    }

    //The Emergency switch option was used in the past when an emergency button was installed
    //double EmergencySwitchLeft = -20.00 * (double)(((double)AdwData[5] - 32768) / 65536);
    //double EmergencySwitchRight = -20.00 * (double)(((double)AdwData[6] - 32768) / 65536);
    //  ً} X C b ` ̃`     l    3.0V ȏ ̓d            烂 [ ^ [   X g b v    
    /*if ((EmergencySwitchLeft >= 3.0 || EmergencySwitchRight >= 3.0)) {
        BotStop();
        TheTrainer->app.AddRobotConsoleError("Emergency stop");
    }*/
}

void TrainerBotManager::BotAquisition() {
    t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    double dt = t - pre_t;

    float PotentiometerRawVoltageValue[2]; // Raw value of the potentiometer
    float PotentiometerRelativeValue[2]; // Relative value of the potentiometer

    //MEASURING AND ANALYZING POTENTIOMETERS OUTPUTn IF THE ROBOT HAS THEM (THE OLD ONE DOES)
    //In fact the potentiometers are not used for the final acquistion anymore because the counters are more efficient. But we still keep the code for a potential future use 
    if (UsingContecAIO) {
        for (int i = 0; i < 2; i++) {  //0 for left 1 for right
            int vector = (i == 0) ? 1 : -1; // 1 for left and -1 for right 
            short channel = (i == 0) ? 00 : 01; // 00 for left and 01 for right

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
        }
    }

    //MEASURING AND ANALYZING COUNTERS OUTPUT
    if (UsingContecCounter) {
        Ret = CntReadCount(IdCNT, ChNo, 2, CntDat);
        CounterValue[0] = CntDat[0];
        CounterValue[1] = CntDat[1];
    }
    if (UsingLPC) {
        UcntReadCounter(hDeviceHandlePls, 0x03, Value);
        CounterValue[0] = Value[0];
        CounterValue[1] = Value[1];
        AdInputAD(hDeviceHandleAd, 8, AD_INPUT_DIFF, &AdSmplChReq[0], AdwData);
    }
    for (int i = 0; i < 2; i++) {
        int vector = (i == 0) ? 1 : -1; // 1 for left and -1 for right 

        if (UsingContecCounter) {
            // There is a jump in encoder measure because we turned the encoder out of the range... BE CAREFUL, THIS PART COULD NOT HAVE BEEN TESTED BECAUSE THE ENCODERS ARE FAR FROM THE RANGE END. TODO : TEST IT
            if (CounterValue[i] - PreCounterValue[i] > 1000000) { //... Too far to the right (The potentiometer made a full turn to the right) ...
                EncoderRoundCounter[i]--;
            }
            else if (CounterValue[i] - PreCounterValue[i] < -1000000) { //... Too far to the left (The potentiometer made a full turn to the left)
                EncoderRoundCounter[i]++;
            }
            AngleRelativeValueUsingCounter[i] = 2 * PI * (CounterValue[i] + EncoderRoundCounter[i] * 4.29497e+09 - initialCounterValue[i]) / (500 * Rp * Rg); // 500 is the precision of the counter (counting only upper fronts). BE CAREFUL : NOT SURE ABOUT THE 4.29497e+09 VALUE. IT SEEMS TO BE THIS BUT IT HAS TO BE TESTED
            if (AngleRelativeValueUsingCounter[i] - PreAngleRelativeValueUsingCounter[i] > 100) { // This is the emergency if. If we miscalculated the angle, we keep the previous value. 
                CounterValue[i] = PreCounterValue[i];
                AngleRelativeValueUsingCounter[i] = PreAngleRelativeValueUsingCounter[i];
                BotProblem++;
                if (BotProblem == 4) { TheTrainer->WantToStopBot = true; } // If we have 4 problems in a row, we stop the bot by security
            }
        }
        if (UsingLPC) {
            AngleRelativeValueUsingCounter[i] = 2 * PI * (CounterValue[i] - initialCounterValue[i]) / (24000 * Rp); // 500 is the precision of the counter (counting only upper fronts). BE CAREFUL : NOT SURE ABOUT THE 4.29497e+09 VALUE. IT SEEMS TO BE THIS BUT IT HAS TO BE TESTED
        }
        deltaTheta[i] = AngleRelativeValueUsingCounter[i] - PreAngleRelativeValueUsingCounter[i];
        DistanceRelativeValueUsingCounter[i] = vector * ((float)Rg / 2) * R * AngleRelativeValueUsingCounter[i]; // Pulley radius is 10cm. Rg is the gear ratio
        //Calculating velocity with a kalman filter
        std::vector<double> results = KalmanFilter2(ListAngleVelocity[i], vector * deltaTheta[i] / dt, KalmanPValueForVelocity[i]);
        AngleVelocity[i] = results[0];
        KalmanPValueForVelocity[i] = results[1];
        AngleAcceleration[i] = vector * (AngleVelocity[i] - ListAngleVelocity[i].back()) / (dt);
        //END ANALYZING COUNTERS OUTPUT


        //To do at the end
        ListAngleVelocity[i].push_back(AngleVelocity[i]);
        PreAngleRelativeValueUsingCounter[i] = AngleRelativeValueUsingCounter[i];
        PreCounterValue[i] = CounterValue[i];
        //Make all the pre_something = something
    }
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
            MotorTorque[0] = TorqueWanted[0] + 3 + f_ang_v_0 * Cm + f_ang_a_0 * Jm; //5 is the torque needed to start the motor
            MotorTorque[1] = TorqueWanted[1] + 3 + f_ang_v_1 * Cm + f_ang_a_1 * Jm; //3 is the torque needed to start the motor
        }
        else if ((AngleVelocity[0] <= 0 && (deltaTheta[0] <= 0.4 && deltaTheta[0] >= 0)) || (AngleVelocity[1] <= 0 && (deltaTheta[1] <= 0.5 && deltaTheta[1] >= 0))) {
            MotorTorque[0] = TorqueWanted[0] + 6 + f_ang_v_0 * Cm + f_ang_a_0 * Jm; //8 is the torque needed to start the motor
            MotorTorque[1] = TorqueWanted[1] + 6 + f_ang_v_1 * Cm + f_ang_a_1 * Jm; //6 is the torque needed to start the motor
        }
        else {
            MotorTorque[0] = TorqueWanted[0];
            MotorTorque[1] = TorqueWanted[1];
        }
    }
    if (6 > t && t >= 5) {
        MotorTorque[0] = TorqueWanted[0];
        MotorTorque[1] = TorqueWanted[1];
    }
    else { // if t < 5 time of response
        MotorTorque[0] = TorqueWanted[0] / (1 + pow(e, -0.8 * (t - 1))); //Gain 0.8 biais 5  t-2
        MotorTorque[1] = TorqueWanted[1] / (1 + pow(e, -0.8 * (t - 2))); //Gain 0.8 biais 5 to match acceleration from left side and right side  t-1
    }


    if (UsingContecAIO) {
        if (Rg == 1) {
            AnalogOutput[0] = (int)(100 * (MotorTorque[0] - 5)) + 32768; //       [ ^ d  [V]     intially 190
            AnalogOutput[1] = -(int)(62.789 * (MotorTorque[1] - 5)) + 32768; //       [ ^ d  [V]     initially 125.97
        }
        else if (Rg == 2) {
            AnalogOutput[0] = (int)(190 * MotorTorque[0]) + 32768; //       [ ^ d  [V] initially 100
            AnalogOutput[1] = -(int)(125.97 * MotorTorque[1]) + 32768; //       [ ^ d  [V] initially 62.789
        }
        Ret = AioSingleAo(Id, 0, AnalogOutput[0]);
        Ret = AioSingleAo(Id, 1, AnalogOutput[1]);
    }
    if (UsingPEX) {
        DawData[0] = (WORD)((-254.42 * TotalWeightWanted + 32768) + 0.5);
        DawData[1] = (WORD)((254.42 * TotalWeightWanted + 32768) + 0.5);
        DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);
    }
}

void TrainerBotManager::BotSaveData() {
    TheTrainer->dataManager.AddData(0, t); // Time
    //TheTrainer->dataManager.AddData(32, CounterValue[0]); // Angular Velocity left
    //TheTrainer->dataManager.AddData(33, CounterValue[1]); // Angular Velocity left
    //TheTrainer->dataManager.AddData(34, MotorTorque[0]); // Relative Motor Torque left
    //TheTrainer->dataManager.AddData(35, MotorTorque[1]); // Relative Motor Torque right
    TheTrainer->dataManager.AddData(36, DistanceRelativeValueUsingCounter[0]); // Relative Distance Left
    TheTrainer->dataManager.AddData(37, AngleVelocity[0]); // Relative Angle Velocity Left
    TheTrainer->dataManager.AddData(38, AngleAcceleration[0]); // Relative Angle Acceleration Left
    TheTrainer->dataManager.AddData(39, DistanceRelativeValueUsingCounter[1]); // Relative Distance Right
    TheTrainer->dataManager.AddData(40, AngleVelocity[1]); // Relative Angle Velocity Right
    TheTrainer->dataManager.AddData(41, AngleAcceleration[1]); // Relative Angle Acceleration right
}

void TrainerBotManager::MotorStop() {
    if (UsingContecAIO) {
        Ret = AioSingleAo(Id, 0, 32768); //left motor at zero
        Ret = AioSingleAo(Id, 1, 32768); //right motor at zero
    }
    else if (UsingPEX) {
        DawData[0] = 32768; //   S ɂƂ߂邽 ߂ɐ  l ύX
        DawData[1] = 32768; //   S ɂƂ߂邽 ߂ɐ  l ύX
        DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);
        OutData &= ~0x04;
        OutData &= ~0x40;
        AdOutputDO(hDeviceHandleAd, OutData);
    }
    if (UsingLPC) { Ret = UcntStopCount(hDeviceHandlePls, 0x03, IFUCNT_CMD_STOP | IFUCNT_CMD_SAMPLING); }
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void TrainerBotManager::BotStop() {
    std::cout << "Bot Stop" << endl;
    MotorStop();
    TakeOffTheClutch();
}

void TrainerBotManager::BotKill() {
    BotStop();
    if (UsingContecAIO) { Ret = AioExit(Id); }					    // Disable access to the AIO device
    if (UsingContecCounter) { Ret = CntExit(IdCNT); }               // Disable access to the CNT device      
    if (UsingLPC) { UcntClose(hDeviceHandlePls); }
    if (UsingPEX) {
        DaClose(hDeviceHandleDa);
        AdClose(hDeviceHandleAd);
    }

    BotIsInitiated = false;
}