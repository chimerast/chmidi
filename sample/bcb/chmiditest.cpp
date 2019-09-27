//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "chmiditest.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TChMidiTestForm *ChMidiTestForm;
//---------------------------------------------------------------------------
__fastcall TExecuteThread::TExecuteThread(TThreadProc AProc)
    : TThread(true), Proc(AProc)
    { ; }
//---------------------------------------------------------------------------
__fastcall TExecuteThread::~TExecuteThread(void)
    { ; }
//---------------------------------------------------------------------------
void __fastcall TExecuteThread::Execute(void)
    { while(!Terminated) Proc(); }
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChMidiTestForm
// 内容      : TChiemraMidiテスト用プログラム
// Version   : 0.75 (2001/01/12)
//***************************************************************************

//***************************************************************************
// コンストラクタ
//***************************************************************************
__fastcall TChMidiTestForm::TChMidiTestForm(TComponent* Owner)
    : TForm(Owner)
{
    memset(PeekVelocity, 0, sizeof(PeekVelocity));

    PlayInfo.MidiFormat     = 0;
    PlayInfo.TimeBase       = 0;
    PlayInfo.EndDeltaTime   = 0;
    PlayInfo.MusicBarChild  = 0;
    PlayInfo.MusicBarMother = 0;
    PlayInfo.MusicBarDelta  = 0;

    ChMidiInit(Handle, HInstance);

    PanelBack = new Graphics::TBitmap();
    PanelBack->Width  = StatusPanel->Width;
    PanelBack->Height = StatusPanel->Height;

    StatusPanelUpdateThread = new TExecuteThread(&StatusPanelUpdate);
    StatusPanelUpdateThread->Priority = tpTimeCritical;
    StatusPanelUpdateThread->Resume();
}


//***************************************************************************
// デストラクタ
//***************************************************************************
__fastcall TChMidiTestForm::~TChMidiTestForm(void)
{
    // スレッドが終わるのを待つ
    StatusPanelUpdateThread->Terminate();
    StatusPanelUpdateThread->WaitFor();

    delete PanelBack;
    delete StatusPanelUpdateThread;
}


//***************************************************************************
// 作成パラメータ
//***************************************************************************
void __fastcall TChMidiTestForm::CreateParams(TCreateParams &Params)
{
    // フレームと最小化ボタンを付け加える
    TForm::CreateParams(Params);
    Params.Style |= WS_DLGFRAME;
    Params.Style |= WS_MINIMIZEBOX;
}


//***************************************************************************
// ウィンドウプロシージャ
//***************************************************************************
void __fastcall TChMidiTestForm::WndProc(TMessage &Message)
{
    switch(Message.Msg)
    {
        case WM_MOVING:
        {
            // 位置変更の限定
            RECT WorkScreen;
            LPRECT lprc = (LPRECT)Message.LParam;
            SystemParametersInfo(SPI_GETWORKAREA,0,&WorkScreen,0);

            if(lprc->left < WorkScreen.left + 10)
            {
                lprc->left = 0;
                lprc->right = Width;
            }
            if(lprc->top < WorkScreen.top + 10)
            {
                lprc->top = 0;
                lprc->bottom = Height;
            }
            if(lprc->right >= WorkScreen.right - 10)
            {
                lprc->left = WorkScreen.right - Width;
                lprc->right = WorkScreen.right;
            }
            if(lprc->bottom >= WorkScreen.bottom - 10)
            {
                lprc->top = WorkScreen.bottom - Height;
                lprc->bottom = WorkScreen.bottom;
            }
            break;
        }

        case WM_NCHITTEST:
        {
            // フォームの一部をタイトルバーとして扱う
            TPoint MousePosition(Message.LParamLo, Message.LParamHi);
            MousePosition = ScreenToClient(MousePosition);

            if(MousePosition.y < Bevel2->Height) Message.Result = HTCAPTION;
            else TForm::WndProc(Message);
            break;
        }

        default:
        {
            TForm::WndProc(Message);
            break;
        }
    }
}


//***************************************************************************
// 演奏ファイル読み込み
// const AnsiString &FileName   ファイル名
//***************************************************************************
void __fastcall TChMidiTestForm::LoadPlayMidiData(const AnsiString &FileName)
{
    TChMidiDataInfo Data;

    // 演奏を終了させDLL内部のSMFデータをすべて消去 
    ChMidiStop();
    ChMidiDeleteAllData();

    // SMFデータを"PlayingData"という名前で構築
    ChMidiCreateData("PlayingData", FileName.c_str());

    // "PlayingData"という名前のSMFデータの情報を取得
    ChMidiGetDataInfo("PlayingData", &Data);
    PlayInfo.Creator        = Data.Creator;
    PlayInfo.SongName       = Data.SongName;
    PlayInfo.MidiFormat     = Data.MidiFormat;
    PlayInfo.TimeBase       = Data.TimeBase;
    PlayInfo.EndDeltaTime   = Data.EndDeltaTime;
    PlayInfo.MusicBarChild  = Data.MusicBarChild;
    PlayInfo.MusicBarMother = Data.MusicBarMother;
    PlayInfo.MusicBarDelta  = Data.MusicBarDelta;
}


//***************************************************************************
// ステータスパネル更新(スレッド使用)
//***************************************************************************
void __fastcall TChMidiTestForm::StatusPanelUpdate(void)
{
    TChMidiChannel Channels;

    // 現在のMIDIチャンネルの状態を取得
    ChMidiGetChannelData(&Channels);

    // ピークホールド
    for(int i = 0; i < 16; ++i) {
        if(Channels.Velocity[i] > PeekVelocity[i])
            PeekVelocity[i] = Channels.Velocity[i];
        else if(PeekVelocity[i] > 2)
            PeekVelocity[i] -= 2;
        else
            PeekVelocity[i] = 0;
    }

    PanelBack->Canvas->Lock();
    PanelBack->Canvas->Brush->Color = clWhite;
    PanelBack->Canvas->FillRect(StatusPanel->ClientRect);

    PanelBack->Canvas->Font->Name = "ＭＳ ゴシック";

    // 画面を描画
    DrawVelocity(2, 2, &Channels);
    DrawFileInfo(132, 2);
    DrawPlayingStatus(132, 50);

    // 転送(スレッドを使っているためロック)
    StatusPanel->Canvas->Lock();
    StatusPanel->Canvas->CopyRect(StatusPanel->ClientRect, PanelBack->Canvas, StatusPanel->ClientRect);
    StatusPanel->Canvas->Unlock();
    PanelBack->Canvas->Unlock();

    Sleep(20);
}



//***************************************************************************
// 曲情報描画
//***************************************************************************
void __fastcall TChMidiTestForm::DrawFileInfo(int X, int Y)
{
    PanelBack->Canvas->Font->Height = 12;
    PanelBack->Canvas->Font->Color  = (TColor)0x00007F00;
    PanelBack->Canvas->Brush->Style = bsSolid;
    PanelBack->Canvas->Brush->Color = (TColor)0x00BFFFBF;

    PanelBack->Canvas->TextRect(
        Rect(X+1,  Y+1, X+39,  Y+15), X+2,  Y+2, "File名");
    PanelBack->Canvas->TextRect(
        Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2,
        FileName!="" ? FileName : AnsiString("選択されていません。"));
    Y += 16;
    PanelBack->Canvas->TextRect(
        Rect(X+1,  Y+1, X+39,  Y+15), X+2,  Y+2, "音楽名");
    PanelBack->Canvas->TextRect(
        Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2, PlayInfo.SongName);
    Y += 16;
    PanelBack->Canvas->TextRect(
        Rect(X+1,  Y+1, X+39,  Y+15), X+2,  Y+2, "著作権");
    PanelBack->Canvas->TextRect(
        Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2, PlayInfo.Creator);
}


//***************************************************************************
// 再生情報描画
//***************************************************************************
void __fastcall TChMidiTestForm::DrawPlayingStatus(int X, int Y)
{
    int Tempo;
    int DeltaTime;
    unsigned long BPS;
    unsigned long Base;

    ChMidiGetMusicTempo(&Tempo);
    ChMidiGetDeltaTime(&DeltaTime);
    BPS = 60 * 1000 * 1000 / Tempo;
    Base = PlayInfo.TimeBase;

    if(Base == 0) Base = 480;
    if(PlayInfo.MusicBarChild == 0) PlayInfo.MusicBarChild = 4;
    if(PlayInfo.EndDeltaTime == 0) PlayInfo.EndDeltaTime = 9999*Base*4;

    PanelBack->Canvas->Brush->Style = bsSolid;
    PanelBack->Canvas->Brush->Color = (TColor)0x00BFBFFF;
    PanelBack->Canvas->FillRect(Rect(X+1, Y+17,
        X+1 + 244 * DeltaTime / PlayInfo.EndDeltaTime, Y+23));


    PanelBack->Canvas->Font->Height = 12;
    PanelBack->Canvas->Font->Color  = (TColor)0x007F0000;

    PanelBack->Canvas->Brush->Style = bsSolid;
    PanelBack->Canvas->Brush->Color = (TColor)0x00FFBFBF;

    PanelBack->Canvas->TextRect(Rect(X+1, Y+1, X+39, Y+15), X+2, Y+2,
        Format("%03dBPS", ARRAYOFCONST(((int)BPS))));
    X += 40;

    PanelBack->Canvas->TextRect(
        Rect(X+1, Y+1, X+57, Y+15), X+2, Y+2,
        AnsiString().Format("%04d/%04d",
            ARRAYOFCONST(((int)(DeltaTime / Base / PlayInfo.MusicBarChild),
                (int)(PlayInfo.EndDeltaTime / Base / PlayInfo.MusicBarChild)))
        )
    );
    X += 58;

    PanelBack->Canvas->TextRect(
        Rect(X+1, Y+1, X+33, Y+15), X+2, Y+2,
        AnsiString().Format("%02d/%02d",
            ARRAYOFCONST(((int)((DeltaTime / Base) % PlayInfo.MusicBarChild + 1),
                (int)PlayInfo.MusicBarChild))
        )
    );
    X += 34;

    PanelBack->Canvas->TextRect(
        Rect(X+1, Y+1, X+57, Y+15), X+2, Y+2,
        AnsiString().Format("%04d/%04d",
            ARRAYOFCONST(((int)(DeltaTime % (Base * PlayInfo.MusicBarChild)),
                (int)(Base * PlayInfo.MusicBarChild)))
        )
    );
    X += 58;

    PanelBack->Canvas->TextRect(
        Rect(X+1, Y+1, X+65, Y+15), X+2, Y+2,
        AnsiString().Format("作:chimera",
            ARRAYOFCONST(((int)(DeltaTime % (Base * PlayInfo.MusicBarChild)),
                (int)(Base * PlayInfo.MusicBarChild)))
        )
    );
}


//***************************************************************************
// ベロシティ描画
//***************************************************************************
void __fastcall TChMidiTestForm::DrawVelocity(int X, int Y, TChMidiChannel *Channels)
{
    PanelBack->Canvas->Brush->Color = (TColor)0x00007F00;
    PanelBack->Canvas->Brush->Style = bsSolid;
    for(int i = 0; i < 16; ++i)
    {
        for(int j = 0; j < (Channels->Velocity[i] + 1) / 8; ++j)
        {
            PanelBack->Canvas->FillRect(Rect(
                X + i*8,
                Y + 64 - (j+1)*4 + 1,
                X + (i+1)*8 - 1,
                Y + 64 - j*4
            ));
        }
        PanelBack->Canvas->FillRect(Rect(
            X + i*8,
            Y + 64 - (PeekVelocity[i]/8+1)*4 + 1,
            X + (i+1)*8 - 1,
            Y + 64 - PeekVelocity[i]/8*4
        ));
    }

    PanelBack->Canvas->Font->Color = clGreen;
    PanelBack->Canvas->Font->Height  = 8;
    PanelBack->Canvas->Brush->Style = bsClear;
    for(int i = 0; i < 16; ++i)
    {
        PanelBack->Canvas->TextOut(
            X + i*8 + 3 - PanelBack->Canvas->TextWidth(i+1)/2, Y + 64, i+1);
    }
}


//***************************************************************************
// ツールボタンイベント
//***************************************************************************
void __fastcall TChMidiTestForm::ToolPlayClick(TObject *Sender)
{
    if(FileName != "")
    {
        ChMidiPlay("PlayingData", true);
    }
    else
    {
        ToolOpenClick(NULL);
        ChMidiPlay("PlayingData", true);
    }
}
//---------------------------------------------------------------------------
void __fastcall TChMidiTestForm::ToolStopClick(TObject *Sender)
{
    ChMidiStop();
}
//---------------------------------------------------------------------------
void __fastcall TChMidiTestForm::ToolOpenClick(TObject *Sender)
{
    if(OpenDialog1->Execute())
    {
        FileName = OpenDialog1->FileName;
        ChMidiStop();
        LoadPlayMidiData(FileName);
    }
}
//---------------------------------------------------------------------------
void __fastcall TChMidiTestForm::CloseTipClick(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------
void __fastcall TChMidiTestForm::MnimizeTipClick(TObject *Sender)
{
    Application->Minimize();
}
//---------------------------------------------------------------------------
