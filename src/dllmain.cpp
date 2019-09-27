//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#define DLL_BUILD
#include "chmidi.h"
#include "chmidisys.h"
//---------------------------------------------------------------------------
TChimeraMidi *ChMidi = NULL;
char LastError[256];
//---------------------------------------------------------------------------
#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            if(!ChMidi) ChMidi = new TChimeraMidi();
            break;

        case DLL_PROCESS_DETACH:
            delete ChMidi;
            break;
    }
    return 1;
}
//---------------------------------------------------------------------------
#define CheckError(p) \
    try { p; LastError[0] = '\0'; return 1; } \
    catch(Exception &e) { strcpy(LastError, e.Message.c_str()); return 0; } \
    catch(...) { strcpy(LastError, "Unknown Error."); return 0; } \
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiGetLastError(char *Buffer, int BufferSize)
    { strncpy(Buffer, LastError, BufferSize); return 1; }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiInit(HWND hWnd, HINSTANCE hInst)
    { CheckError(ChMidi->Init(hWnd, hInst)); }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiPlay(const char *DataName, int Repeat)
    { CheckError(ChMidi->Play(DataName, Repeat)); }
DLLEXPORT int __stdcall ChMidiStop(void)
    { CheckError(ChMidi->Stop()); }
DLLEXPORT int __stdcall ChMidiFadeOut(int Time)
    { CheckError(ChMidi->SetFadeOut(Time)); }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiSetReport(int Value)
    { CheckError(ChMidi->SetReport(Value)); }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiCreateData(const char *DataName, const char *FileName)
    { CheckError(ChMidi->CreateData(DataName, FileName)); }
DLLEXPORT int __stdcall ChMidiDeleteData(const char *DataName)
    { CheckError(ChMidi->DeleteData(DataName)); }
DLLEXPORT int __stdcall ChMidiDeleteAllData(void)
    { CheckError(ChMidi->DeleteAllData()); }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiNoteOn(int Channel, int Note, int Velocity, int Time)
    { CheckError(ChMidi->NoteOn(Channel, Note, Velocity, Time)); }
DLLEXPORT int __stdcall ChMidiNoteOff(int Channel, int Note)
    { CheckError(ChMidi->NoteOff(Channel, Note)); }
DLLEXPORT int __stdcall ChMidiControlChange(int Channel, int Number, int Value)
    { CheckError(ChMidi->ControlChange(Channel, Number, Value)); }
DLLEXPORT int __stdcall ChMidiProgramChange(int Channel, int Number)
    { CheckError(ChMidi->ProgramChange(Channel, Number)); }
DLLEXPORT int __stdcall ChMidiChannelPressure(int Channel, int Pressure)
    { CheckError(ChMidi->ChannelPressure(Channel, Pressure)); }
DLLEXPORT int __stdcall ChMidiPitchBend(int Channel, int PitchBend)
    { CheckError(ChMidi->PitchBend(Channel, PitchBend)); }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiMidiReset(int Reset)
    { CheckError(ChMidi->MidiReset((TMidiReset)Reset)); }
DLLEXPORT int __stdcall ChMidiSetGSReverb(int Reverb)
    { CheckError(ChMidi->SetGSReverb((TGSReverb)Reverb)); }
DLLEXPORT int __stdcall ChMidiSetGSChorus(int Chorus)
    { CheckError(ChMidi->SetGSChorus((TGSChorus)Chorus)); }
DLLEXPORT int __stdcall ChMidiSetGSDelay(int Delay)
    { CheckError(ChMidi->SetGSDelay((TGSDelay)Delay)); }
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiGetDataInfo(const char *DataName, TChMidiDataInfo *Info)
{
    try {
        const TChimeraMidiDataInfo &DataInfo = ChMidi->GetSMFData(DataName);
        Info->MidiFormat     = DataInfo.MidiFormat;
        Info->TimeBase       = DataInfo.TimeBase;
        Info->EndDeltaTime   = DataInfo.EndDeltaTime;
        Info->MusicBarChild  = DataInfo.MusicBarChild;
        Info->MusicBarMother = DataInfo.MusicBarMother;
        Info->MusicBarDelta  = DataInfo.MusicBarDelta;
        strncpy(Info->Creator, DataInfo.Creator->Count
            ? DataInfo.Creator->Strings[0].c_str() : "", 256);
        strncpy(Info->SongName, DataInfo.SongName->Count
            ? DataInfo.SongName->Strings[0].c_str() : "", 256);
        LastError[0] = '\0';
        return 1;
    } catch(Exception &e) {
        strcpy(LastError, e.Message.c_str());
        return 0;
    } catch(...) {
        strcpy(LastError, "Unknown Error.");
        return 0;
    }
}
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiGetDeltaTime(int *DeltaTime)
{
    try {
        *DeltaTime = ChMidi->GetDeltaTime();
        return 1;
    } catch(Exception &e) {
        strcpy(LastError, e.Message.c_str());
        return 0;
    } catch(...) {
        strcpy(LastError, "Unknown Error.");
        return 0;
    }
}
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiGetMusicTempo(int *MusicTempo)
{
    try {
        *MusicTempo = ChMidi->GetMusicTempo();
        return 1;
    } catch(Exception &e) {
        strcpy(LastError, e.Message.c_str());
        return 0;
    } catch(...) {
        strcpy(LastError, "Unknown Error.");
        return 0;
    }
}
//---------------------------------------------------------------------------
DLLEXPORT int __stdcall ChMidiGetChannelData(TChMidiChannel *Channels)
{
    try {
        TChimeraMidiChannel ChMidiChannel[16];
        ChMidi->GetChannelData(ChMidiChannel);
        for(int i = 0; i < 16; ++i)
        {
            Channels->Velocity[i]   = ChMidiChannel[i].Velocity;
            Channels->Program[i]    = ChMidiChannel[i].Program;
            Channels->Volume[i]     = ChMidiChannel[i].Volume;
            Channels->Pan[i]        = ChMidiChannel[i].Pan;
            Channels->Expression[i] = ChMidiChannel[i].Expression;
            Channels->Reverb[i]     = ChMidiChannel[i].Reverb;
            Channels->Chorus[i]     = ChMidiChannel[i].Chorus;
            Channels->Delay[i]      = ChMidiChannel[i].Delay;
        }
        return 1;
    } catch(Exception &e) {
        strcpy(LastError, e.Message.c_str());
        return 0;
    } catch(...) {
        strcpy(LastError, "Unknown Error.");
        return 0;
    }
}
