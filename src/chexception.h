//---------------------------------------------------------------------------
#ifndef chexceptionH
#define chexceptionH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
//---------------------------------------------------------------------------
namespace ChimeraSystem
{
    class EChAutoPtr : public Exception
    {
    public:
        inline __fastcall EChAutoPtr(const AnsiString Msg)
            : Exception("TChimeraAutoPtr エラー\n\n" + Msg) {}
        inline __fastcall virtual ~EChAutoPtr(void) {}
    };


    // TChimeraMidi例外 ============================
    class EChMidi : public Exception
    {
    public:
        inline __fastcall EChMidi(const AnsiString Msg)
            : Exception("TChimeraMidi エラー\n\n" + Msg) {}
        inline __fastcall virtual ~EChMidi(void) {}
    };
};
//---------------------------------------------------------------------------
using namespace ChimeraSystem;
//---------------------------------------------------------------------------
#endif
