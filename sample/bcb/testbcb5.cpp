//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
USEFORM("chmiditest.cpp", ChMidiTestForm);
USELIB("chmidi.lib");
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
        Application->CreateForm(__classid(TChMidiTestForm), &ChMidiTestForm);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------
