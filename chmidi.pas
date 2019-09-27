unit ChMidi;

interface

uses
  Windows, Messages, MMSystem;

const
  // 演奏ウィンドウメッセージ
  WM_CHMIDISTART        = WM_USER + 30;
  WM_CHMIDIEND          = WM_USER + 31;

  // リセット用定数
  mrGM              = 0;
  mrGS              = 1;

  // リバーブ用定数
  gsrRoom1          = 0;
  gsrRoom2          = 1;
  gsrRoom3          = 2;
  gsrHall1          = 3;
  gsrHall2          = 4;
  gsrPlate          = 5;
  gsrDelay          = 6;
  gsrPanningDelay   = 7;

  // コーラス用定数
  gscChorus1        = 0;
  gscChorus2        = 1;
  gscChorus3        = 2;
  gscChorus4        = 3;
  gscFeedBackChorus = 4;
  gscFlanger        = 5;
  gscShortDelay     = 6;
  gscShortDelayFB   = 7;

  // ディレイ用定数
  gsdDelay1         = 0;
  gsdDelay2         = 1;
  gsdDelay3         = 2;
  gsdDelay4         = 3;
  gsdPanDelay1      = 4;
  gsdPanDelay2      = 5;
  gsdPanDelay3      = 6;
  gsdPanDelay4      = 7;
  gsdDelayToReverb  = 8;
  gsdPanRepeat      = 9;

  // コントロールチェンジ
  ccModulation      = $01;
  ccPortamentoTime  = $02;
  ccVolume          = $07;
  ccPanpot          = $0A;
  ccExpression      = $0B;
  ccHold1           = $40;
  ccPortamento      = $41;
  ccSostenuto       = $42;
  ccSoft            = $43;
  ccPortamentoControl = $54;
  ccEffect1         = $5B;
  ccEffect3         = $5D;
  ccEffect4         = $5E;

type
  TChMidiDataInfo = Record
    MidiFormat      : Longword;                 // フォーマットの種類
    TimeBase        : Longword;                 // MIDIファイルのベース
    EndDeltaTime    : Longword;                 // 最終デルタタイム
    MusicBarChild   : Longword;                 // 拍子（分子）
    MusicBarMother  : Longword;                 // 拍子（分母）
    MusicBarDelta   : Longword;                 // 拍子MIDIクロック
    Creator         : array[0..255] of Char;
    SongName        : array[0..255] of Char;
  end;
  PChMidiDataInfo = ^TChMidiDataInfo;

  TChMidiChannel  = Record
    Velocity        : array[0..15] of Byte;     // ベロシティー
    Programs        : array[0..15] of Byte;     // プログラム
    Volume          : array[0..15] of Byte;     // ボリューム
    Pan             : array[0..15] of Byte;     // パン
    Expression      : array[0..15] of Byte;     // エクスプレッション
    Reverb          : array[0..15] of Byte;     // リバーブ
    Chorus          : array[0..15] of Byte;     // コーラス
    Delay           : array[0..15] of Byte;     // ディレイ
  end;
  PChMidiChannel  = ^TChMidiChannel;

const
  CHMIDIDLL = 'chmidi.dll';

  //------------------------------------------------------
  function ChMidiGetLastError(Buffer: PChar; BufferSize: Integer): Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------
  function ChMidiInit(hWnd: HWND; hInst: Longint): Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------
  function ChMidiPlay(DataName: PChar; RepeatFlag: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiStop: Integer; stdcall; external CHMIDIDLL;
  function ChMidiFadeOut(Time: Integer): Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------
  function ChMidiSetReport(Value: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiGetDeltaTime(DeltaTime: PInteger): Integer; stdcall; external CHMIDIDLL;
  function ChMidiGetMusicTempo(MusicTempo: PInteger): Integer; stdcall; external CHMIDIDLL;
  function ChMidiGetChannelData(Channels: PChMidiChannel): Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------
  function ChMidiCreateData(DataName, FileName: PChar): Integer; stdcall; external CHMIDIDLL;
  function ChMidiGetDataInfo(DataName: PChar; Info: PChMidiDataInfo): Integer; stdcall; external CHMIDIDLL;
  function ChMidiDeleteData(DataName: PChar): Integer; stdcall; external CHMIDIDLL;
  function ChMidiDeleteAllData: Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------
  function ChMidiNoteOn(Channel, Note, Velocity, Time: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiNoteOff(Channel, Note: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiControlChange(Channel, Number, Value: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiProgramChange(Channel, Number: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiChannelPressure(Channel, Pressure: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiPitchBend(Channel, PitchBend: Integer): Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------
  function ChMidiMidiReset(Reset: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiSetGSReverb(Reverb: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiSetGSChorus(Chorus: Integer): Integer; stdcall; external CHMIDIDLL;
  function ChMidiSetGSDelay(Delay: Integer): Integer; stdcall; external CHMIDIDLL;
  //------------------------------------------------------

implementation

end.
