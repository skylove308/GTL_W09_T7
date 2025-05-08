#include "Class.h"
#include <cassert>

#include "EngineStatics.h"
#include "UObjectArray.h"
#include "Serialization/Archive.h"


UClass::UClass(
    const char* InClassName,
    uint32 InClassSize,
    uint32 InAlignment,
    UClass* InSuperClass,
    ClassConstructorType InCTOR
)
    : ClassCTOR(InCTOR)
    , ClassSize(InClassSize)
    , ClassAlignment(InAlignment)
    , SuperClass(InSuperClass)
{
    NamePrivate = InClassName;
}

bool UClass::IsChildOf(const UClass* SomeBase) const
{
    assert(this);
    if (!SomeBase) return false;

    // Super의 Super를 반복하면서 
    for (const UClass* TempClass = this; TempClass; TempClass=TempClass->GetSuperClass())
    {
        if (TempClass == SomeBase)
        {
            return true;
        }
    }
    return false;
}

UObject* UClass::GetDefaultObject() const
{
    if (!ClassDefaultObject)
    {
        const_cast<UClass*>(this)->CreateDefaultObject();
    }
    return ClassDefaultObject;
}

void UClass::RegisterProperty(const FProperty& Prop)
{
    Properties.Add(Prop);
}

void UClass::SerializeBin(FArchive& Ar, void* Data)
{
    // 상속받은 클래스의 프로퍼티들도 직렬화
    if (SuperClass)
    {
        SuperClass->SerializeBin(Ar, Data);
    }

    // 이 클래스의 프로퍼티들 직렬화
    for (const FProperty& Prop : Properties)
    {
        void* PropData = static_cast<uint8*>(Data) + Prop.Offset;
        Ar.Serialize(PropData, Prop.Size);
    }
}

UObject* UClass::CreateDefaultObject()
{
    if (!ClassDefaultObject)
    {
        ClassDefaultObject = ClassCTOR();
        if (!ClassDefaultObject)
        {
            return nullptr;
        }

        ClassDefaultObject->ClassPrivate = this;
        ClassDefaultObject->NamePrivate = GetName() + "_CDO";
        ClassDefaultObject->UUID = UEngineStatics::GenUUID();
        GUObjectArray.AddObject(ClassDefaultObject);
    }
    return ClassDefaultObject;
}
