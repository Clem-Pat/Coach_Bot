#pragma once
#include <windows.h>

#include <time.h>
#include <fstream>
#include <vector>

#include "GPC3300\include\Fbida.h"
#include "GPC3100\include\Fbiad.h"
#include "GPC6204\Include\FbiPenc.h"
#include "GPC6320\include\IFHScnt.h"
#include "GPC6320\include\IFUcnt.h"


class Trainer;

class TrainerBotManager {
public:
    unsigned long timeC;
    unsigned long timeD;
    double pre_ts = 0;
    double dt_syori;
    LARGE_INTEGER timeA;
    LARGE_INTEGER timeB;
    LARGE_INTEGER timeDT;
    LARGE_INTEGER freq;            //�b�ɖ߂����ߗp
    MMRESULT TimerID;
    time_t t;
    struct tm* date;
    char fname[256];
    char fnames[256];
    FILE* fp;		               // �t�@�C���|�C���^�錾
    int cnt = 0;
    int Rg = 0;
    int emergency_switch;

    //�����͎Z�o�p 
    double POWER[2];

    //--PCI �A�i���O�o�͗p(�w�ߓd��)�ϐ��錾-----------//
    HANDLE      hDeviceHandleDa;
    DASMPLCHREQ DaSmplChReq[2];
    WORD        DawData[2];
    WORD        AnalogOutput[2];
    int DA_cnt = 0;

    //--PCI AD���͗p(�|�e���V�����[�^�p)�ϐ��錾-----------//
    HANDLE      hDeviceHandleAd;
    ADSMPLCHREQ AdSmplChReq[8];
    WORD        AdwData[8];// 16bit�̕���\

    //�|�e���V�����[�^ for potintiometer
    int         thetaAdData[2][5] = { 0 };
    int         thetaAd[2] = { 0 };
    double      theta_deg[2] = { 0 };
    double      theta_diff[2] = { 0 };

    double      theta[2];			  	   // �p�x[rad]
    double      p_theta[2];		     	   // 1�X�e�b�v�O�̊p�x
    double      theta_VData[2][5] = {};
    double      theta_V[2] = { 0 };		   // �p���x[rad/s]
    double      theta_AData[2][5] = {};
    double      p_theta_V[2] = { 0 };	   // 1�X�e�b�v�O�̊p���x
    double      theta_A[2] = { 0 };		   // �p�����x
    double      ang_a_ave[2][10] = { 0 }; //�p�����x�̈ړ����ϒl why 10 ? Because num is equal to 10


    //--PCI �p���X�J�E���g�p�ϐ��錾 (PCI for pulse count)----------------//
    HANDLE hDeviceHandlePls;
    int nRet;						       // �֐��߂�l
    DWORD Value[2];					       // �J�E���^�l�i�[�p
    DWORD preValue[2] = { 0x50000,0x50000 };   // �O�̃J�E���^�l�i�[�p
    double pre_ang_v[2];                   //�O�̊p���x�l�i�[�p
    double pre_distance[2];                //�O�̊p���x�l�i�[�p
    std::vector<double> distancesMeasured{}; //�p���x�̈ړ����ϒl
    bool WantRecordDistances = false;

    /*--�؊�������p�ϐ��錾 variables for cotrolling training load ---------------------------------------------//
    //-------------------------------------------------------------------*/
    char dir[10];           // �f�B���N�g����

    double  I[2] = { 0 };   //�w�ߓd���l
    double  Vc[2] = { 0 };  //�w�ߓd���l�ɑΉ�����w�ߓd���l

    int modecheck3;		//�M���I��
    //double timer, timer_start, now;		//�o�ߎ���[ms]�𑪒肷�邽��;
    double timer, timer_start;		//�o�ߎ���[ms]�𑪒肷�邽��;
    double torque_emit[2];		//�o�͕��� [Nm]
    double  V_static[2][10] = { 0 };
    double V_sum[2];

    time_t     now = time(0);
    struct tm  tstruct = *localtime(&now);
    char       datetime[80];
    char       filevalue1[80];
    char       filevalue2[80];
    char       filevalue3[80];

    int BotModeSelected;
    int InputMotorTorque; // This variable will hold the current value of the slider. It is the old INPUT_TORQUE variable
    int MaxOutputMotorTorque;

    Trainer* TheTrainer;
    TrainerBotManager(Trainer* trainer) {
        this->TheTrainer = trainer;
        this->BotModeSelected = 0;
        this->InputMotorTorque = 5;
        this->MaxOutputMotorTorque = 30;
    }

    void Begin();
    void DeviceSetup(void);
    void ChangeBotModeOption();
    void TakeOffTheClutch();
    void StartBot();
    void TimerProcedure();
    void MotorTorqueOutput(double ang_v_0, double ang_v_1, double ang_a_0, double ang_a_1);
    void RecordDistances();
    void StopBot();
    void StopMotor();
    void StopClutch();
    void StopCommunicationWithSystem();
    double MovingAverageAndExponentialSmoothing(std::vector<double>& oldSmoothedValues, double ValueToSmooth);
    std::vector<double> KalmanFilter(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P);
    std::vector<double> KalmanFilter2(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P);
};
