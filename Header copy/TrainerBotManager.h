#pragma once
#include <windows.h>

#include <time.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <iostream>


class Trainer;

class TrainerBotManager {
public:
    bool BotIsInitiated = false;

    int Rg = 2; // Either 1 or 2 wether we use clutch for high speed or for low speed
    double PI = 3.141592;
    double Rp = 51;	         //  M A w b h ̌      Reduction ratio of gear
    double n = 1.0;          //  ` B    
    double Kt = 0.192;	     //  g   N 萔[Nm/A]
    double Jm = 0.803482246;  //        [     g[kg m2]
    double Cm = 1.17065;      //  S   W  [Nm/(rad/s)]
    double num = 10;        //  ړ    ς̕W {   @ 
    double e = 2.7182818284; //    R ΐ  ̒ 
    double g = 9.81;         //  W { ̒  gravitationnal field
    double R = 0.1;      // Radius of the pulley is at 10 cm

    double AnalogOutput[2] = { 32768, 32768 }; // 32768 is the zero value of the output

    int ClutchModeSelected;
    int OldClutchModeSelected;
    bool WantTakeOffTheClutch = false;
    int TemporaryTotalWeightWanted; // This variable will hold the current value of the slider. It is the old INPUT_TORQUE variable
    int TotalWeightWanted; // This variable will hold the current value of the slider. It is the old INPUT_TORQUE variable
    int MaxOutputMotorTorque;

    //Contec board
    /* The Contec boards are interface boards using USB connection. We highly recommend to use them for the project.
     CLUTCH (If the robot uses clutches) :
         Black wire   -> B20 (Digital GND)
         White wire   -> B18 (Digital 00)
         Red   wire   -> B17 (Digital 01)
         Brown wire   -> B16 (Digital 02)
         Yellow wire  -> B15 (Digital 03)
     POTENTIOMETERS (If the robot uses digital encoders, the app would rather use encoders than potentiometers)
         Blue wire    -> A40 (Analog ground)      Left Potentiometer
         White wire   -> A44 (Analog 00)          Left Potentiometer output
         Grey wire    -> A39 (Analog ground)      Right Potentiometer
         Violet wire  -> A42 (Analog 01)          Right Potentiometer output
     MOTORS (In the code, 0 stands for left, 1 stands for right)
         Brown wire   -> A45 (Analog GND)         Left GND
         Yellow wire  -> A46 (Analog 01)          Left Command
         Orange wire  -> A47 (Analog GND)         Right GND
         Red   wire   -> A48 (Analog 02)          Right Command
     COUNTERS (If the robot uses digital encoders, the app would rather use encoders than potentiometers) In the code, 0 stands for left (Channel 1), 1 stands for right (Channel 2)
         Red wire     -> PCOM (+)			        Counter power supply
         Black wire   -> EQ N (GND)
         Red wire     -> EQ P (+)
         Yellow wire  -> A
         Blue wire    -> B*/
    bool UsingContecAIO = false;
    bool UsingContecCounter = false;
    long  Ret;								// 戻り値 return value
    short Id;								// Device ID for interface board
    short IdCNT; 						    // Device ID for Counter
    short ChNo[2] = { 0, 1 };				// listen counter on channel 0 and 1 (0 : left, 1: right)
    unsigned long CntDat[2];				// Counter data

    //LPC and PEX board
    /*LPC and PEX board are interface boards that don't use USB connection. They require a PCI slot in the computer. They were used in the past for the project. This is why we still keep the code for them.
     EVERY CODING LINE DEALING WITH PEX AND LPC IS THE WORK OF FORMER STUDENT NATSUO TOJO.
     LPC :
     PCI { [ hDA o ͏    ݒ
     Channel 0 :  E     [ ^
     Channel 1 :        [ ^
     PEX :
     PCI { [ hAD   ͏    ݒ
     Channel 0 : E | e   V     [ ^ potentiometer right
     Channel 1 :   | e   V     [ ^ potentiometer left
     Channel 2 :   p X C b ` Emergency switch
     Channel 3 : E   [ ^ d   l ɉ      o ͓d    tension de sortie en fonction du courant moteur droit
     Channel 4 :     [ ^ d   l ɉ      o ͓d    tension de sortie en fonction du courant moteur gauche
     Channel 5 : E     p X C b ` Right emergency switch if it has one
     Channel 6 :       p X C b ` Left emergency switch if it has one*/
     //For new robot : 24000 pulse/2pi. motoreduction : 1/15 
    bool UsingPEX = false;  //PEX is used as an Analog input output board
    bool UsingLPC = false;  //LPC is used to count the number of turns of the encoders


    //Measuring data
    std::chrono::steady_clock::time_point ExperimentT0 = std::chrono::steady_clock::now();;
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - ExperimentT0).count();
    double pre_t;
    double MotorTorque[2] = { 0, 0 };
    double deltaTheta[2];
    int PotentiometerRoundCounter[2] = { 0, 0 };
    int EncoderRoundCounter[2] = { 0, 0 };
    float PotentiometerOrigin[2];
    std::vector<std::vector<double>> ListPotentiometerRawVoltageValue; // [[leftValue, leftValue, ...], [rightValue, rightValue, ...]]
    double PrePotentiometerRelativeValue[2] = { 0, 0 };
    double AngleRelativeValueUsingPotentiometer[2];
    double initialCounterValue[2];
    double CounterValue[2];
    double PreCounterValue[2] = { 0, 0 };				// Previous counter data
    double AngleRelativeValueUsingCounter[2];
    double PreAngleRelativeValueUsingCounter[2];
    double DistanceRelativeValueUsingPotentiometer[2];
    double DistanceRelativeValueUsingCounter[2];
    double AngleVelocity[2];
    double preAngleVelocity[2];
    int BotProblem = 0;
    std::vector<std::vector<double>> ListAngleVelocity = { {},{} }; // [[leftValue, leftValue, ...], [rightValue, rightValue, ...]]    Useful for Kalman filter
    double KalmanPValueForVelocity[2] = { 0.1, 0.1 };   //Initial value for Kalman filter
    double AngleAcceleration[2];


    void Begin();
    void BotSetup();
    void InitializeBotWithCONTECBoards();
    void InitializeBotWithLPCAndPEXBoards();
    void ChangeClutchOption();
    void TakeOffTheClutch();
    void PutOnTheClutch();
    double MovingAverageAndExponentialSmoothing(std::vector<double>& oldSmoothedValues, double ValueToSmooth);
    std::vector<double> KalmanFilter(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P);
    std::vector<double> KalmanFilter2(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P);

    void BotStart();
    void MotorACOn();
    void MotorStart();
    void BotAquisitionUsingPEX();
    void BotAquisition();
    void BotCommand();
    void BotSaveData();
    void MotorStop();
    void BotStop();
    void BotKill();


    Trainer* TheTrainer;
    TrainerBotManager(Trainer* trainer) {
        this->TheTrainer = trainer;
        this->ClutchModeSelected = 0;
        this->OldClutchModeSelected = -1;
        this->TotalWeightWanted = 10;
        this->TemporaryTotalWeightWanted = this->TotalWeightWanted;
        this->MaxOutputMotorTorque = 40;
        BotSetup();
    }
};