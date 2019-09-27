unit chmiditest;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls, ComCtrls, ToolWin, ChMidi;

type
  // 実行用関数
  TThreadProc = procedure of object;
  //------------------------------------------------------

  // SmfMidiFileのデータ
  TMidiDataInfo = Record
    MidiFormat: Longword;     // フォーマットの種類
    TimeBase: Longword;       // MIDIファイルのベース
    EndDeltaTime: Longword;   // 最終デルタタイム
    MusicBarChild: Longword;  // 拍子（分子）
    MusicBarMother: Longword; // 拍子（分母）
    MusicBarDelta: Longword;  // 拍子MIDIクロック

    Creator: AnsiString;      // 著作権表示
    SongName: AnsiString;     // 曲名＆トラック名
  end;
  //------------------------------------------------------

  // 実行スレッド
  TExecuteThread = class(TThread)
  protected
    Proc: TThreadProc;
    procedure Execute; override;
  public
    constructor Create(AProc: TThreadProc);
    destructor Destroy; override;
  end;
  //------------------------------------------------------

  // Midi再生フォーム
  TChMidiTestForm = class(TForm)
    StatusPanel: TPaintBox;
    OpenDialog1: TOpenDialog;
    Bevel1: TBevel;
    Bevel2: TBevel;
    ToolTip: TToolBar;
    ToolPlay: TToolButton;
    ToolStop: TToolButton;
    ToolOpen: TToolButton;
    SystemTip: TToolBar;
    MnimizeTip: TToolButton;
    CloseTip: TToolButton;
    procedure ToolPlayClick(Sender: TObject);
    procedure ToolStopClick(Sender: TObject);
    procedure ToolOpenClick(Sender: TObject);
    procedure MnimizeTipClick(Sender: TObject);
    procedure CloseTipClick(Sender: TObject);
  private
  protected
    PeekVelocity: array[0..15] of Integer;
    FileName: AnsiString;
    PlayInfo: TMidiDataInfo;

    StatusPanelUpdateThread: TExecuteThread;
    PanelBack: TBitmap;

    procedure StatusPanelUpdate; virtual;

    procedure DrawFileInfo(X, Y: Integer); virtual;
    procedure DrawPlayingStatus(X, Y: Integer); virtual;
    procedure DrawVelocity(X, Y: Integer; var Channels: TChMidiChannel); virtual;

    procedure LoadPlayMidiData(var FileName: AnsiString); virtual;

    procedure WndProc(var Message: TMessage); override;
    procedure CreateParams(var Params: TCreateParams); override;

  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
  end;
  //------------------------------------------------------
var
  ChMidiTestForm: TChMidiTestForm;

implementation

{$R *.DFM}



//***************************************************************************
// 関数実行スレッドクラス
//***************************************************************************
//---------------------------------------------------------------------------
constructor TExecuteThread.Create(AProc: TThreadProc);
begin
  inherited Create(true);
  Proc := AProc;
end;
//---------------------------------------------------------------------------
destructor TExecuteThread.Destroy;
begin
  inherited;
end;
//---------------------------------------------------------------------------
procedure TExecuteThread.Execute;
begin
  while not Terminated do
    Proc();
end;
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChMidiTestForm
// 内容    : TChiemraMidiテスト用プログラム
// Version   : 0.75 (2001/01/12)
//***************************************************************************

//***************************************************************************
// コンストラクタ
//***************************************************************************
constructor TChMidiTestForm.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);

  FillChar(PeekVelocity, 0, SizeOf(PeekVelocity));

  with PlayInfo do
  begin
    MidiFormat     := 0;
    TimeBase       := 0;
    EndDeltaTime   := 0;
    MusicBarChild  := 0;
    MusicBarMother := 0;
    MusicBarDelta  := 0;
  end;

  ChMidiInit(Handle, HInstance);

  PanelBack := TBitmap.Create;
  PanelBack.Width  := StatusPanel.Width;
  PanelBack.Height := StatusPanel.Height;

  StatusPanelUpdateThread := TExecuteThread.Create(StatusPanelUpdate);
  StatusPanelUpdateThread.Priority := tpTimeCritical;
  StatusPanelUpdateThread.Resume;
end;


//***************************************************************************
// デストラクタ
//***************************************************************************
destructor TChMidiTestForm.Destroy;
begin
  // スレッドが終わるのを待つ
  StatusPanelUpdateThread.Terminate;
  StatusPanelUpdateThread.WaitFor;

  PanelBack.Free;
  StatusPanelUpdateThread.Free;

  inherited;
end;


//***************************************************************************
// 作成パラメータ
//***************************************************************************
procedure TChMidiTestForm.CreateParams(var Params: TCreateParams);
begin
  inherited CreateParams(Params);
  //フレームと最小化ボタンを付け加える
  Params.Style := Params.Style or WS_DLGFRAME or WS_MINIMIZEBOX;
end;


//***************************************************************************
// ウィンドウプロシージャ
//***************************************************************************
procedure TChMidiTestForm.WndProc(var Message: TMessage);
var
  WorkScreen: TRect;
  lpRect: PRect;
  MousePosition: TPoint;
begin
  case Message.Msg of
    WM_MOVING:
    begin
      // 位置変更の限定

      lpRect := PRect(Message.LParam);
      SystemParametersInfo(SPI_GETWORKAREA, 0, @WorkScreen, 0);

      if lpRect.Left < WorkScreen.Left + 10 then
      begin
        lpRect.Left := 0;
        lpRect.Right := Width;
      end;
      if lpRect.Top < WorkScreen.Top + 10 then
      begin
        lpRect.Top := 0;
        lpRect.Bottom := Height;
      end;
      if lpRect.Right >= WorkScreen.Right - 10 then
      begin
        lpRect.Left := WorkScreen.Right - Width;
        lpRect.Right := WorkScreen.Right;
      end;
      if lpRect.Bottom >= WorkScreen.Bottom - 10 then
      begin
        lpRect.Top := WorkScreen.Bottom - Height;
        lpRect.Bottom := WorkScreen.Bottom;
      end;
    end;

    WM_NCHITTEST:
    begin
      // フォームの一部をタイトルバーとして扱う
      MousePosition := ScreenToClient(Point(Message.LParamLo, Message.LParamHi));
      if MousePosition.y < Bevel2.Height then
        Message.Result := HTCAPTION
      else
        inherited WndProc(Message);
    end;

    else
    begin
        inherited WndProc(Message);
    end;
  end;
end;


//***************************************************************************
// 演奏ファイル読み込み
// const AnsiString &FileName ファイル名
//***************************************************************************
procedure TChMidiTestForm.LoadPlayMidiData(var FileName: AnsiString);
var
  Data: TChMidiDataInfo;
begin
  // 演奏を終了させDLL内部のSMFデータをすべて消去
  ChMidiStop;
  ChMidiDeleteAllData;

  // SMFデータをPlayingDataという名前で構築
  ChMidiCreateData('PlayingData', PChar(FileName));

  // "PlayingData"という名前のSMFデータの情報を取得
  ChMidiGetDataInfo('PlayingData', @Data);
  with PlayInfo do
  begin
    Creator        := Data.Creator;
    SongName       := Data.SongName;
    MidiFormat     := Data.MidiFormat;
    TimeBase       := Data.TimeBase;
    EndDeltaTime   := Data.EndDeltaTime;
    MusicBarChild  := Data.MusicBarChild;
    MusicBarMother := Data.MusicBarMother;
    MusicBarDelta  := Data.MusicBarDelta;
  end;
end;


//***************************************************************************
// ステータスパネル更新(スレッド使用)
//***************************************************************************
procedure TChMidiTestForm.StatusPanelUpdate;
var
  i: Integer;
  Channels: TChMidiChannel;
begin
  // 現在のMIDIチャンネルの状態を取得
  ChMidiGetChannelData(@Channels);

  // ピークホールド
  for i := 0 to 15 do
  begin
    if Channels.Velocity[i] > PeekVelocity[i] then
      PeekVelocity[i] := Channels.Velocity[i]
    else if PeekVelocity[i] > 2 then
      PeekVelocity[i] := PeekVelocity[i] - 2
    else
      PeekVelocity[i] := 0;
  end;

  PanelBack.Canvas.Lock;
  PanelBack.Canvas.Brush.Color := clWhite;
  PanelBack.Canvas.FillRect(StatusPanel.ClientRect);
  PanelBack.Canvas.Font.Name := 'ＭＳ ゴシック';

  // 画面を描画
  DrawVelocity(2, 2, Channels);
  DrawFileInfo(132, 2);
  DrawPlayingStatus(132, 50);

  // 転送(スレッドを使っているためロック)
  StatusPanel.Canvas.Lock();
  StatusPanel.Canvas.CopyRect(StatusPanel.ClientRect, PanelBack.Canvas, StatusPanel.ClientRect);
  StatusPanel.Canvas.Unlock();
  PanelBack.Canvas.Unlock();

  Sleep(20);
end;



//***************************************************************************
// 曲情報描画
//***************************************************************************
procedure TChMidiTestForm.DrawFileInfo(X, Y: Integer);
begin
  PanelBack.Canvas.Font.Height := 12;
  PanelBack.Canvas.Font.Color  := $00007F00;
  PanelBack.Canvas.Brush.Style := bsSolid;
  PanelBack.Canvas.Brush.Color := $00BFFFBF;

  PanelBack.Canvas.TextRect(
    Rect(X+1,  Y+1, X+39,  Y+15), X+2,  Y+2, 'File名');

  if FileName <> '' then
    PanelBack.Canvas.TextRect(Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2, FileName)
  else
    PanelBack.Canvas.TextRect(Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2, '選択されていません。');
  Inc(Y, 16);
  PanelBack.Canvas.TextRect(Rect(X+1,  Y+1, X+39,  Y+15), X+2,  Y+2, '音楽名');
  PanelBack.Canvas.TextRect(Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2, PlayInfo.SongName);
  Inc(Y, 16);
  PanelBack.Canvas.TextRect(Rect(X+1,  Y+1, X+39,  Y+15), X+2,  Y+2, '著作権');
  PanelBack.Canvas.TextRect(Rect(X+41, Y+1, X+255, Y+15), X+42, Y+2, PlayInfo.Creator);
end;


//***************************************************************************
// 再生情報描画
//***************************************************************************
procedure TChMidiTestForm.DrawPlayingStatus(X, Y: Integer);
var
  Tempo, DeltaTime, BPS, Base: Longword;
begin
  ChMidiGetMusicTempo(@Tempo);
  ChMidiGetDeltaTime(@DeltaTime);
  BPS := 60 * 1000 * 1000 div Tempo;
  Base := PlayInfo.TimeBase;

  if Base = 0 then
    Base := 480;
  if PlayInfo.MusicBarChild = 0 then
    PlayInfo.MusicBarChild := 4;
  if PlayInfo.EndDeltaTime = 0 then
    PlayInfo.EndDeltaTime := 9999 * Base * 4;

  PanelBack.Canvas.Brush.Style := bsSolid;
  PanelBack.Canvas.Brush.Color := $00BFBFFF;
  PanelBack.Canvas.FillRect(Rect(X+1, Y+17, X+1 + 244 * DeltaTime div PlayInfo.EndDeltaTime, Y+23));

  PanelBack.Canvas.Font.Height := 12;
  PanelBack.Canvas.Font.Color  := $007F0000;

  PanelBack.Canvas.Brush.Style := bsSolid;
  PanelBack.Canvas.Brush.Color := $00FFBFBF;

  PanelBack.Canvas.TextRect(Rect(X+1, Y+1, X+39, Y+15), X+2, Y+2, Format('%03dBPS', [BPS]));
  Inc(X, 40);

  PanelBack.Canvas.TextRect(Rect(X+1, Y+1, X+57, Y+15), X+2, Y+2,
    Format('%04d/%04d', [DeltaTime div Base div PlayInfo.MusicBarChild,
        PlayInfo.EndDeltaTime div Base div PlayInfo.MusicBarChild]));
  Inc(X, 58);

  PanelBack.Canvas.TextRect(Rect(X+1, Y+1, X+33, Y+15), X+2, Y+2,
    Format('%02d/%02d', [(DeltaTime div Base) mod PlayInfo.MusicBarChild + 1,
        PlayInfo.MusicBarChild]));
  Inc(X, 34);

  PanelBack.Canvas.TextRect(Rect(X+1, Y+1, X+57, Y+15), X+2, Y+2,
    Format('%04d/%04d', [DeltaTime mod (Base * PlayInfo.MusicBarChild),
        Base * PlayInfo.MusicBarChild]));
  Inc(X, 58);

  PanelBack.Canvas.TextRect(Rect(X+1, Y+1, X+65, Y+15), X+2, Y+2,
    Format('作:chimera', [DeltaTime mod (Base * PlayInfo.MusicBarChild),
      Base * PlayInfo.MusicBarChild]));
end;


//***************************************************************************
// ベロシティ描画
//***************************************************************************
procedure TChMidiTestForm.DrawVelocity(X, Y: Integer; var Channels: TChMidiChannel);
var
  i, j: Integer;
begin
  PanelBack.Canvas.Brush.Color := $00007F00;
  PanelBack.Canvas.Brush.Style := bsSolid;

  for i := 0 to 15 do
  begin
    for j := 0 to (Channels.Velocity[i] + 1) div 8 - 1 do
    begin
      PanelBack.Canvas.FillRect(Rect(
        X + i*8,
        Y + 64 - (j+1)*4 + 1,
        X + (i+1)*8 - 1,
        Y + 64 - j*4));
    end;

    PanelBack.Canvas.FillRect(Rect(
      X + i*8,
      Y + 64 - (PeekVelocity[i] div 8 + 1) * 4 + 1,
      X + (i+1)*8 - 1,
      Y + 64 - PeekVelocity[i] div 8 * 4 ));
  end;

  PanelBack.Canvas.Font.Color   := clGreen;
  PanelBack.Canvas.Font.Height  := 8;
  PanelBack.Canvas.Brush.Style  := bsClear;
  for i := 0 to 15 do
    PanelBack.Canvas.TextOut(
      X + i*8 + 3 - PanelBack.Canvas.TextWidth(IntToStr(i+1)) div 2, Y + 64, IntToStr(i+1));
end;


//***************************************************************************
// ツールボタンイベント
//***************************************************************************
procedure TChMidiTestForm.ToolPlayClick(Sender: TObject);
begin
  if FileName <> '' then
    ChMidiPlay('PlayingData', 1)
  else
  begin
    ToolOpenClick(nil);
    ChMidiPlay('PlayingData', 1);
  end;

end;
//---------------------------------------------------------------------------
procedure TChMidiTestForm.ToolStopClick(Sender: TObject);
begin
  ChMidiStop;
end;
//---------------------------------------------------------------------------
procedure TChMidiTestForm.ToolOpenClick(Sender: TObject);
begin
  if OpenDialog1.Execute then
  begin
    FileName := OpenDialog1.FileName;
    ChMidiStop;
    LoadPlayMidiData(FileName);
  end;
end;
//---------------------------------------------------------------------------
procedure TChMidiTestForm.MnimizeTipClick(Sender: TObject);
begin
  Application.Minimize;
end;
//---------------------------------------------------------------------------
procedure TChMidiTestForm.CloseTipClick(Sender: TObject);
begin
  Close;
end;
//---------------------------------------------------------------------------


end.
