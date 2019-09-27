//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include <list>
#include <map>
#include "chautoptr.h"
#include "chmidisys.h"
//---------------------------------------------------------------------------
using namespace std;
#pragma link "chautoptr"
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define _ISINIT  { if(!FInit) { throw EChMidi("まだ初期化されてません。");   } }
#define Test_ChMidi(t, e)     { if(!t) { throw EChMidi(e);                   } }
#define Test_ChMidiInit(t, e) { if(FAILED(t)) { CleanUp(); throw EChMidi(e); } }
//---------------------------------------------------------------------------
HINSTANCE hInst = NULL;
//---------------------------------------------------------------------------
inline unsigned short ByteToWord(unsigned char *Data)
{ return (unsigned short)((*Data << 8) | (*(Data+1))); }
//---------------------------------------------------------------------------
inline unsigned long ByteToDword(unsigned char *Data)
{ return (*Data << 24) | (*(Data+1) << 16) | (*(Data+2) << 8) | (*(Data+3)); }
//---------------------------------------------------------------------------
unsigned long GetValue(unsigned char *Data, unsigned long &CurrentPos)
{
    // 可変長データ読み込み
    unsigned long Temp, Buffer;
    Buffer = 0;
    do {
        Temp = *(Data + CurrentPos);
        Buffer = (Buffer << 7) | (Temp & 0x7F);
        ++CurrentPos;
    } while(Temp & 0x80);
    return Buffer;
}
//---------------------------------------------------------------------------
namespace ChimeraMidiSystem
{
    typedef TShortMessageList::iterator TShortMessageItr;
    typedef TLongMessageList::iterator TLongMessageItr;
    typedef TMetaEventList::iterator TMetaEventItr;
};
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChimeraMidi
// 内容      : MIDI機器、データの管理
// Version   : 0.75 (2000/01/14)
//***************************************************************************

//***************************************************************************
// コンストラクタ
//***************************************************************************
__fastcall TChimeraMidi::TChimeraMidi(void)
    : hMidiOut(NULL), PlayThread(NULL), TimeCount(0), MusicTempo(500*1000),
      FFadeOut(0), FInit(false), FLoaded(false), FPlay(false), FStop(false),
      FReport(true), FDummyHWnd(NULL)
{
    DataMap = new TChimeraMidiDataMap();

    UserMessageList = new TUserMessageList();

    // Iterator構築
    SMIterator = new TShortMessageItr();
    LMIterator = new TLongMessageItr();
    MEIterator = new TMetaEventItr();

    // 同時参照防止薬設定
    ChannelCritical = new TCriticalSection();
    UserDataCritical = new TCriticalSection();

    EndEvent = new TEvent(NULL, false, false, "");

    // 再生スレッド
    PlayThread = new TChimeraMidiPlayThread(this, 10);
    PlayThread->Priority = tpTimeCritical;

    PlayThread->Resume();
}


//***************************************************************************
// デストラクタ
//***************************************************************************
__fastcall TChimeraMidi::~TChimeraMidi(void)
{
    PlayThread->FreeOnTerminate = true;
    PlayThread->Terminate();
    Sleep(10);

    // 開放
    CleanUp();

    delete ChannelCritical;
    delete UserDataCritical;
    delete EndEvent;
    delete PlayThread;
    delete DataMap;

    delete (TShortMessageItr*)SMIterator;
    delete (TLongMessageItr*)LMIterator;
    delete (TMetaEventItr*)MEIterator;

    delete UserMessageList;
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// MidiOut初期化関係
//***************************************************************************

//***************************************************************************
// 初期化
//***************************************************************************
void __fastcall TChimeraMidi::Init(HWND hWnd, HINSTANCE hInstance)
{
    // MIDIOUTのオープン
    if(!hMidiOut)
    {
        Test_ChMidiInit(
            midiOutOpen(&hMidiOut, MIDI_MAPPER, NULL, 0, CALLBACK_NULL),
            "MidiOutの作成に失敗しました。");
    }

    FDummyHWnd = hWnd;
    hInst = hInstance;

    FInit = true;
}


//***************************************************************************
// 全開放
//***************************************************************************
void __fastcall TChimeraMidi::CleanUp(void)
{
    // 演奏を止めてからMIDIOUTを閉じる
    if(hMidiOut)
    {
        Stop();
        midiOutClose(hMidiOut);
        hMidiOut = NULL;
    }
    FInit = false;
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// SMFデータ関係
//***************************************************************************

//***************************************************************************
// 読み込み
//***************************************************************************
void __fastcall TChimeraMidi::CreateData(const AnsiString &DataName,
    const AnsiString &FileName)
{
    if((*DataMap).find(DataName) != (*DataMap).end())
        return;

    (*DataMap)[DataName].Reset(new TChimeraMidiData());
    try {
        (*DataMap)[DataName]->LoadFile(FileName);
    } catch(...) {
        (*DataMap).erase(DataName);
        throw;
    }
}


//***************************************************************************
// SMFデータ情報取得
//***************************************************************************
const TChimeraMidiDataInfo& __fastcall TChimeraMidi::GetSMFData(
    const AnsiString &DataName)
{
    TChimeraMidiDataMap::iterator pitr; // 反復子

    // 登録されていなかったら例外送出
    pitr = (*DataMap).find(DataName);
    if(pitr == (*DataMap).end())
    {
        throw EChMidi("サーフェイスが存在しません。\nスプライト名："
            + DataName);
    }

    return (*pitr).second->GetData();
}


//***************************************************************************
// サーフェイス開放
//***************************************************************************
void __fastcall TChimeraMidi::DeleteData(const AnsiString &DataName)
{
    (*DataMap).erase(DataName);
}


//***************************************************************************
// サーフェイス全開放
//***************************************************************************
void __fastcall TChimeraMidi::DeleteAllData(void)
{
    (*DataMap).clear();
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// 再生関係
//***************************************************************************

//***************************************************************************
// 演奏開始
// bool RepeatFlag  リピート
//***************************************************************************
void __fastcall TChimeraMidi::Play(const AnsiString &DataName, bool RepeatFlag)
{
    _ISINIT;

    TChimeraMidiDataMap::iterator pitr; // 反復子
    TChimeraMidiData *Data;

    // プレイ中じゃなければ
    if(!FPlay)
    {
        // 登録されていなかったら例外送出
        pitr = (*DataMap).find(DataName);
        if(pitr == (*DataMap).end())
            throw EChMidi("データが存在しません。\nデータ名："
                + DataName);

        Reset();
        FRepeat = RepeatFlag;

        // MIDIデータ読み込み
        Data = (*pitr).second.Get();
        PlayData = &(Data->GetData());
        *(TShortMessageItr*)SMIterator = PlayData->ShortMessageList->begin();
        *(TLongMessageItr*)LMIterator  = PlayData->LongMessageList->begin();
        *(TMetaEventItr*)MEIterator    = PlayData->MetaEventList->begin();

        // MIDI機器初期化
        MidiReset(ResetGM);

        FPlay = true;
    }
}


//***************************************************************************
// 演奏終了
//***************************************************************************
void __fastcall TChimeraMidi::Stop(void)
{
    _ISINIT;

    if(FPlay)
    {
        FStop = true;
        EndEvent->WaitFor(1000);
    }
    FFadeOut = 0;
    midiOutReset(hMidiOut);
}


//***************************************************************************
// フェードアウト設定
// int Time         フェードアウトまでの時間
//***************************************************************************
void __fastcall TChimeraMidi::SetFadeOut(int Time)
{
    _ISINIT;

    if(!FFadeOut)
    {
        VolumeRate = 1.0f;
        DeltaVolume = 1.0f / Time;
        FFadeOut = Time;
    }
}


//***************************************************************************
// 演奏情報初期化
//***************************************************************************
void __fastcall TChimeraMidi::Reset(void)
{
    // 再生初期化
    DeltaTime   = 0;
    TimeCount   = 0;
    FFadeOut    = 0;
    RepeatPoint = 0;
    MusicTempo  = 500*1000;

    // チャンネル情報初期化
    for(int i = 0; i < MaxChannel; ++i)
    {
        MidiChannels[i].Velocity         = 0x00;
        MidiChannels[i].Volume           = 0x64;
        MidiChannels[i].Pan              = 0x40;
        MidiChannels[i].Expression       = 0x7F;
        MidiChannels[i].Reverb           = 0x28;
        MidiChannels[i].Chorus           = 0x00;
        MidiChannels[i].Delay            = 0x00;
    }

    ChannelCritical->Enter();
    memcpy(&SyncMidiChannels, MidiChannels, sizeof(SyncMidiChannels));
    ChannelCritical->Leave();
}


//***************************************************************************
// 演奏位置初期化
//***************************************************************************
void __fastcall TChimeraMidi::Repeat(void)
{
    // 再生初期化
    DeltaTime  = 0;

    *(TShortMessageItr*)SMIterator = PlayData->ShortMessageList->begin();
    *(TLongMessageItr*)LMIterator  = PlayData->LongMessageList->begin();
    *(TMetaEventItr*)MEIterator    = PlayData->MetaEventList->begin();

    if(RepeatPoint)
    {
        TShortMessageItr &smpitr = *(TShortMessageItr*)SMIterator;
        TLongMessageItr &lmpitr = *(TLongMessageItr*)LMIterator;
        TMetaEventItr &mepitr = *(TMetaEventItr*)MEIterator;
        DeltaTime = RepeatPoint;

        // イベントの送信
        const TMetaEventItr &mend = PlayData->MetaEventList->end();
        while(mepitr != mend && (*mepitr).DeltaTime < DeltaTime)
        {
            if((*mepitr).DataType == meSetTempo)
                MusicTempo = (*mepitr).Data;
            ++mepitr;
        }

        const TLongMessageItr &lend = PlayData->LongMessageList->end();
        while(lmpitr != lend && (*lmpitr).DeltaTime < DeltaTime)
            { ++lmpitr; }

        const TShortMessageItr &send = PlayData->ShortMessageList->end();
        while(smpitr != send && (*smpitr).DeltaTime < DeltaTime)
            { ++smpitr; }
    }
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// MIDI演奏
//***************************************************************************

//***************************************************************************
// SMFデータ演奏
// unsigned long Delay  前回演奏からの時間
//***************************************************************************
void __fastcall TChimeraMidi::SmfPlay(unsigned long Delay)
{
    unsigned long Step;         // １デルタタイム当たりのミリ秒
    unsigned long DeltaCount;

    Step = MusicTempo / 1000;
    DeltaCount = PlayData->TimeBase * Delay;

    TimeCount += DeltaCount;
    DeltaTime += TimeCount / Step;
    TimeCount = TimeCount % Step;   // 残りは次にまわす

    TShortMessageItr &smpitr = *(TShortMessageItr*)SMIterator;
    TLongMessageItr &lmpitr = *(TLongMessageItr*)LMIterator;
    TMetaEventItr &mepitr = *(TMetaEventItr*)MEIterator;

    if(FReport)
    {
        for(int i = 0; i < MaxChannel; ++i)
        {
            if(MidiChannels[i].Velocity > 4)
                MidiChannels[i].Velocity -= 4;
            else
                MidiChannels[i].Velocity = 0;
        }
    }


    // イベントの送信
    const TMetaEventItr &mend = PlayData->MetaEventList->end();
    while(mepitr != mend && (*mepitr).DeltaTime <= DeltaTime)
    {
        if((*mepitr).DataType == meSetTempo)
            { MusicTempo = (*mepitr).Data; }
        else if((*mepitr).DataType == meMarker)
        {
            if((*mepitr).StrData.AnsiCompareIC("RepeatStart") == 0)
                { RepeatPoint = (*mepitr).DeltaTime; }
            else if((*mepitr).StrData.AnsiCompareIC("RepeatEnd") == 0 && FRepeat)
                { Repeat(); return; }
        }
        else if((*mepitr).DataType == meQueue)
        {
            if((*mepitr).StrData.AnsiCompareIC("repeat") == 0)
                { RepeatPoint = (*mepitr).DeltaTime; }
            else if((*mepitr).StrData.AnsiCompareIC("end") == 0 && FRepeat)
                { Repeat(); return; }
        }
        ++mepitr;
    }

    const TLongMessageItr &lend = PlayData->LongMessageList->end();
    while(lmpitr != lend && (*lmpitr).DeltaTime <= DeltaTime)
    {
        LongMsg(*lmpitr);
        ++lmpitr;
    }

    const TShortMessageItr &send = PlayData->ShortMessageList->end();
    while(smpitr != send && (*smpitr).DeltaTime <= DeltaTime)
    {
        ShortMsg(*smpitr);
        ++smpitr;
    }

    // フェードアウト処理
    if(FFadeOut)
    {
        VolumeRate -= DeltaVolume * Delay;
        if(FFadeSend % 32 == 0 && VolumeRate >= 0.0f)
        {
            for(int i = 0; i < MaxChannel; ++i)
                ShortMsg(TShortMessage(0xB0 | i, 0x07,
                    (char)(MidiChannels[i].Volume * VolumeRate)));
        }

        FFadeOut -= Delay;
        ++FFadeSend;
        if(FFadeOut < 0)
        {
            FPlay = false;
            PostMessage(FDummyHWnd, WM_CHMIDIEND, 0, 0);
            Reset();
        }
    }

    if(FReport)
    {
        ChannelCritical->Enter();
        memcpy(&SyncMidiChannels, MidiChannels, sizeof(SyncMidiChannels));
        ChannelCritical->Leave();
    }

    // 送り出すものがなくなったらスレッドをとめる
    if(mepitr == mend && lmpitr == lend && smpitr == send)
    {
        if(FRepeat)
        { Repeat(); }
        else
        { FPlay = false; PostMessage(FDummyHWnd, WM_CHMIDIEND, 0, 0); Reset(); }
    }
}


//***************************************************************************
// ユーザー入力データ演奏
// unsigned long Delay  前回演奏からの時間
//***************************************************************************
void __fastcall TChimeraMidi::UserPlay(unsigned long Delay)
{
    TUserMessageList::iterator pitr;

    UserDataCritical->Enter();
    pitr = UserMessageList->begin();

    while(pitr != UserMessageList->end() && (*pitr).DeltaTime < Delay)
    {
        ShortMsg(*pitr);
        pitr = UserMessageList->erase(pitr);
    }

    for(; pitr != UserMessageList->end(); ++pitr)
        (*pitr).DeltaTime -= Delay;
    UserDataCritical->Leave();
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// MIDI送信処理
//***************************************************************************

//***************************************************************************
// システムエクスクルーシブ送信
// TLongMessage &Msg    送信するデータ
//***************************************************************************
void __fastcall TChimeraMidi::LongMsg(const TLongMessage &Msg)
{
    MIDIHDR MidiHdr;            // システムエクスクルーシブ送信用

    memset(&MidiHdr, 0, sizeof(MidiHdr));
    MidiHdr.lpData = (LPSTR)Msg.Data;
    MidiHdr.dwBufferLength = Msg.DataSize;

    midiOutPrepareHeader(hMidiOut, &MidiHdr, sizeof(MidiHdr));
    for(int i = 0; i < 200; ++i)
        if(midiOutLongMsg(hMidiOut, &MidiHdr, sizeof(MidiHdr))
            != MIDIERR_NOTREADY) break;
    midiOutUnprepareHeader(hMidiOut, &MidiHdr, sizeof(MidiHdr));
}


//***************************************************************************
// メッセージ送信
// TShortMessage &Msg   送信するデータ
//***************************************************************************
void __fastcall TChimeraMidi::ShortMsg(const TShortMessage &Msg)
{
    // ボリュームは別格
    if((Msg.Msg & 0xF0) == 0xB0 && Msg.Param1 == 0x07)
    {
        if(!FFadeOut) MidiChannels[Msg.Msg & 0x0F].Volume = Msg.Param2;
    }

    if(FReport)
    {
        if((Msg.Msg & 0xF0) == 0x90)
        {
            if(Msg.Param2 > MidiChannels[Msg.Msg & 0x0F].Velocity)
                MidiChannels[Msg.Msg & 0x0F].Velocity = Msg.Param2;
        }
        else if((Msg.Msg & 0xF0) == 0xB0)
        {
            switch(Msg.Param1)
            {
//              case 0x07:  // ボリューム
//                  MidiChannels[Msg.Msg & 0x0F].Volume = Msg.Param2;
//                  break;
                case 0x0A:  // パン
                    MidiChannels[Msg.Msg & 0x0F].Pan = Msg.Param2;
                    break;
                case 0x0B:  // エクスプレッション
                    MidiChannels[Msg.Msg & 0x0F].Expression = Msg.Param2;
                    break;
                case 0x5B:  // リバーブ
                    MidiChannels[Msg.Msg & 0x0F].Reverb = Msg.Param2;
                    break;
                case 0x5D:  // コーラス
                    MidiChannels[Msg.Msg & 0x0F].Chorus = Msg.Param2;
                    break;
                case 0x5E:  // ディレイ
                    MidiChannels[Msg.Msg & 0x0F].Delay = Msg.Param2;
                    break;
            }
        }
        else if((Msg.Msg & 0xF0) == 0xC0)
        {
            MidiChannels[Msg.Msg & 0x0F].Program = Msg.Param1;
        }
    }

    midiOutShortMsg(hMidiOut, Msg.Data);
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// 直接演奏
//***************************************************************************

//***************************************************************************
// ノートオン
// unsigned char Channel    演奏チャンネル
// unsigned char Note       演奏鍵盤
// unsigned char Velocity   強さ
// unsigned long Time       演奏時間
//***************************************************************************
void __fastcall TChimeraMidi::NoteOn(unsigned char Channel,
    unsigned char Note, unsigned char Velocity, unsigned long Time)
{
    TUserMessageList::iterator pitr;

    UserDataCritical->Enter();
    UserMessageList->push_front(TShortMessage(0x90 | Channel, Note, Velocity));
    if(Time != 0xFFFFFFFF)
    {
        pitr = UserMessageList->begin();
        while(pitr != UserMessageList->end() && (*pitr).DeltaTime < Time)
            ++pitr;
        UserMessageList->insert(pitr, TShortMessage(Time, 0x80 | Channel, Note, 0));
    }
    UserDataCritical->Leave();
}


//***************************************************************************
// ノートオフ
// unsigned char Channel    演奏チャンネル
// unsigned char Note       演奏鍵盤
//***************************************************************************
void __fastcall TChimeraMidi::NoteOff(unsigned char Channel,
    unsigned char Note)
{
    UserDataCritical->Enter();
    UserMessageList->push_front(TShortMessage(0x80 | Channel, Note, 0));
    UserDataCritical->Leave();
}


//***************************************************************************
// コントロールチェンジ
// unsigned char Channel    演奏チャンネル
// unsigned char Number     コントロールナンバー
// unsigned char Value      値
//***************************************************************************
void __fastcall TChimeraMidi::ControlChange(unsigned char Channel,
    unsigned char Number, unsigned char Value)
{
    UserDataCritical->Enter();
    UserMessageList->push_front(TShortMessage(0xB0 | Channel, Number, Value));
    UserDataCritical->Leave();
}


//***************************************************************************
// プログラムチェンジ
// unsigned char Channel    演奏チャンネル
// unsigned char Number     プログラムナンバー
//***************************************************************************
void __fastcall TChimeraMidi::ProgramChange(unsigned char Channel,
    unsigned char Number)
{
    UserDataCritical->Enter();
    UserMessageList->push_front(TShortMessage(0xC0 | Channel, Number, 0));
    UserDataCritical->Leave();
}


//***************************************************************************
// チャンネルプレッシャー
// unsigned char Channel    演奏チャンネル
// unsigned char Pressure   チャンネルプレッシャー
//***************************************************************************
void __fastcall TChimeraMidi::ChannelPressure(unsigned char Channel,
    unsigned char Pressure)
{
    UserDataCritical->Enter();
    UserMessageList->push_front(TShortMessage(0xD0 | Channel, Pressure, 0));
    UserDataCritical->Leave();
}


//***************************************************************************
// チャンネルプレッシャー
// unsigned char Channel    演奏チャンネル
// unsigned char PitchBend  ピッチベンド
//***************************************************************************
void __fastcall TChimeraMidi::PitchBend(unsigned char Channel,
    unsigned char PitchBend)
{
    UserDataCritical->Enter();
    UserMessageList->push_front(TShortMessage(0xE0 | Channel, PitchBend, 0));
    UserDataCritical->Leave();
}





/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// 音源情報取得
//***************************************************************************

//***************************************************************************
// レポート設定
//***************************************************************************
void __fastcall TChimeraMidi::SetReport(bool Value)
{
    FReport = Value;
}


//***************************************************************************
// 再生情報取得
//***************************************************************************
void __fastcall TChimeraMidi::GetChannelData(TChimeraMidiChannel *Channels)
{
    if(FReport)
    {
        ChannelCritical->Enter();
        memcpy(Channels, SyncMidiChannels, sizeof(SyncMidiChannels));
        ChannelCritical->Leave();
    }
    else
    {
        EChMidi("Reportプロパティは、falseになっています。");
    }
}


//***************************************************************************
// 現在のデルタタイム
//***************************************************************************
unsigned long __fastcall TChimeraMidi::GetDeltaTime(void)
{
    return DeltaTime;
}


//***************************************************************************
// 現在のテンポ
//***************************************************************************
unsigned long __fastcall TChimeraMidi::GetMusicTempo(void)
{
    return MusicTempo;
}







/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// 音源操作
//***************************************************************************

//***************************************************************************
// 音源リセット
// int Reset        リセットの種類
//***************************************************************************
void __fastcall TChimeraMidi::MidiReset(TMidiReset Reset)
{
    _ISINIT;

    unsigned char GMReset[] = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
    unsigned char GSReset[] = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40,
        0x00, 0x7F, 0x00, 0x41, 0xF7};

    switch(Reset)
    {
        case ResetGM:
            LongMsg(TLongMessage(GMReset, sizeof(GMReset)));
            break;
        case ResetGS:
            LongMsg(TLongMessage(GSReset, sizeof(GSReset)));
            break;
    }
}


//***************************************************************************
// リバーブセット
// int reverb   リバーブの種類
//***************************************************************************
void __fastcall TChimeraMidi::SetGSReverb(TGSReverb Reverb)
{
    _ISINIT;

    unsigned char GSReverb[11] =
        { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x30, 0x00, 0x00, 0xF7 };

    GSReverb[8] = (unsigned char)Reverb;
    GSReverb[9] = (unsigned char)(0x0F - Reverb);

    LongMsg(TLongMessage(GSReverb, 11));
}


//***************************************************************************
// コーラスセット
// int Chorus   コーラスの種類
//***************************************************************************
void __fastcall TChimeraMidi::SetGSChorus(TGSChorus Chorus)
{
    _ISINIT;

    unsigned char GSChorus[11] =
        { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x38, 0x00, 0x00, 0xF7 };

    GSChorus[8] = (unsigned char)Chorus;
    GSChorus[9] = (unsigned char)(0x07 - Chorus);

    LongMsg(TLongMessage(GSChorus, 11));
}


//***************************************************************************
// ディレイセット
// int Delay    ディレイの種類
//***************************************************************************
void __fastcall TChimeraMidi::SetGSDelay(TGSDelay Delay)
{
    _ISINIT;

    unsigned char GSDelay[11] =
        { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x50, 0x00, 0x00, 0xF7 };

    GSDelay[8] = (unsigned char)Delay;
    GSDelay[9] = (unsigned char)(0x6F - Delay);

    LongMsg(TLongMessage(GSDelay, 11));
}










/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChimeraMidiPlayThread
// 内容      : MIDI演奏スレッド
// Version   : 0.8 (2001/01/04)
//***************************************************************************

//***************************************************************************
// コンストラクタ
//***************************************************************************
__fastcall TChimeraMidiPlayThread::TChimeraMidiPlayThread(TChimeraMidi *AOwner,
    unsigned long ADelay)
    : TThread(true), Owner(AOwner), Delay(ADelay)
{
    FreeOnTerminate = false;
}


//***************************************************************************
// デストラクタ
//***************************************************************************
__fastcall TChimeraMidiPlayThread::~TChimeraMidiPlayThread(void)
{
}


//***************************************************************************
// 実行スレッド
//***************************************************************************
void __fastcall TChimeraMidiPlayThread::Execute(void)
{
    unsigned long NowTime;
    unsigned long PrevTime;
    PrevTime = timeGetTime();

    while(!Terminated)
    {
        NowTime = timeGetTime();
        if(Owner->FPlay)
        {
            Owner->SmfPlay(NowTime - PrevTime);
        }

        if(Owner->FStop)
        {
            Owner->EndEvent->SetEvent();
            Owner->FPlay = false;
            Owner->FStop = false;
            PostMessage(Owner->FDummyHWnd, WM_CHMIDIEND, 0, 0);
            Owner->Reset();
        }

        Owner->UserPlay(NowTime - PrevTime);

        PrevTime = NowTime;
        Sleep(Delay);
    }
}










/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChimeraMidiData
// 内容      : SMFデータ保持＆読み込み処理
// Version   : 0.8 (2001/01/3)
//***************************************************************************

//***************************************************************************
// コンストラクタ
//***************************************************************************
__fastcall TChimeraMidiData::TChimeraMidiData(void)
{
}


//***************************************************************************
// デストラクタ
//***************************************************************************
__fastcall TChimeraMidiData::~TChimeraMidiData(void)
{
}


//***************************************************************************
// MIDIファイル読み込み
// const AnsiString &AFileName  MIDIファイルのリソース名もしくはファイルパス
//***************************************************************************
void __fastcall TChimeraMidiData::LoadFile(const AnsiString &AFileName)
{
    struct TSMFHeader {
        char ID[4];
        unsigned char Size[4];
        unsigned char Format[2];
        unsigned char TrackCount[2];
        unsigned char TimeBase[2];
    };

    struct TSMFTrackHeader {
        char ID[4];
        unsigned char Size[4];
    };

    TStream *SMFStrm;           // ストリーム
    TSMFHeader SmfHeader;       // SMFファイルヘッダ取得用
    TSMFTrackHeader SmfTrackHeader; // トラックヘッダ取得用
    unsigned long TrackCount;   // トラック数
    unsigned char *Data;
    unsigned long DataSize;

    // ファイルの検索
    try {
        SMFStrm = new TResourceStream((int)hInst, AFileName, "MIDI");
    } catch(...) {
        try {
            SMFStrm = new TFileStream(AFileName, fmOpenRead | fmShareDenyWrite);
        } catch(...) {
            throw EChMidi(
                "MIDIの読み込みに失敗しました。\nファイル名："
                + AnsiString(AFileName));
        }
    }

    try
    {
        // データの初期化
        SMFData.Clear();

        // ヘッダの読み取り
        if(SMFStrm->Read(&SmfHeader, 14) != 14
                || strncmp(SmfHeader.ID, "MThd", 4) != 0)
            throw EChMidi(
                "ファイルヘッダが不正です。\nファイル名："
                + AnsiString(AFileName));

        // 情報を取り込む
        TrackCount = ByteToWord(SmfHeader.TrackCount);
        SMFData.MidiFormat = ByteToWord(SmfHeader.Format);
        SMFData.TimeBase   = ByteToWord(SmfHeader.TimeBase);

        // SMFformat0か1以外は処理しない
        if((SMFData.MidiFormat != 0) && (SMFData.MidiFormat != 1))
            throw EChMidi(
                "SMFFormat0またはSMFFormat1ではありません。\nファイル名："
                + AnsiString(AFileName));

        for(unsigned long i = 0; i < TrackCount; ++i)
        {
            // トラックのヘッダの読みとり
            if(SMFStrm->Read(&SmfTrackHeader, 8) != 8
                    || strncmp(SmfTrackHeader.ID, "MTrk", 4) != 0)
                throw EChMidi(
                    "トラックヘッダが不正です。\nファイル名："
                    + AnsiString(AFileName));

            // 情報を取り込む
            DataSize = ByteToDword(SmfTrackHeader.Size);
            Data     = new unsigned char[DataSize];

            if(SMFStrm->Read(Data, DataSize) != (int)DataSize)
            {
                delete[] Data;
                throw EChMidi(
                    "ファイルデータが不正です。\nファイル名："
                    + AnsiString(AFileName));
            }

            ReadTrack(Data, DataSize);
            delete[] Data;
        }
    }
    __finally
    {
        delete SMFStrm;
    }
}


//***************************************************************************
// １トラック分解析
// unsigned char *Data      トラックデータ
// unsigned long DataSize   データサイズ
//***************************************************************************
void __fastcall TChimeraMidiData::ReadTrack(unsigned char *Data,
    unsigned long DataSize)
{
    bool Ret;                   // 終了フラグ
    unsigned char LastMsg;      // 前回のメッセージ
    unsigned long DeltaTime;    // デルタタイム
    unsigned long CurrentPos;   // 現在の読み込み位置

    TShortMessageItr pShortMsg;
    TLongMessageItr pLongMsg;
    TMetaEventItr pMetaEvt;

    // 準備
    Ret = true;
    DeltaTime = 0;
    CurrentPos = 0;

    pShortMsg = SMFData.ShortMessageList->begin();
    pLongMsg  = SMFData.LongMessageList->begin();
    pMetaEvt  = SMFData.MetaEventList->begin();

    while(Ret && CurrentPos < DataSize)
    {
        DeltaTime += GetValue(Data, CurrentPos);

        if(*(Data + CurrentPos) & 0x80)
            { LastMsg = *(Data + (CurrentPos++)); }

        switch(LastMsg)
        {
            case 0xFF:  // メタイベント
                Ret = MetaEvent(Data, CurrentPos, LastMsg,
                    DeltaTime, &pMetaEvt);
                break;

            case 0xF0:  // システムエクスクルーシブ
            case 0xF7:  // システムエクスクルーシブ
                Ret = SystemExclusive(Data, CurrentPos, LastMsg,
                    DeltaTime, &pLongMsg);
                break;

            default:    // その他
                Ret = DefaultEvent(Data, CurrentPos, LastMsg,
                    DeltaTime, &pShortMsg);
                break;
        }
    }

    if(DeltaTime > SMFData.EndDeltaTime)
        SMFData.EndDeltaTime = DeltaTime;

}


//***************************************************************************
// 通常イベント読み込み
// unsigned char *Data  トラックデータの先頭
// unsigned long &CurrentPos    現在のポジション
// unsigned char Msg    処理に出されているメッセージ
// unsigned long DeltaTime  現在のデルタタイム
// void *Node       リストの反復子（コンパイル高速化のためvoid*に変換）
//***************************************************************************
bool __fastcall TChimeraMidiData::DefaultEvent(
    unsigned char *Data, unsigned long &CurrentPos,
    unsigned char Msg, unsigned long DeltaTime, void *Node)
{
    TShortMessageItr &pitr = *((TShortMessageItr*)Node);
    const TShortMessageItr &end = SMFData.ShortMessageList->end();

    // 入力位置までリストを進める
    while(pitr != end && (*pitr).DeltaTime <= DeltaTime)
        pitr++;

    unsigned char Param1, Param2;

    switch(Msg & 0xF0)
    {
        case 0xC0:  // プログラムチェンジ
        case 0xD0:  // チャンネルプレッシャー
            Param1 = *(Data + (CurrentPos++));
            SMFData.ShortMessageList->insert(pitr,
                TShortMessage(DeltaTime, Msg, Param1, 0));
            break;
        default:    // 上の以外
            Param1 = *(Data + (CurrentPos++));
            Param2 = *(Data + (CurrentPos++));
            SMFData.ShortMessageList->insert(pitr,
                TShortMessage(DeltaTime, Msg, Param1, Param2));
            break;
    }
    return true;
}


//***************************************************************************
// システムエクスクルーシブ読み込み
// unsigned char *Data  トラックデータの先頭
// unsigned long &CurrentPos    現在のポジション
// unsigned char Msg    処理に出されているメッセージ
// unsigned long DeltaTime  現在のデルタタイム
// void *Node       リストの反復子（コンパイル高速化のためvoid*に変換）
//***************************************************************************
bool __fastcall TChimeraMidiData::SystemExclusive(
    unsigned char *Data, unsigned long &CurrentPos,
    unsigned char Msg, unsigned long DeltaTime, void *Node)
{
    TLongMessageItr &pitr = *((TLongMessageItr*)Node);
    const TLongMessageItr &end = SMFData.LongMessageList->end();

    // 入力位置までリストを進める
    while(pitr != end && (*pitr).DeltaTime <= DeltaTime)
        pitr++;

    unsigned long Length;       // データの長さ
    unsigned char *Buffer;      // データ用バッファ

    // データの長さを取得
    Length = GetValue(Data, CurrentPos);

    // バッファ領域を作る
    Buffer = new unsigned char[Length + 1];

    // バッファの始めにシステムエクスクルーシブを書き込む
    Buffer[0] = Msg;
    memcpy(&Buffer[1], Data + CurrentPos, Length);
    SMFData.LongMessageList->insert(pitr,
        TLongMessage(DeltaTime, Buffer, Length + 1));
    // 領域削除
    delete[] Buffer;

    CurrentPos += Length;

    return true;
}


//***************************************************************************
// メタイベント読み込み
// unsigned char *Data  トラックデータの先頭
// unsigned long &CurrentPos    現在のポジション
// unsigned char Msg    処理に出されているメッセージ
// unsigned long DeltaTime  現在のデルタタイム
// void *Node       リストの反復子（コンパイル高速化のためvoid*に変換）
//***************************************************************************
bool __fastcall TChimeraMidiData::MetaEvent(
    unsigned char *Data, unsigned long &CurrentPos,
    unsigned char Msg, unsigned long DeltaTime, void *Node)
{
    TMetaEventItr &pitr = *((TMetaEventItr*)Node);
    const TMetaEventItr &end = SMFData.MetaEventList->end();

    // 入力位置までリストを進める
    while(pitr != end && (*pitr).DeltaTime <= DeltaTime)
        pitr++;

    bool Result;
    char Buffer[256];           // データ取得用バッファ
    unsigned char EventType;    // メタイベントの種類
    unsigned long Length;       // データの長さ
    unsigned char *DataPtr;     // データへのポインタ

    Result = true;

    EventType = *(Data + (CurrentPos++));

    // データの長さを取得
    Length = GetValue(Data, CurrentPos);

    // データの先頭ポインタ
    DataPtr = Data + CurrentPos;

    switch(EventType)
    {
        case meSequenceNumber:  //シーケンス番号
            break;

        case meText:            // テキスト
            memset(Buffer, 0, sizeof(Buffer));
            memcpy(Buffer, DataPtr, Length);
            SMFData.Text->Add(Buffer);
            break;

        case meCreator:         // 著作権
            memset(Buffer, 0, sizeof(Buffer));
            memcpy(Buffer, DataPtr, Length);
            SMFData.Creator->Add(Buffer);
            break;

        case meSequenceTrackName:   // 曲／トラック名
            memset(Buffer, 0, sizeof(Buffer));
            memcpy(Buffer, DataPtr, Length);
            SMFData.SongName->Add(Buffer);
            break;

        case meInstrument:      // 楽器名
            break;

        case meSong:            // 歌詞
            memset(Buffer, 0, sizeof(Buffer));
            memcpy(Buffer, DataPtr, Length);
            SMFData.MetaEventList->insert(pitr,
                TMetaEvent(DeltaTime, meSong, 0, Buffer));
            break;

        case meMarker:          // マーカー
            memset(Buffer, 0, sizeof(Buffer));
            memcpy(Buffer, DataPtr, Length);
            SMFData.MetaEventList->insert(pitr,
                TMetaEvent(DeltaTime, meMarker, 0, Buffer));
            break;

        case meQueue:           // キュー
            memset(Buffer, 0, sizeof(Buffer));
            memcpy(Buffer, DataPtr, Length);
            SMFData.MetaEventList->insert(pitr,
                TMetaEvent(DeltaTime, meQueue, 0, Buffer));
            break;

        case meChannelPrefix:   // MIDIチャンネルプリフィクス
            break;

        case meEndOfTrack:      // エンドオブトラック
            Result = false;
            break;

        case meSetTempo:        // テンポ
            SMFData.MetaEventList->insert(pitr,
                TMetaEvent(DeltaTime, meSetTempo,
                (*(DataPtr)<<16) | (*(DataPtr+1)<<8) | *(DataPtr+2),
                ""));
            break;

        case meBar:             // 拍子
            SMFData.MusicBarChild  = *(DataPtr+0);
            SMFData.MusicBarMother = *(DataPtr+1);
            SMFData.MusicBarDelta  = *(DataPtr+2);
            break;

        case meKey:             // 調
            break;

        case meIDMetaEvent:     // 固有メタイベント
            break;
    }

    // 演奏位置を進める
    CurrentPos += Length;

    return Result;
}










/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChimeraMidiDataInfo
// 内容      : SMFデータの保持(TChimeraMidiDataの所有クラス)
// Version   : 0.8 (2001/01/03)
//***************************************************************************

//***************************************************************************
// コンストラクタ
//***************************************************************************
__fastcall TChimeraMidiDataInfo::TChimeraMidiDataInfo(void)
    : MidiFormat(0), TimeBase(0), EndDeltaTime(0), MusicBarChild(0),
      MusicBarMother(0), MusicBarDelta(0)
{
    ShortMessageList = new TShortMessageList();
    LongMessageList = new TLongMessageList();
    MetaEventList = new TMetaEventList;

    Text     = new TStringList();
    Creator  = new TStringList();
    SongName = new TStringList();
}


//***************************************************************************
// デストラクタ
//***************************************************************************
__fastcall TChimeraMidiDataInfo::~TChimeraMidiDataInfo(void)
{
    delete Text;
    delete Creator;
    delete SongName;

    delete ShortMessageList;
    delete LongMessageList;
    delete MetaEventList;
}


//***************************************************************************
// 初期化
//***************************************************************************
void __fastcall TChimeraMidiDataInfo::Clear(void)
{
    Text->Clear();
    Creator->Clear();
    SongName->Clear();

    ShortMessageList->clear();
    LongMessageList->clear();
    MetaEventList->clear();
}
