#include "TrainerBotManager.h"
#include "Trainer.h"

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <string>
#include <iostream>
#include <time.h>
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

#define PI          3.141592  
#define Rp			51	         // �M�A�w�b�h�̌�����
#define n			1.0          // �`�B����
#define Kt			0.192	     // �g���N�萔[Nm/A]
#define Jm          0.803482246  // �������[�����g[kg m2]
#define Cm          1.17065      // �S���W��[Nm/(rad/s)]
#define num         10           // �ړ����ς̕W�{���@ 
#define e           2.7182818284 // ���R�ΐ��̒�

//std::ofstream writing_file;   //Old file writing method
extern std::ofstream writing_file;

void CALLBACK TimerProc(TrainerBotManager, UINT, UINT, DWORD, DWORD, DWORD);
void CALLBACK Motor_Proofreading(UINT, UINT, DWORD, DWORD, DWORD);


void TrainerBotManager::Begin() {
    DawData[0] = 32768;
    DawData[1] = 32768;
    DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);
    nRet = AdOutputDO(hDeviceHandleAd, 0x00);
    //Sleep(2000);

    // ---------------------------
    //�f�o�C�X�̕ϐ�������	Initializing the machine
    // ---------------------------
    DeviceSetup();
    TheTrainer->dataManager.Begin("data.csv");
    TheTrainer->dataManager.ConfigureDataNames({"ts", "ang_v_0", "ang_v_1", "ang_a_0", "ang_a_1", "distance_0", "distance_1", "estimate_power_kg[0]", "estimate_power_kg[1]", "estimate_power_both_kg", "estimate_power_N[0]", "estimate_power_N[1]", "estimate_power_both_N", "dennryu_Left", "dennryu_Right", "Loadcell_Left", "Loadcell_Right" });
}

void TrainerBotManager::DeviceSetup() {
    DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);
    nRet = AdOutputDO(hDeviceHandleAd, 0x00);
    //-----------------------------------------
    // PCI�{�[�hDA�o�͏����ݒ�
    // Channel 0 : �E�����[�^
    // Channel 1 : �������[�^
    //-----------------------------------------
    hDeviceHandleDa = DaOpen((LPCTSTR)"FBIDA1");
    if (hDeviceHandleDa == INVALID_HANDLE_VALUE) {
        TheTrainer->app.AddRobotConsoleError("The FBIDA1 failure");
    }
    // channle 0 settings
    DaSmplChReq[0].ulChNo = 1;
    DaSmplChReq[0].ulRange = DA_10V;
    // channle 1 settings
    DaSmplChReq[1].ulChNo = 2;
    DaSmplChReq[1].ulRange = DA_10V;

    //-----------------------------------------
    // PCI�{�[�hAD���͏����ݒ�
    // Channel 0 :�E�|�e���V�����[�^ potentiometer right
    // Channel 1 :���|�e���V�����[�^ potentiometer left
    // Channel 2 :���p�X�C�b�` interrupteur d'urgence
    // Channel 3 :�E���[�^�d���l�ɉ������o�͓d��  tension de sortie en fonction du courant moteur droit 
    // Channel 4 :�����[�^�d���l�ɉ������o�͓d��  tension de sortie en fonction du courant moteur gauche
    // Channel 5 :�E�����p�X�C�b�` interrupteur d'urgence cote droit
    // Channel 6 :�������p�X�C�b�` interrupteur d'urgence cote gauche
    //-----------------------------------------
    hDeviceHandleAd = AdOpen((LPCTSTR)"FBIAD1");
    if (hDeviceHandleAd == INVALID_HANDLE_VALUE) {
        TheTrainer->app.AddRobotConsoleError("The FBIAD1 failure");
    }
    // ����Ch ���͂̂��߂ɃX�L�����N���b�N��ύX
    ADBMSMPLREQ AdBmSmplConfig;
    ADBOARDSPEC AdBoardSpec;
    AdBmGetSamplingConfig(hDeviceHandleAd, &AdBmSmplConfig);
    nRet = AdGetDeviceInfo(hDeviceHandleAd, &AdBoardSpec);
    // �V���O���G���h7�`�����l���A�}10V�����W�ŃT���v�����O
    AdBmSmplConfig.fScanFreq = 14000;  // fSmplFreq * ulChCount <= fScanFreq 
    AdBmSetSamplingConfig(hDeviceHandleAd, &AdBmSmplConfig);
    // �T���v�����O������ݒ�
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

    //-----------------------------------------
    // PCI�{�[�h�p���X�J�E���g�����ݒ�
    // �f�o�C�X��"FBIPENC81"��3 ���[�h�p���X�J�E���^�̏�����
    //�`�����l�����p���X�J�E���g ���́A4 ���{�A�񓯊��N���A�A�A�b�v�J�E���g�A��v���o�����A�\�t�g�E�F�A���b�`�ɐݒ肵�܂�
    //-----------------------------------------
    hDeviceHandlePls = UcntOpen((LPCTSTR)"IFUCNT1");
    if (hDeviceHandlePls == INVALID_HANDLE_VALUE) {
        TheTrainer->app.AddRobotConsoleError("The IFUCNT1 failure");
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
    nRet = UcntSetPulseCountMode(hDeviceHandlePls, 1, dwCountMode, dwLoadMode, dwLatchMode);
    nRet = UcntSetPulseCountMode(hDeviceHandlePls, 2, dwCountMode, dwLoadMode, dwLatchMode);
    // �����l�ݒ�
    dwCounter[0] = 10000000;
    dwCounter[1] = 10000000;
    nRet = UcntSetCounter(hDeviceHandlePls, 0x03, dwCounter);

    //-------------------------------------------
    // �^�C�}�[�֐�(QueryPerformanceFrequency)�̏�����
    //-------------------------------------------
    QueryPerformanceFrequency(&freq);
}

void TrainerBotManager::StartBot() {
    // ---------------------------
    //�g���[�j���O�J�n Start experiment 
    // ---------------------------
    nRet = UcntStartCount(hDeviceHandlePls, 0x03, IFUCNT_CMD_START);
    QueryPerformanceCounter(&timeA);
    QueryPerformanceCounter(&timeB);
    UcntReadCounter(hDeviceHandlePls, 0x03, preValue);
    // Timer Interrupt Process
    //���荞�݊Ԋu���Ƃ�timer_handle�֐������s
    printf("\n�^�C�}�[���荞�݊J�n");
    //TimerID = timeSetEvent(10,//���荞�݊Ԋu[ms]
    //    1,// ���荞�ݐ��x[ms]
    //    (LPTIMECALLBACK)TimerProc,//�R�[���o�b�N�֐� 
    //    NULL,//���荞�ݏ����ւ̈��� 
    //    TIME_PERIODIC// �J��Ԃ�,���荞�ݏ���
    //);
}

void TrainerBotManager::ChangeBotModeOption() {
    switch (BotModeSelected) {
    case 1://  �ᑬ�M������ gear low speed both sides
        nRet = AdOutputDO(hDeviceHandleAd, 0x03); //0000 0011
        Rg = 2;
        break;
    case 2: // �����M������ gear high speed both sides
        nRet = AdOutputDO(hDeviceHandleAd, 0x0C); //0000 1100
        Rg = 1;
        break;
    case 3://  �ᑬ�M������ gear low speed left side
        nRet = AdOutputDO(hDeviceHandleAd, 0x02); //0000 0010
        Rg = 2;
        break;
    case 4://  �ᑬ�M���E�� gear low speed right side
        nRet = AdOutputDO(hDeviceHandleAd, 0x01); //0000 0001
        Rg = 2;
        break;
    case 5: // �����M���E�� gear high speed right side
        nRet = AdOutputDO(hDeviceHandleAd, 0x04); //0000 0100�@�@
        Rg = 1;
        break;
    case 6: // �����M������ gear high speed left side
        nRet = AdOutputDO(hDeviceHandleAd, 0x08); //0000 1000
        Rg = 1;
        break;
    case 7: // �S�Œ� everything corrected ???
        nRet = AdOutputDO(hDeviceHandleAd, 0x00); //0000 0000
        Rg = 2;
        break;
    case 8: // �S���� errase all ???
        nRet = AdOutputDO(hDeviceHandleAd, 0x0f); //0000 1111
        Rg = 1;
        break;
    default:
        break;
    }
    char line[30];
    sprintf(line, "Bot Mode Changed to option %d", BotModeSelected+1);
    TheTrainer->app.DeleteRobotConsoleMessage("Bot Mode Changed to option");
    TheTrainer->app.AddRobotConsoleMessage(line);
}

void TrainerBotManager::TakeOffTheClutch() {
    nRet = AdOutputDO(hDeviceHandleAd, 0x00);
}

void TrainerBotManager::MotorTorqueOutput(double ang_v_0, double ang_v_1, double ang_a_0, double ang_a_1) {
    //-----------------------
    //���[�^����p�֐�
    //-----------------------
    double torque_output[2] = { 0,0 };
    double motor_speed[2] = { 0,0 };
    double motor_torque[2] = { 0,0 };
    double estimate_power_kg[2] = { 0,0 };
    double estimate_power_both_kg;
    double estimate_power_N[2] = { 0,0 };
    double estimate_power_both_N;

    // �|�e���V�����[�^�̉�]�p�x����A�[���̏o�̓g���N�𐄒肷��
    torque_output[0] = ((double)InputMotorTorque / 2) - 1; // �� [Nm] ���a10cm
    torque_output[1] = ((double)InputMotorTorque / 2) - 1; // �E [Nm] ���a10cm

    // �팱�҂̎��ɂ�����g���N�ɕϊ�����
    // �␳���ԕ��� 3�b�͑��x�p���x�I�t
    QueryPerformanceCounter(&timeA);
    double ts = (double)(timeA.QuadPart - timeB.QuadPart) / freq.QuadPart;
    if (ts >= 6) {
        // �p���x����ъp�����x��0�ȏ�ɂȂ�悤�ɏ������򂷂�
        double f_ang_v_0 = (ang_v_0 >= 0) ? ang_v_0 : 0;
        double f_ang_v_1 = (ang_v_1 >= 0) ? ang_v_1 : 0;
        double f_ang_a_0 = (ang_a_0 >= 0) ? ang_a_0 : 0;
        double f_ang_a_1 = (ang_a_1 >= 0) ? ang_a_1 : 0;

        if ((ang_v_0 <= 0 && (theta_diff[0] < 0 && theta_diff[0] >= -1.2)) || (ang_v_1 <= 0 && (theta_diff[1] < 0 && theta_diff[1] >= -1.2))) {
            motor_torque[0] = torque_output[0] + 5 + f_ang_v_0 * Cm + f_ang_a_0 * Jm;
            motor_torque[1] = torque_output[1] + 3 + f_ang_v_0 * Cm + f_ang_a_0 * Jm;
        }
        else if ((ang_v_0 <= 0 && (theta_diff[0] <= 0.4 && theta_diff[0] >= 0)) || (ang_v_1 <= 0 && (theta_diff[1] <= 0.5 && theta_diff[1] >= 0))) {
            motor_torque[0] = torque_output[0] + 8 + f_ang_v_0 * Cm + f_ang_a_0 * Jm;
            motor_torque[1] = torque_output[1] + 6 + f_ang_v_0 * Cm + f_ang_a_0 * Jm;
        }
        else {
            motor_torque[0] = torque_output[0];
            motor_torque[1] = torque_output[1];
        }
    }
    else if (6 > ts && ts >= 5) {
        motor_torque[0] = torque_output[0];
        motor_torque[1] = torque_output[1];
    }
    else {
        motor_torque[0] = torque_output[0] / (1 + pow(e, -0.8 * (ts - 2))); //�Q�C��0.8 �o�C�A�X5
        motor_torque[1] = torque_output[0] / (1 + pow(e, -0.8 * (ts - 1))); //�Q�C��0.8 �o�C�A�X4 //���E�ŉ��������킹�邽��
    }

    // �A�[���̃g���N�o�͂���o�͓d���𐄒�
    // �M�A�䗦�@����1�@�ᑬ2
    if (Rg == 1) {
        AnalogOutput[0] = (int)(115.89 * motor_torque[0]) + 32768; // �� ���[�^�d��[V]
        AnalogOutput[1] = -(int)(125.97 * motor_torque[1]) + 32900; // �E ���[�^�d��[V]
    }
    else if (Rg == 2) {
        AnalogOutput[0] = (int)(60.416 * (motor_torque[0] - 5)) + 32768; // �� ���[�^�d��[V]����
        AnalogOutput[1] = -(int)(62.787 * (motor_torque[1] - 5)) + 32770; // �E ���[�^�d��[V]
    }

    estimate_power_kg[0] = (ang_v_0 * (Cm)+ang_a_0 * (Jm - 0.5)) / 0.1 / 9.81 + (double)InputMotorTorque / 2;//�E�r������
    estimate_power_kg[1] = (ang_v_1 * (Cm)+ang_a_1 * (Jm - 0.5)) / 0.1 / 9.81 + (double)InputMotorTorque / 2;//���r������
    estimate_power_both_kg = estimate_power_kg[0] + estimate_power_kg[1]; //���r������
    estimate_power_N[0] = estimate_power_kg[0] * 9.81;
    estimate_power_N[1] = estimate_power_kg[1] * 9.81;
    estimate_power_both_N = estimate_power_N[0] + estimate_power_N[1];

    //�d���l
    double dennryu_Left = -20.00 * (double)(((double)AdwData[3] - 32768) / 65536);
    double dennryu_Right = -20.00 * (double)(((double)AdwData[4] - 32768) / 65536);
    double Loadcell_Left = -20.00 * (double)(((double)AdwData[2] - 32768) / 65536) * 6.25;
    double Loadcell_Right = -20.00 * (double)(((double)AdwData[7] - 32768) / 65536) * 6.25;

    // Rajoute par moi
    double distance_0 = AnalogOutput[0];
    double distance_1 = AnalogOutput[1];

    // change all -nan(ind) values and nan values by zero in data list
    std::vector<double> data = { std::isfinite(ts) ? ts : 0, std::isfinite(ang_v_0) ? ang_v_0 : 0.1, std::isfinite(ang_v_1) ? ang_v_1 : 0.1, std::isfinite(ang_a_0) ? ang_a_0 : 0.1, std::isfinite(ang_a_1) ? ang_a_1 : 0.1, std::isfinite(distance_0) ? distance_0 : 0.1, std::isfinite(distance_1) ? distance_1 : 0.1, std::isfinite(estimate_power_kg[0]) ? estimate_power_kg[0] : 0.1, std::isfinite(estimate_power_kg[1]) ? estimate_power_kg[1] : 0.1, std::isfinite(estimate_power_both_kg) ? estimate_power_both_kg : 0.1, std::isfinite(estimate_power_N[0]) ? estimate_power_N[0] : 0.1, std::isfinite(estimate_power_N[1]) ? estimate_power_N[1] : 0.1,
            std::isfinite(estimate_power_both_N) ? estimate_power_both_N : 0.1, std::isfinite(dennryu_Left) ? dennryu_Left : 0.1, std::isfinite(dennryu_Right) ? dennryu_Right : 0.1, std::isfinite(Loadcell_Left) ? Loadcell_Left : 0.1, std::isfinite(Loadcell_Right) ? Loadcell_Right : 0.1 };
    TheTrainer->dataManager.AddData(data);
    std::cout << std::to_string(std::isfinite(ts) ? ts : 0.1) + std::to_string(std::isfinite(ang_v_0) ? ang_v_0 : 0.1) << std::endl;

    if (TheTrainer->AcquisitionAsked) {
        char line[200];
        sprintf(line, "%06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf, %06.2lf",
            ts, ang_v_0, ang_v_1, ang_a_0, ang_a_1, AnalogOutput[0], AnalogOutput[1], estimate_power_kg[0], estimate_power_kg[1], estimate_power_both_kg, estimate_power_N[0], estimate_power_N[1],
            estimate_power_both_N, dennryu_Left, dennryu_Right, Loadcell_Left, Loadcell_Right);
        TheTrainer->dataManager.AddLineToFile(line, true);
    }

    // ���[�^�Ɏw�ߓd�����o��
    DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], AnalogOutput);
}

void TrainerBotManager::TimerProcedure()
{
    //-----------------------------------------
    // �^�C�}�[���Ƃ̎��ԊԊu�𐄒�
    //-----------------------------------------
    static int tm_cnt = 0;
    double dt;
    QueryPerformanceCounter(&timeA);
    double ts = (double)(timeA.QuadPart - timeB.QuadPart) / freq.QuadPart;
    dt = ts - pre_ts;

    //-----------------------------------------
    // PCI�{�[�hAD���͏����ݒ�
    // Channel 0 :�E�|�e���V�����[�^ potentiometer right
    // Channel 1 :���|�e���V�����[�^ potentiometer left
    // Channel 2 :���p�X�C�b�` interrupteur d'urgence
    // Channel 3 :�E���[�^�d���l�ɉ������o�͓d��  tension de sortie en fonction du courant moteur droit 
    // Channel 4 :�����[�^�d���l�ɉ������o�͓d��  tension de sortie en fonction du courant moteur gauche
    // Channel 5 :�E�����p�X�C�b�` interrupteur d'urgence cote droit
    // Channel 6 :�������p�X�C�b�` interrupteur d'urgence cote gauche
    //-----------------------------------------
    AdInputAD(hDeviceHandleAd, 8, AD_INPUT_DIFF, &AdSmplChReq[0], AdwData);

    //-------------------------
    //�G���R�[�_�̊p���x�C�p�����x�Z�o
    //-------------------------
    //�G���R�[�_�[�̃J�E���g���擾
    double copy_pre_ang_v[2] = { pre_ang_v[0],pre_ang_v[1] };

    double copy_pre_distance[2] = { pre_distance[0],pre_distance[1] };

    UcntReadCounter(hDeviceHandlePls, 0x03, Value);
    int i = 0;
    double delta_count[2] = { 0 };
    double delta_theta[2] = { 0 };
    double ang_v[2] = { 0 };//�p���x
    double ang_a[2] = { 0 };//�p�����x
    double distance[2] = { 0 }; //�ʒu

    // �G���R�[�_�̊p���x�C�p�����x�Z�o
    // i=0�G�E���G���R�[�_�[
    // i=1�F�����G���R�[�_�[
    for (i = 0; i < 2; i++) {
        double vector = (i == 0) ? 1 : -1;// ��]�����W��
        //�ړ����ς̏�����
        for (int j = num - 1; j > 0; j--) ang_a_ave[i][j] = ang_a_ave[i][j - 1];//�ߋ�num�񕪂̒l���L�^

        //�J�E���g�l�擾
        delta_count[i] = double(Value[i]) - double(preValue[i]);
        //theta�̕ω��� = �i��L�̃J�E���g����4���{������̃J�E���g���j��2PI/�i�M�A�w�b�h�����䁖�N���b�`�M�A������)
        delta_theta[i] = 2.0 * PI * (delta_count[i] / (4 * 500)) / (Rp * Rg);

        // �p���x�Z�o [rad/s] 
        ang_v[i] = vector * delta_theta[i] / (dt);
        // �p�����x�Z�o[rad/s2]
        ang_a_ave[i][0] = vector * (ang_v[i] - copy_pre_ang_v[i]) / (dt);
        float a_ave = 0;
        for (int j = 0; j < num; j++) a_ave += ang_a_ave[i][j]; //���a�����߂�
        ang_a[i] = (float)a_ave / num;                     //�W�{���Ŋ����ĕ��ς����߂�

        distance[i] = ang_v[i] * 0.1 * dt + copy_pre_distance[i];
    }

    // ���̃��[�v�Ɍ����l���X�V	
    for (int j = 0; j < 2; j++) {
        pre_ang_v[j] = ang_v[j];
        preValue[j] = Value[j];
        pre_distance[j] = distance[j];
    }
    pre_ts = ts;

    // ------------------------
    // �|�e���V�����[�^�̊p�x�Z�o(2020�N���݃|�e���V�����[�^�̃f�[�^�͕s�g�p)
    // ------------------------
    //�E�|�e���V�����[�^�p�x�ω���
    theta_diff[0] = (20 * 85 * (double)(AdwData[0] - 32768) / 65536 - 37) - theta_deg[0];
    //�E�|�e���V�����[�^�p�x
    theta_deg[0] = 20 * 85 * (double)(AdwData[0] - 32768) / 65536 - 37;

    //���|�e���V�����[�^�p�x�ω���
    theta_diff[1] = (-20 * 85 * (double)(AdwData[1] - 32768) / 65536 - 20) - theta_deg[1];
    //���|�e���V�����[�^�p�x
    theta_deg[1] = -20 * 85 * (double)(AdwData[1] - 32768) / 65536 - 20;

    //----------------------
    // ���X�C�b�`�̓���{���[�^�̓���
    //----------------------
    double EmergencySwitchLeft = -20.00 * (double)(((double)AdwData[5] - 32768) / 65536);
    double EmergencySwitchRight = -20.00 * (double)(((double)AdwData[6] - 32768) / 65536);
    // �ً}�X�C�b�`�̃`�����l����3.0V�ȏ�̓d������������烂�[�^�[���X�g�b�v����
    if ((EmergencySwitchLeft >= 3.0 || EmergencySwitchRight >= 3.0) && !TheTrainer->compilingInTestMode) {
        StopBot();
        TheTrainer->app.AddRobotConsoleError("Emergency stop");
    }
    else {
        // ���퓮��
        // ���[�^�o��(�g���N����)
        MotorTorqueOutput(ang_v[0], ang_v[1], ang_a[0], ang_a[1]);
    }
    RecordDistances();
    printf("ts:%05.2lf ang_v0:%06.2lf ang_v1:%06.2lf  ang_a0:%06.2lf  ang_a1:%06.2lf distance0:%06.2lf  distance1:%06.2lf\n", ts, ang_v[0], ang_v[1], ang_a[0], ang_a[1], distance[0], distance[1]);
}

void TrainerBotManager::RecordDistances() {
    if (WantRecordDistances) {
        double distance_0 = AnalogOutput[0];
        double distance_1 = AnalogOutput[1];
        std::vector<double> vec = { distance_0, distance_1 };
        distancesMeasured.push_back(std::accumulate(vec.begin(), vec.end(), 0.0) / vec.size());
    }
}

void TrainerBotManager::StopBot() {
    // ---------------------------
    //�g���[�j���O��  Stop experiment
    // ---------------------------

    // ---------------------------
    //�g���[�j���O�I��
    // ---------------------------
    //�^�C�}�[�����X�g�b�v
    timeEndPeriod(1);
    //�J�E���^�X�g�b�v
    nRet = UcntStopCount(hDeviceHandlePls, 0x03, IFUCNT_CMD_STOP | IFUCNT_CMD_SAMPLING);

    StopMotor();
    StopClutch();
    StopCommunicationWithSystem();
    TheTrainer->dataManager.End();
    TheTrainer->BotWorkingInProgress = false;
}

void TrainerBotManager::StopMotor() {
    //��Ƀ��[�^���~�߂Ă���C0.2�b��ɃN���b�`����� Stop motor then wait for 0.2 seconds before stop clutch
    DawData[0] = 32812; //���S�ɂƂ߂邽�߂ɐ��l�ύX
    DawData[1] = 32768; //���S�ɂƂ߂邽�߂ɐ��l�ύX
    DaOutputDA(hDeviceHandleDa, 2, &DaSmplChReq[0], DawData);
    cout << "Motors Stopped" << endl;
}

void TrainerBotManager::StopClutch() {
    nRet = AdOutputDO(hDeviceHandleAd, 0x0F);
}

void TrainerBotManager::StopCommunicationWithSystem() {
    //���o�̓f�o�C�X�̏I������
    DaClose(hDeviceHandleDa);
    AdClose(hDeviceHandleAd);
    UcntClose(hDeviceHandlePls);
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
    double R = 0.01;  // Covariance du bruit de mesure 0.01
    //static double P = 1;  // Covariance d'erreur initiale
    double alpha = 0.7; // How much we trust the predicted value
    double K; // Gain de Kalman

    if (oldSmoothedValues.size() > 2) {
        // Time update
        double old_value = oldSmoothedValues[oldSmoothedValues.size() - 1];
        double slope = old_value - oldSmoothedValues[oldSmoothedValues.size() - 2];
        double intersect = old_value - slope;
        double predicted_value = slope * 2 + intersect;
        P = P + Q;

        // Mise à jour
        K = P / (P + R);
        double smoothed_value = (alpha * predicted_value + (1 - alpha) * old_value) + K * (ValueToSmooth - (alpha* predicted_value + (1-alpha) * old_value));
        P = (1 - K) * P;

        return { smoothed_value, P };
    }
    else {
        return { ValueToSmooth, P };
    }
}

std::vector<double> TrainerBotManager::KalmanFilter(std::vector<double>& oldSmoothedValues, double ValueToSmooth, double P) {
    /*Kalman filter for 1D data to smooth the data measured
    */

    double Q = 0.001; // Covariance du bruit de processus
    double R = 0.01;  // Covariance du bruit de mesure
    //static double P = 1;  // Covariance d'erreur initiale
    double K; // Gain de Kalman

    if (oldSmoothedValues.size() > 2) {
        // Time update
        double old_value = oldSmoothedValues[oldSmoothedValues.size() - 1];
        P = P + Q;

        // Mise à jour
        K = P / (P + R);
        double smoothed_value = old_value + K * (ValueToSmooth - old_value);
        P = (1 - K) * P;

        return { smoothed_value, P };
    }
    else {
        return { ValueToSmooth, P };
    }
}
