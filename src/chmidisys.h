//---------------------------------------------------------------------------
#ifndef chmidisysH
#define chmidisysH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Controls.hpp>
#include <Classes.hpp>
#include <Forms.hpp>
#include <SyncObjs.hpp>
//---------------------------------------------------------------------------
#include <mmsystem.h>
#include "chexception.h"
#include "tmpfwd.h"
#include "chmidi.h"
//---------------------------------------------------------------------------
namespace ChimeraMidiSystem
{
    class TChimeraMidi;
    class TChimeraMidiData;
    class TChimeraMidiPlayThread;


    // 定数 ========================================
    const int MaxChannel        = 16;
    const unsigned long WM_CHMIDISTART  = WM_USER + 30;
    const unsigned long WM_CHMIDIEND    = WM_USER + 31;

    // 初期化用定数
    enum TMidiReset
    {
        ResetGM                 = 0,
        ResetGS                 = 1
    };

    // リバーブ用定数
    enum TGSReverb
    {
        ReverbRoom1             = 0,
        ReverbRoom2             = 1,
        ReverbRoom3             = 2,
        ReverbHall1             = 3,
        ReverbHall2             = 4,
        ReverbPlate             = 5,
        ReverbDelay             = 6,
        ReverbPanningDelay      = 7
    };

    // コーラス用定数
    enum TGSChorus
    {
        ChorusChorus1           = 0,
        ChorusChorus2           = 1,
        ChorusChorus3           = 2,
        ChorusChorus4           = 3,
        ChorusFeedBackChorus    = 4,
        ChorusFlanger           = 5,
        ChorusShortDelay        = 6,
        ChorusShortDelayFB      = 7
    };

    // ディレイ用定数
    enum TGSDelay
    {
        DelayDelay1             = 0,
        DelayDelay2             = 1,
        DelayDelay3             = 2,
        DelayDelay4             = 3,
        DelayPanDelay1          = 4,
        DelayPanDelay2          = 5,
        DelayPanDelay3          = 6,
        DelayPanDelay4          = 7,
        DelayDelayToReverb      = 8,
        DelayPanRepeat          = 9
    };

    // メタイベント
    enum TSMFMetaEvent
    {
        meSequenceNumber        = 0x00,
        meText                  = 0x01,
        meCreator               = 0x02,
        meSequenceTrackName     = 0x03,
        meInstrument            = 0x04,
        meSong                  = 0x05,
        meMarker                = 0x06,
        meQueue                 = 0x07,
        meChannelPrefix         = 0x20,
        meEndOfTrack            = 0x2F,
        meSetTempo              = 0x51,
        meBar                   = 0x58,
        meKey                   = 0x59,
        meIDMetaEvent           = 0x7F,
        meNone                  = 0xFF
    };

    // コントロールチェンジ
    enum TMIDIControlChange
    {
        ccModulation            = 0x01,
        ccPortamentoTime        = 0x02,
        ccVolume                = 0x07,
        ccPanpot                = 0x0A,
        ccExpression            = 0x0B,
        ccHold1                 = 0x40,
        ccPortamento            = 0x41,
        ccSostenuto             = 0x42,
        ccSoft                  = 0x43,
        ccPortamentoControl     = 0x54,
        ccEffect1               = 0x5B,
        ccEffect3               = 0x5D,
        ccEffect4               = 0x5E
    };

    // TChimeraMidiChannel =========================
    struct TChimeraMidiChannel
    {
        unsigned char Velocity;             // ベロシティー
        unsigned char Program;              // プログラム
        unsigned char Volume;               // ボリューム
        unsigned char Pan;                  // パン
        unsigned char Expression;           // エクスプレッション
        unsigned char Reverb;               // リバーブ
        unsigned char Chorus;               // コーラス
        unsigned char Delay;                // ディレイ
    };


    // TShortMessage ==============================
    struct TShortMessage
    {
        unsigned long DeltaTime;            // デルタタイム
        union {
            unsigned long Data;
            struct {
                unsigned char Msg;          // MIDIイベント
                unsigned char Param1;       // パラメータ１
                unsigned char Param2;       // パラメータ２
            };
        };
        TShortMessage(void)
            : DeltaTime(0), Msg(0), Param1(0), Param2(0)
            { ; }
        TShortMessage(unsigned char AMsg,
            unsigned char AParam1, unsigned char AParam2)
            : DeltaTime(0), Msg(AMsg), Param1(AParam1), Param2(AParam2)
            { ; }
        TShortMessage(unsigned long ADeltaTime, unsigned char AMsg,
            unsigned char AParam1, unsigned char AParam2)
            : DeltaTime(ADeltaTime), Msg(AMsg), Param1(AParam1), Param2(AParam2)
            { ; }
    };


    // TLongMessage ===============================
    struct TLongMessage
    {
        unsigned long DeltaTime;            // デルタタイム
        unsigned char *Data;                // システムエクスクルーシブ
        unsigned long DataSize;             // サイズ
        const TLongMessage& operator =(const TLongMessage &Org)
        {
            if(this == &Org) return (*this);
            delete[] Data;
            Data = NULL;
            DeltaTime = Org.DeltaTime;
            DataSize = Org.DataSize;
            Data = new unsigned char[DataSize];
            memcpy(Data, Org.Data, DataSize);
            return (*this);
        }
        TLongMessage(void)
            : DeltaTime(0), Data(NULL), DataSize(0)
            { ; }
        TLongMessage(unsigned char *AData, unsigned long ADataSize)
            : DeltaTime(0), Data(NULL), DataSize(ADataSize)
            { Data = new unsigned char[DataSize]; memcpy(Data, AData, DataSize);  }
        TLongMessage(unsigned long ADeltaTime, unsigned char *AData,
            unsigned long ADataSize)
            : DeltaTime(ADeltaTime), Data(NULL), DataSize(ADataSize)
            { Data = new unsigned char[DataSize]; memcpy(Data, AData, DataSize);  }
        TLongMessage(const TLongMessage &Org)
            : DeltaTime(Org.DeltaTime), Data(NULL), DataSize(Org.DataSize)
            { Data = new unsigned char[DataSize]; memcpy(Data, Org.Data, DataSize); }
        ~TLongMessage(void)
            { delete[] Data; }
    };


    // TMetaEvent ==================================
    struct TMetaEvent
    {
        unsigned long DeltaTime;            // デルタタイム
        TSMFMetaEvent DataType;             // データのタイプ
        unsigned long Data;                 // 数値データ
        AnsiString StrData;                 // 文字列データ

        TMetaEvent(void)
            : DeltaTime(0), DataType(meNone), Data(0), StrData("")
            { ; }
        TMetaEvent(unsigned long ADeltaTime, TSMFMetaEvent ADataType,
            unsigned long AData, const char *AStrData)
            : DeltaTime(ADeltaTime), DataType(ADataType), Data(AData),
              StrData(AStrData)
            { ; }
    };


    // イベント型定義
    typedef std::list<TShortMessage, std::allocator<TShortMessage>
        > TShortMessageList;
    typedef std::list<TLongMessage, std::allocator<TLongMessage>
        > TLongMessageList;
    typedef std::list<TMetaEvent, std::allocator<TMetaEvent>
        > TMetaEventList;
    typedef std::list<TShortMessage, std::allocator<TShortMessage>
        > TUserMessageList;


    // TChimeraMidiDataInfo ========================
    class TChimeraMidiDataInfo
    {
    private:
        TChimeraMidiDataInfo(const TChimeraMidiDataInfo &Org);
        
    public:
        unsigned long MidiFormat;           // フォーマットの種類
        unsigned long TimeBase;             // MIDIファイルのベース
        unsigned long EndDeltaTime;         // 最終デルタタイム
        unsigned long MusicBarChild;        // 拍子（分子）
        unsigned long MusicBarMother;       // 拍子（分母）
        unsigned long MusicBarDelta;        // 拍子MIDIクロック

        TShortMessageList *ShortMessageList;// 通常メッセージ
        TLongMessageList *LongMessageList;  // システムエクスクルーシブ
        TMetaEventList *MetaEventList;      // メタイベント

        TStringList *Text;                  // テキスト
        TStringList *Creator;               // 著作権表示
        TStringList *SongName;              // 曲名＆トラック名

        void __fastcall Clear(void);
        __fastcall TChimeraMidiDataInfo(void);
        __fastcall ~TChimeraMidiDataInfo(void);
    };


    // TChimeraMidiData ============================
    class TChimeraMidiData
    {
    private:
        TChimeraMidiDataInfo SMFData;       // SMFファイル

        void __fastcall ReadTrack(unsigned char *Data, unsigned long DataSize);

        bool __fastcall DefaultEvent(
            unsigned char *Data, unsigned long &CurrentPos,
            unsigned char Msg, unsigned long DeltaTime, void *Node);
        bool __fastcall SystemExclusive(
            unsigned char *Data, unsigned long &CurrentPos,
            unsigned char Msg, unsigned long DeltaTime, void *Node);
        bool __fastcall MetaEvent(
            unsigned char *Data, unsigned long &CurrentPos,
            unsigned char Msg, unsigned long DeltaTime, void *Node);

    public:
        TChimeraMidiDataInfo& GetData(void)
            { return SMFData; }
        unsigned long GetTimeBase(void)
            { return SMFData.TimeBase; }
        void SetTimeBase(unsigned long Value)
            { SMFData.TimeBase = Value; }

        void __fastcall LoadFile(const AnsiString &AFileName);
        __fastcall TChimeraMidiData(void);
        __fastcall ~TChimeraMidiData(void);
    };


    // TChimeraMidiPlayThread ======================
    class TChimeraMidiPlayThread : public TThread
    {
    private:
        TChimeraMidi *Owner;                // データ持ち主
        unsigned long Delay;                // 呼び出し周期
    protected:
        virtual void __fastcall Execute(void);
    public:
        __fastcall TChimeraMidiPlayThread(TChimeraMidi *AOwner,
            unsigned long ADelay);
        __fastcall ~TChimeraMidiPlayThread(void);
    };


    // TChimeraMidi ================================
    class TChimeraMidi
    {
        typedef std::map<AnsiString, TChimeraAutoPtr<TChimeraMidiData>,
                std::less<AnsiString>,
                std::allocator<std::pair<const AnsiString,
                    TChimeraAutoPtr<TChimeraMidiData> > >
            > TChimeraMidiDataMap;

    protected:
        friend TChimeraMidiPlayThread;
        bool FInit;                         // 初期化済フラグ
        HMIDIOUT hMidiOut;                  // MIDI出力ハンドル
        HWND FDummyHWnd;                    // ウィンドウメッセージ取得用
        TCriticalSection *ChannelCritical;  // チャンネルデータ
        TCriticalSection *UserDataCritical; // ユーザー演奏データ
        TEvent *EndEvent;                   // 演奏終了イベント

        bool FLoaded;                       // 読み込み済みフラグ
        bool FPlay;                         // 再生中
        bool FStop;                         // 停止
        bool FRepeat;                       // リピートフラグ
        bool FReport;                       // 再生状況の取得
        long FFadeOut;                      // フェードアウト
        long FFadeSend;                     // 送信用カウンタ
        float VolumeRate;                   // フェードアウト時の差分
        float DeltaVolume;                  // フェードアウト時の差分

        unsigned long MusicTempo;           // テンポ
        unsigned long TimeCount;            // つじつま合わせ
        unsigned long DeltaTime;            // デルタタイム
        unsigned long RepeatPoint;          // リピートポイント

        TChimeraMidiPlayThread *PlayThread; // MIDI再生用スレッド

        TChimeraMidiDataInfo *PlayData;     // 再生データ
        TUserMessageList *UserMessageList;  // ユーザー入力

        void *SMIterator;                   // MIDIイベント反復子
        void *LMIterator;                   // システムエクスクルーシブ反復子
        void *MEIterator;                   // メタイベントリスト反復子

        TChimeraMidiChannel MidiChannels[MaxChannel];   // MIDIチャンネルデータ
        TChimeraMidiChannel SyncMidiChannels[MaxChannel];   // 同期用

        TChimeraMidiDataMap *DataMap;       // MIDIデータののマップ

        // MIDIトラック処理
        void __fastcall SmfPlay(unsigned long Delay);
        void __fastcall UserPlay(unsigned long Delay);

        // トラック処理関係
        void __fastcall ShortMsg(const TShortMessage &Msg);
        void __fastcall LongMsg(const TLongMessage &Msg);
        void __fastcall LoadFile(unsigned char *DataPtr);

        // 演奏初期化関係
        void __fastcall Reset(void);
        void __fastcall Repeat(void);

    public:
        // MidiOut初期化関係
        void __fastcall Init(HWND hWnd, HINSTANCE hInstance);
        void __fastcall CleanUp(void);

        // SMF操作関係
        void __fastcall CreateData(const AnsiString &DataName,
            const AnsiString &FileName);
        const TChimeraMidiDataInfo& __fastcall GetSMFData(
            const AnsiString &DataName);
        void __fastcall DeleteData(const AnsiString &DataName);
        void __fastcall DeleteAllData(void);

        // MIDI再生関係
        void __fastcall Play(const AnsiString &DataName, bool RepeatFlag);
        void __fastcall SetFadeOut(int Time);
        void __fastcall Stop(void);

        // 再生情報関係
        void __fastcall SetReport(bool Value);
        unsigned long __fastcall GetDeltaTime(void);
        unsigned long __fastcall GetMusicTempo(void);
        void __fastcall GetChannelData(TChimeraMidiChannel *Channels);

        // 直接演奏
        void __fastcall NoteOn(unsigned char Channel, unsigned char Note,
            unsigned char Velocity, unsigned long Time);
        void __fastcall NoteOff(unsigned char Channel, unsigned char Note);
        void __fastcall ControlChange(unsigned char Channel,
            unsigned char Number, unsigned char Value);
        void __fastcall ProgramChange(unsigned char Channel, unsigned char Number);
        void __fastcall ChannelPressure(unsigned char Channel, unsigned char Pressure);
        void __fastcall PitchBend(unsigned char Channel, unsigned char PitchBend);

        // 音源操作関係
        void __fastcall MidiReset(TMidiReset Reset);
        void __fastcall SetGSReverb(TGSReverb Reverb);
        void __fastcall SetGSChorus(TGSChorus Chorus);
        void __fastcall SetGSDelay(TGSDelay Delay);

        __fastcall TChimeraMidi(void);
        virtual __fastcall ~TChimeraMidi(void);
    };
};
//---------------------------------------------------------------------------
using namespace ChimeraMidiSystem;
//---------------------------------------------------------------------------
#endif
