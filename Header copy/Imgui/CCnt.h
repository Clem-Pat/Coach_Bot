//============================================================-
// CNT.H
// Headder file for CONTEC Counter device
// Supported Devices:
//       CNT24-2(USB)
// Ver1.00
//                                              CONTEC.Co.,Ltd.
//============================================================-

//-------------------------------------------------
// Prototype definition
//-------------------------------------------------
#ifdef __cplusplus
extern "C"{
#endif
// Common function
long WINAPI CntInit(char *DeviceName, short *Id);
long WINAPI CntExit(short Id);
long WINAPI CntResetDevice(short Id);
long WINAPI CntGetErrorString(long ErrorCode , char *ErrorString);
// Setting function (set)
long WINAPI CntSetZMode(short Id, short ChNo, short Mode);
long WINAPI CntSetZLogic(short Id, short ChNo, short ZLogic);
long WINAPI CntSelectChannelSignal(short Id, short ChNo, short SigType);
long WINAPI CntSetCountDirection(short Id, short ChNo, short Dir);
long WINAPI CntSetOperationMode(short Id, short ChNo, short Phase, short Mul, short SyncClr);
long WINAPI CntSetDigitalFilter(short Id, short ChNo, short FilterValue);
long WINAPI CntSetPulseWidth(short Id, short ChNo, short PlsWidth);
// Setting function (get)
long WINAPI CntGetZMode(short Id, short ChNo, short *Mode);
long WINAPI CntGetZLogic(short Id, short ChNo, short *ZLogic);
long WINAPI CntGetChannelSignal(short Id, short ChNo, short *SigType);
long WINAPI CntGetCountDirection(short Id, short ChNo, short *Dir);
long WINAPI CntGetOperationMode(short Id, short ChNo, short *Phase, short *Mul, short *SyncClr);
long WINAPI CntGetDigitalFilter(short Id, short ChNo, short *FilterValue);
long WINAPI CntGetPulseWidth(short Id, short ChNo, short *PlsWidth);
// Counter function
long WINAPI CntPreset(short Id, short *ChNo, short ChNum, DWORD *PresetData);
long WINAPI CntStartCount(short Id, short *ChNo, short ChNum);
long WINAPI CntStopCount(short Id, short *ChNo, short ChNum);
long WINAPI CntReadCount(short Id, short *ChNo, short ChNum, DWORD *CntDat);
long WINAPI CntReadStatus(short Id, short ChNo, short *Sts);
// Count match function
long WINAPI CntNotifyCountUp(short Id, short ChNo, short RegNo, DWORD Count, HANDLE hWnd);
long WINAPI CntStopNotifyCountUp(short Id, short ChNo, short RegNo);
// Timer function
long WINAPI CntNotifyTimeUp(short Id, DWORD TimeValue, HANDLE hWnd);
long WINAPI CntStopNotifyTimeUp(short Id);
// General purpose input function
long WINAPI CntInputDIByte(short Id, short Reserved, BYTE *bData);
// Information function
long WINAPI CntGetDeviceInfo(char *Device, short InfoType, void *Param1, void *Param2, void *Param3);
long WINAPI CntQueryDeviceName(short Index, char *DeviceName, char *Device);
long WINAPI CntGetDeviceType(char *Device, short *DeviceType);
long WINAPI CntGetMaxChannels(short Id, short *ChannelNum);
#ifdef __cplusplus
};
#endif
//-------------------------------------------------
// Type definition
//-------------------------------------------------

#define	DEVICE_TYPE_ISA					0	//ISAまたはCバス
#define	DEVICE_TYPE_PCI					1	//PCIバス
#define	DEVICE_TYPE_PCMCIA				2	//PCMCIA
#define	DEVICE_TYPE_USB					3	//USB
#define	DEVICE_TYPE_FIT					4	//FIT

//-------------------------------------------------
// Mode definition
//-------------------------------------------------
// Z Phase Mode
#define	CNT_ZPHASE_NOT_USE			1
#define	CNT_ZPHASE_NEXT_ONE			2
#define	CNT_ZPHASE_EVERY_TIME		3
// Z Phase Logic
#define	CNT_ZLOGIC_POSITIVE			0
#define	CNT_ZLOGIC_NEGATIVE			1
// Signal Source
#define	CNT_SIGTYPE_ISOLATE			0
#define	CNT_SIGTYPE_TTL				1
#define	CNT_SIGTYPE_LINERECEIVER	2
// Count Direction
#define	CNT_DIR_DOWN				0
#define	CNT_DIR_UP					1
// 1Phase/2Phase
#define	CNT_MODE_1PHASE				0
#define	CNT_MODE_2PHASE				1
#define	CNT_MODE_GATECONTROL		2
// Mul
#define	CNT_MUL_X1					0
#define	CNT_MUL_X2					1
#define	CNT_MUL_X4					2
// Sync Clear
#define	CNT_CLR_ASYNC				0
#define	CNT_CLR_SYNC				1
// Gate Control
#define	CNT_UPDOWN					1
#define	CNT_GATECONTROL				0

// Generic I/O Signal
#define	CNT_GENIO_LINERECEIVER		0
#define	CNT_GENIO_TTL				1

// Device Information
#define	ICNT_DEVICE_TYPE			0	// device type.						Param1:short
#define	ICNT_NUMBER_OF_CH			1	// ﾁｬﾈﾙ数							Param1:int
#define	ICNT_BIT					2	// ﾋﾞｯﾄ数							Param1:int
#define	ICNT_IS_DIGITAL_FILTER		3	// ﾃﾞｼﾞﾀﾙﾌｨﾙﾀがあるか？				Param1:BOOL(True/False)
#define	ICNT_IS_DEFF				4	// 汎用入力の差動切り替えあるか？	Param1:BOOL(True/False)
#define	ICNT_CNTSOURCE				5	// カウンタの信号源　　　　　　　　 Param1:int(BIT0:TTL, BIT1:PHOTO, BIT2:LINERECEIVER)

#define	ICNT_CNTSOURCE_TTL			0x01
#define	ICNT_CNTSOURCE_PHOTO		0x02
#define	ICNT_CNTSOURCE_LINERECEIVER	0x04
//-------------------------------------------------
// Message
//-------------------------------------------------
#define	CNTM_COUNTUP_CH0	0x1100		// カウントアップ、チャネル番号0
#define	CNTM_COUNTUP_CH1	0x1101		//         "                   1
#define	CNTM_COUNTUP_CH2	0x1102		//         "                   2
#define	CNTM_COUNTUP_CH3	0x1103		//         "                   3
#define	CNTM_COUNTUP_CH4	0x1104		//         "                   4
#define	CNTM_COUNTUP_CH5	0x1105		//         "                   5
#define	CNTM_COUNTUP_CH6	0x1106		//         "                   6
#define	CNTM_COUNTUP_CH7	0x1107		//         "                   7

#define	CNTM_TIME_UP		0x1140		// タイムアップ

// Count Match Direction(UP=1, DOWN=2)
#define	CNTM_DIR_UP			1			// アップカウントで一致
#define	CNTM_DIR_DOWN		2			// ダウンカウントで一致


//-------------------------------------------------
// Error codes
//-------------------------------------------------
// Initialize Error
// Common
#define	CNT_ERR_SUCCESS						0		//	：正常終了
#define CNT_ERR_INI_RESOURCE				1		//	：リソースの取得に失敗しました
#define	CNT_ERR_INI_INTERRUPT				2		//	：割り込みルーチンの登録に失敗しました
#define	CNT_ERR_INI_MEMORY					3		//	：メモリの割り当てに失敗しました
#define	CNT_ERR_INI_REGISTRY				4		//	：レジストリのアクセスに失敗しました
// CNT

// DLL Error
// Common
#define	CNT_ERR_DLL_DEVICE_NAME			10000	//	：デバイスマネージャーに登録されていないデバイス名が指定されました
#define	CNT_ERR_DLL_INVALID_ID			10001	//	：無効なIDが指定されました
#define	CNT_ERR_DLL_CALL_DRIVER			10002	//	：ドライバを呼び出せません。(デバイスI/Oコントロールに失敗)
#define	CNT_ERR_DLL_CREATE_FILE			10003	//	：ファイルの作成に失敗しました(CreateFile失敗)
#define	CNT_ERR_DLL_CLOSE_FILE			10004	//	：ファイルのクローズに失敗しました(CloseFile失敗)
#define	CNT_ERR_DLL_CREATE_THREAD		10005	//	：スレッドの作成に失敗しました(CreateThread失敗)
#define	CNT_ERR_INFO_INVALID_DEVICE		10050	//	：指定したデバイス名称が見つかりません。スペルを確認してください。
#define	CNT_ERR_INFO_NOT_FIND_DEVICE	10051	//	：利用可能なデバイスが見つかりません。
#define	CNT_ERR_INFO_INVALID_INFOTYPE	10052	//	：指定したデバイス情報タイプが範囲外です。
// CNT
#define	CNT_ERR_DLL_MODE				10100	//	：モードが設定可能範囲外です
#define CNT_ERR_DLL_BUFF_ADDRESS		10101	//	：データバッファアドレスが不正です
#define	CNT_ERR_DLL_HWND				10200	//	：ウィンドウハンドルが指定可能範囲外です

// SYS Error
// Common
#define	CNT_ERR_SYS_MEMORY				20000	//	：メモリの確保に失敗しました
#define	CNT_ERR_SYS_NOT_SUPPORTED		20001	//　：このボードではこの関数は使用できません
#define	CNT_ERR_SYS_BOARD_EXECUTING		20002	//　：ボードが動作中のため実行できません
#define	CNT_ERR_SYS_USING_OTHER_PROCESS	20003	//　：他のプロセスがデバイスを使用しているため、実行できません

#ifndef STATUS_SYS_USB_CRC
#define	STATUS_SYS_USB_CRC						20020	// エンドポイントから受け取った最後のデータパケットにCRCエラーが存在しています。
#define	STATUS_SYS_USB_BTSTUFF					20021	// エンドポイントから受け取った最後のデータパケットにビット詰め違反のエラーが存在しています。
#define	STATUS_SYS_USB_DATA_TOGGLE_MISMATCH		20022	// エンドポイントから受け取った最後のデータパケットに期待した値にマッチしないデータトグルパケット
#define	STATUS_SYS_USB_STALL_PID				20023	// エンドポイントがSTALLパケット識別子を返しました。
#define	STATUS_SYS_USB_DEV_NOT_RESPONDING		20024	// デバイスがトークン(IN)に応答していないかハンドシェークをサポートしていません。
#define	STATUS_SYS_USB_PID_CHECK_FAILURE		20025	// エンドポイントから受け取ったパケット識別子上のチェックビットに問題があります。
#define	STATUS_SYS_USB_UNEXPECTED_PID			20026	// 受け取ったパケット識別子が無効か未定義です。
#define	STATUS_SYS_USB_DATA_OVERRUN				20027	// エンドポイントから戻されたデータの量が許容されたデータパケットの最大長あるいはバッファの残量を超えています。
#define	STATUS_SYS_USB_DATA_UND0RRUN			20028	// エンドポイントから戻されたデータの量が期待したデータのサイズに足りません。
#define	STATUS_SYS_USB_BUFFER_OVERRUN			20029	// IN転送において指定されたバッファが小さすぎてデバイスから受け取ったデータをすべて格納することができません。
#define	STATUS_SYS_USB_BUFFER_UNDERRUN			20030	// OUT転送において指定されたバッファにはデバイスに送出するための十分なデータが格納されていません。
#define	STATUS_SYS_USB_ENDPOINT_HALTED			20031	// エンドポイントがSTALL状態のため転送に失敗しました。
#define	STATUS_SYS_USB_NOT_FOUND_DEVINFO		20032	// 情報取得でデバイスの情報が見つかりませんでした。
#define STATUS_SYS_USB_ACCESS_DENIED			20033	//　ハードウェアへのアクセスが拒否されました。
#define STATUS_SYS_USB_INVALID_HANDLE			20034	//　指定したハンドルは無効です。
#endif
// CNT
#define	CNT_ERR_SYS_MODE				20100	//	：モードが設定可能範囲外です
#define	CNT_ERR_SYS_CH_NO				20201	//	：チャネル番号が設定可能範囲外です
#define	CNT_ERR_SYS_CH_NUM				20202	//	：チャネル数が設定可能範囲外です
#define	CNT_ERR_SYS_CH_VALUE			20203	//	：カウンタ値が設定可能範囲外です
#define	CNT_ERR_SYS_REGNO			20204	//	：比較レジスタ番号が設定可能範囲外です
#define	CNT_ERR_SYS_TIMER_VALUE			20300	//	：タイマ値が設定可能範囲外です
