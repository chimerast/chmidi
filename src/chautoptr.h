//---------------------------------------------------------------------------
#ifndef chautoptrH
#define chautoptrH
//---------------------------------------------------------------------------
#include <mem.h>
#include "chexception.h"
//---------------------------------------------------------------------------
#pragma warn -inl
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//***************************************************************************
// クラス名  : TChimeraAutoPtr
// Version   : 1.0 (2000/8/8)
//***************************************************************************
namespace ChimeraSystem
{
    class TReferenceCountHash;
    class TChimeraAutoPtrBase;
    template <class T> class TChimeraAutoPtr;

    // TReferenceCountHash =========================
    class TReferenceCountHash
    {
        static const int HashTableSize = 0x2000;

        struct THashNode
        {
            void *Pointer;
            int Count;
            THashNode *Next;
            THashNode *Previous;
            THashNode(void *APointer, THashNode *ANext)
                : Pointer(APointer), Count(1), Previous(NULL), Next(ANext)
                { if(ANext) ANext->Previous = this; }
        };

    private:
        THashNode **HashTable; // ハッシュテーブル

        int HashFunction(void *APointer)
        {
            return ((int)APointer >> 4) & (HashTableSize - 1);
        }

        THashNode* FindNode(void *APointer, int ANumber)
        {
            THashNode *Temp = HashTable[ANumber];
            while(Temp != NULL)
            {
                if(Temp->Pointer == APointer) return Temp;
                Temp = Temp->Next;
            }
            return NULL;
        }

    public:
        template <class U>
        void IncrementReference(U *APointer)
        {
            int Number;
            THashNode *Temp;

            if(APointer != NULL)
            {
                Number = HashFunction(APointer);
                Temp = FindNode(APointer, Number);

                if(Temp != NULL)
                {
                    Temp->Count++;
                }
                else
                {
                    THashNode *OldFirst = HashTable[Number];
                    HashTable[Number] = new THashNode(APointer, OldFirst);
                }
            }
        }

        template <class U>
        void DecrementReference(U *APointer)
        {
            int Number;
            THashNode *Temp;

            if(APointer != NULL)
            {
                Number = HashFunction(APointer);
                Temp = FindNode(APointer, Number);

                if(Temp != NULL)
                {
                    if(--(Temp->Count) == 0)
                    {
                        if(Temp->Next) Temp->Next->Previous = Temp->Previous;

                        if(Temp->Previous) Temp->Previous->Next = Temp->Next;
                        else HashTable[Number] = Temp->Next;

                        delete Temp;
                        delete APointer;
                    }
                }
                else
                {
                    throw EChAutoPtr(
                        "リストされていないポインタが検出されました。");
                }
            }
        }

        TReferenceCountHash(void)
        {
            HashTable = new THashNode*[HashTableSize];
            std::memset(HashTable, 0, HashTableSize * sizeof(THashNode*));
        }

        ~TReferenceCountHash(void)
        {
            delete[] HashTable;
        }
    };


    // TChimeraAutoPtrBase =========================
    class TChimeraAutoPtrBase
    {
        template <class T>
        friend class TChimeraAutoPtr;
    private:
        static TReferenceCountHash ReferenceCounter;
        TChimeraAutoPtrBase(void) { }
    };


    // TChimeraAutoPtr =============================
    template <class T>
    class TChimeraAutoPtr : public TChimeraAutoPtrBase
    {
    private:
        T *ObjectPointer;

        void IncrementReference(void)
        {
            ReferenceCounter.IncrementReference(ObjectPointer);
        }

        void DecrementReference(void)
        {
            ReferenceCounter.DecrementReference(ObjectPointer);
            ObjectPointer = NULL;
        }

    public:
        T* Get(void) const { return ObjectPointer; }
        T& operator* (void) const { return *ObjectPointer; }
        T* operator-> (void) const { return ObjectPointer; }

        TChimeraAutoPtr& operator=(const TChimeraAutoPtr &Rhs)
        {
            if(this == &Rhs) return *this;
            DecrementReference();
            ObjectPointer = Rhs.ObjectPointer;
            IncrementReference();
            return *this;
        }

        template <class U>
        TChimeraAutoPtr& operator=(const TChimeraAutoPtr<U>& Rhs)
        {
            if(ObjectPointer == Rhs.Get()) return *this;
            DecrementReference();
            ObjectPointer = Rhs.Get();
            IncrementReference();
            return *this;
        }

        void Reset(T *Rhs = NULL)
        {
            if(ObjectPointer == Rhs) return;
            DecrementReference();
            ObjectPointer = Rhs;
            IncrementReference();
        }

        void Release(void)
            { DecrementReference(); }

        explicit TChimeraAutoPtr(T *APointer=NULL)
            : ObjectPointer(APointer)
            { IncrementReference(); }

        TChimeraAutoPtr(const TChimeraAutoPtr &Rhs)
            : ObjectPointer(Rhs.ObjectPointer)
            { IncrementReference(); }

        template <class U>
        TChimeraAutoPtr(const TChimeraAutoPtr<U> &Rhs)
            : ObjectPointer(Rhs.Get())
            { IncrementReference(); }

        ~TChimeraAutoPtr(void)
            { DecrementReference(); }
    };
};
//---------------------------------------------------------------------------
#pragma warn +inl
//---------------------------------------------------------------------------
using namespace ChimeraSystem;
//---------------------------------------------------------------------------
#endif
