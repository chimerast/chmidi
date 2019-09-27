//---------------------------------------------------------------------------
#ifndef chmiditestH
#define chmiditestH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ToolWin.hpp>
#include <Dialogs.hpp>
#include "chmidi.h"
//---------------------------------------------------------------------------
typedef void __fastcall (__closure*TThreadProc)(void);
//---------------------------------------------------------------------------
struct TMidiDataInfo
{
    unsigned long MidiFormat;           // フォーマットの種類
    unsigned long TimeBase;             // MIDIファイルのベース
    unsigned long EndDeltaTime;         // 最終デルタタイム
    unsigned long MusicBarChild;        // 拍子（分子）
    unsigned long MusicBarMother;       // 拍子（分母）
    unsigned long MusicBarDelta;        // 拍子MIDIクロック

    AnsiString Creator;                 // 著作権表示
    AnsiString SongName;                // 曲名＆トラック名
};
//---------------------------------------------------------------------------
class TExecuteThread : public TThread
{
protected:
    TThreadProc Proc;
    virtual void __fastcall Execute(void);
public:
    __fastcall TExecuteThread(TThreadProc AProc);
    virtual __fastcall ~TExecuteThread(void);
};
//---------------------------------------------------------------------------
class TChMidiTestForm : public TForm
{
__published:
    TPaintBox *StatusPanel;
    TBevel *Bevel1;
    TBevel *Bevel2;
    TToolBar *ToolTip;
    TToolButton *ToolPlay;
    TToolBar *SystemTip;
    TToolButton *MnimizeTip;
    TToolButton *CloseTip;
    TToolButton *ToolOpen;
    TOpenDialog *OpenDialog1;
    TToolButton *ToolStop;
    void __fastcall ToolPlayClick(TObject *Sender);
    void __fastcall CloseTipClick(TObject *Sender);
    void __fastcall MnimizeTipClick(TObject *Sender);
    void __fastcall ToolOpenClick(TObject *Sender);
    void __fastcall ToolStopClick(TObject *Sender);

protected:
    int PeekVelocity[16];
    AnsiString FileName;
    TMidiDataInfo PlayInfo;

    TExecuteThread *StatusPanelUpdateThread;
    Graphics::TBitmap *PanelBack;

    virtual void __fastcall StatusPanelUpdate(void);
    virtual void __fastcall DrawFileInfo(int X, int Y);
    virtual void __fastcall DrawPlayingStatus(int X, int Y);
    virtual void __fastcall DrawVelocity(int X, int Y, TChMidiChannel *Channels);

    virtual void __fastcall LoadPlayMidiData(const AnsiString &FileName);

    virtual void __fastcall CreateParams(TCreateParams &Params);
    virtual void __fastcall WndProc(TMessage &Message);

public:
    __fastcall TChMidiTestForm(TComponent* Owner);
    __fastcall ~TChMidiTestForm(void);
};
//---------------------------------------------------------------------------
extern PACKAGE TChMidiTestForm *ChMidiTestForm;
//---------------------------------------------------------------------------
#endif
