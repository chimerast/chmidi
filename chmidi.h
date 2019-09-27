//---------------------------------------------------------------------------
#ifndef chmidiH
#define chmidiH
//---------------------------------------------------------------------------
#ifdef DLL_BUILD
    #define DLLEXPORT extern "C" __declspec(dllexport)
#else
    #define DLLEXPORT extern "C" __declspec(dllimport)
#endif
//---------------------------------------------------------------------------
// 演奏ウィンドウメッセージ
const unsigned long WM_CHMIDISTART  = WM_USER + 30;
const unsigned long WM_CHMIDIEND    = WM_USER + 31;
//---------------------------------------------------------------------------
// リセット用定数
const int mrGM                      = 0;
const int mrGS                      = 1;
//---------------------------------------------------------------------------
// リバーブ用定数
const int gsrRoom1                  = 0;
const int gsrRoom2                  = 1;
const int gsrRoom3                  = 2;
const int gsrHall1                  = 3;
const int gsrHall2                  = 4;
const int gsrPlate                  = 5;
const int gsrDelay                  = 6;
const int gsrPanningDelay           = 7;
//---------------------------------------------------------------------------
// コーラス用定数
const int gscChorus1                = 0;
const int gscChorus2                = 1;
const int gscChorus3                = 2;
const int gscChorus4                = 3;
const int gscFeedBackChorus         = 4;
const int gscFlanger                = 5;
const int gscShortDelay             = 6;
const int gscShortDelayFB           = 7;
//---------------------------------------------------------------------------
// ディレイ用定数
const int gsdDelay1                 = 0;
const int gsdDelay2                 = 1;
const int gsdDelay3                 = 2;
const int gsdDelay4                 = 3;
const int gsdPanDelay1              = 4;
const int gsdPanDelay2              = 5;
const int gsdPanDelay3              = 6;
const int gsdPanDelay4              = 7;
const int gsdDelayToReverb          = 8;
const int gsdPanRepeat              = 9;
//---------------------------------------------------------------------------
// コントロールチェンジ
const int ccModulation              = 0x01;
const int ccPortamentoTime          = 0x02;
const int ccVolume                  = 0x07;
const int ccPanpot                  = 0x0A;
const int ccExpression              = 0x0B;
const int ccHold1                   = 0x40;
const int ccPortamento              = 0x41;
const int ccSostenuto               = 0x42;
const int ccSoft                    = 0x43;
const int ccPortamentoControl       = 0x54;
const int ccEffect1                 = 0x5B;
const int ccEffect3                 = 0x5D;
const int ccEffect4                 = 0x5E;
//---------------------------------------------------------------------------
// SMFデータ情報
struct TChMidiDataInfo
{
    unsigned long MidiFormat;           // フォーマットの種類
    unsigned long TimeBase;             // MIDIファイルのベース
    unsigned long EndDeltaTime;         // 最終デルタタイム
    unsigned long MusicBarChild;        // 拍子（分子）
    unsigned long MusicBarMother;       // 拍子（分母）
    unsigned long MusicBarDelta;        // 拍子MIDIクロック
    char Creator[256];
    char SongName[256];
};
//---------------------------------------------------------------------------
// チャンネル状態
struct TChMidiChannel
{
    unsigned char Velocity[16];         // ベロシティー
    unsigned char Program[16];          // プログラム
    unsigned char Volume[16];           // ボリューム
    unsigned char Pan[16];              // パン
    unsigned char Expression[16];       // エクスプレッション
    unsigned char Reverb[16];           // リバーブ
    unsigned char Chorus[16];           // コーラス
    unsigned char Delay[16];            // ディレイ
};
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiGetLastError(char *Buffer, int BufferSize);
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiInit(HWND hWnd, HINSTANCE hInst);
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiPlay(const char *DataName, int Repeat);
DLLEXPORT int __stdcall ChMidiStop(void);
DLLEXPORT int __stdcall ChMidiFadeOut(int Time);
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiSetReport(int Value);
DLLEXPORT int __stdcall ChMidiGetDeltaTime(int *DeltaTime);
DLLEXPORT int __stdcall ChMidiGetMusicTempo(int *MusicTempo);
DLLEXPORT int __stdcall ChMidiGetChannelData(TChMidiChannel *Channels);
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiCreateData(const char *DataName, const char *FileName);
DLLEXPORT int __stdcall ChMidiGetDataInfo(const char *DataName, TChMidiDataInfo *Info);
DLLEXPORT int __stdcall ChMidiDeleteData(const char *DataName);
DLLEXPORT int __stdcall ChMidiDeleteAllData(void);
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiNoteOn(int Channel, int Note, int Velocity, int Time);
DLLEXPORT int __stdcall ChMidiNoteOff(int Channel, int Note);
DLLEXPORT int __stdcall ChMidiControlChange(int Channel, int Number, int Value);
DLLEXPORT int __stdcall ChMidiProgramChange(int Channel, int Number);
DLLEXPORT int __stdcall ChMidiChannelPressure(int Channel, int Pressure);
DLLEXPORT int __stdcall ChMidiPitchBend(int Channel, int PitchBend);
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiMidiReset(int Reset);
DLLEXPORT int __stdcall ChMidiSetGSReverb(int Reverb);
DLLEXPORT int __stdcall ChMidiSetGSChorus(int Chorus);
DLLEXPORT int __stdcall ChMidiSetGSDelay(int Delay);
//---------------------------------------------------------------------------
#endif