program testdelphi4;

uses
  Forms,
  chmiditest in 'chmiditest.pas' {ChMidiTestForm},
  ChMidi in 'chmidi.pas';

{$R *.RES}

begin
  Application.Initialize;
  Application.CreateForm(TChMidiTestForm, ChMidiTestForm);
  Application.Run;
end.
