#pragma once
#include "CoreMiscDefines.h"
#include "Container/String.h"
#include "HAL/PlatformType.h"

class UObject;
class FName;


// TODO: 나중에 Version 관련 메서드 추가
class FArchive
{
protected:
    FArchive() = default;

public:
    virtual ~FArchive() = default;

    /**
     * FName을 직렬화 합니다.
     *
     * 이 함수는 Subclass에서 구현됩니다.
     *
     * @param Value Serialize할 FName
     * @return 현재 FArchive의 인스턴스
     */
    virtual FArchive& operator<<(FName& Value)
    {
        return *this;
    }

    /**
     * UObject를 직렬화 합니다.
     *
     * 이 함수는 Subclass에서 구현됩니다.
     *
     * @param Value Serialize할 Object
     * @return 현재 FArchive의 인스턴스
     */
    virtual FArchive& operator<<(UObject*& Value)
    {
        return *this;
    }

    template <typename T>
        requires std::is_arithmetic_v<T>
    FORCEINLINE friend FArchive& operator<<(FArchive& Ar, T& Value)
    {
        Ar.Serialize(Value);
        return Ar;
    }

    FORCEINLINE friend FArchive& operator<<(FArchive& Ar, bool& Value)
    {
        uint8 BoolValue = Value ? 1 : 0;
        Ar.Serialize(&BoolValue, sizeof(uint8));

        if (Ar.IsLoading())
        {
            Value = BoolValue != 0;
        }

        return Ar;
    }

    FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FString& Value)
    {
        int32 Length = Value.Len();
        Ar << Length; // 문자열 길이 직렬화

        if (Ar.IsLoading())
        {
            Value.Resize(Length);
        }
        Ar.Serialize(GetData(Value), Length * sizeof(TCHAR));

        return Ar;
    }

    virtual void Serialize(void* V, int64 Length)
    {
        if (IsLoading())
        {
            // 데이터 로드 로직
            LoadData(V, Length);
        }
        else
        {
            // 데이터 저장 로직
            SaveData(V, Length);
        }
    }

    template <typename T>
    void Serialize(T& Value)
    {
        Serialize((void*)&Value, sizeof(T));
    }

    virtual void Seek(int64 InPos) {}
    virtual int64 Tell() { return INDEX_NONE; }


    virtual void SaveData(const void* Data, uint64 Length) {}
    virtual void LoadData(void* Data, uint64 Length) {}

    bool IsLoading() const { return bIsLoading; }
    bool IsSaving() const { return bIsSaving; }

protected:
    bool bIsLoading = false;
    bool bIsSaving = false;
};
