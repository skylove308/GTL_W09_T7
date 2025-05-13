#pragma once
#include <fstream>

#include "Container/String.h"

struct Serializer
{
    /* Write FString */
    static void WriteFString(std::ofstream& Stream, const FString& InString)
    {
        uint32 Length = InString.Len();
        Stream.write(reinterpret_cast<const char*>(&Length), sizeof(Length));
        Stream.write(GetData(InString), Length * sizeof(char));
    }

    /* Read FString */
    static void ReadFString(std::ifstream& Stream, FString& InString)
    {
        uint32 Length = 0;
        Stream.read(reinterpret_cast<char*>(&Length), sizeof(Length));
        char* Buffer = new char[Length + 1];
        Stream.read(Buffer, Length);
        Buffer[Length] = '\0';
        InString = Buffer;
        delete[] Buffer;
    }

    /* Write FWString */
    static void WriteFWString(std::ofstream& Stream, const FWString& InString)
    {
        uint32 Length = static_cast<uint32>(InString.length());
        Stream.write(reinterpret_cast<const char*>(&Length), sizeof(Length));
        Stream.write(reinterpret_cast<const char*>(InString.c_str()), Length * sizeof(wchar_t));
    }

    /* Read FWString */
    static void ReadFWString(std::ifstream& Stream, FWString& InString)
    {
        uint32 Length = 0;
        Stream.read(reinterpret_cast<char*>(&Length), sizeof(Length));
        wchar_t* Buffer = new wchar_t[Length + 1];
        Stream.read(reinterpret_cast<char*>(Buffer), Length * sizeof(wchar_t));
        Buffer[Length] = L'\0';
        InString = Buffer;
        delete[] Buffer;
    }


    /* Write TArray */
    template<typename T>
    static void WriteArray(std::ofstream& Stream, const TArray<T>& InArray)
    {
        uint32 BoneInfoCount = InArray.Num();                                                                     
        Stream.write(reinterpret_cast<const char*>(&BoneInfoCount), sizeof(BoneInfoCount));                                                    
        Stream.write(reinterpret_cast<const char*>(InArray.GetData()), BoneInfoCount * sizeof(T));
    }

    /* Read TArray */
    template<typename T>
    static void ReadArray(std::ifstream& Stream, TArray<T>& OutArray)
    {
        uint32 Length = 0;
        Stream.read(reinterpret_cast<char*>(&Length), sizeof(Length));
        OutArray.SetNum(Length);
        Stream.read(reinterpret_cast<char*>(OutArray.GetData()), Length * sizeof(T));
    }


    // /* Write TArray */
    // template<typename Key, typename Value>
    // static void WriteMap(std::ofstream& Stream, const TMap<Key, Value>& InArray)
    // {
    //     uint32 BoneInfoCount = InArray.Num();                                                                     
    //     Stream.write(reinterpret_cast<const char*>(&BoneInfoCount), sizeof(BoneInfoCount));                                                    
    //     Stream.write(reinterpret_cast<const char*>(InArray.GetData()), BoneInfoCount * sizeof(T));
    // }
    //
    // /* Read TArray */
    // template<typename Key, typename Value>
    // static void ReadMap(std::ifstream& Stream, const TMap<Key, Value>& OutArray)
    // {
    //     uint32 Length = 0;
    //     Stream.read(reinterpret_cast<char*>(&Length), sizeof(Length));
    //     OutArray.SetNum(Length);
    //     Stream.read(reinterpret_cast<char*>(OutArray.GetData()), Length * sizeof(T));
    // }
};
