#include "Color.h"

#include "Misc/Parse.h"

const FLinearColor FLinearColor::White(1.f, 1.f, 1.f);
const FLinearColor FLinearColor::Gray(0.5f, 0.5f, 0.5f);
const FLinearColor FLinearColor::Black(0, 0, 0);
const FLinearColor FLinearColor::Transparent(0, 0, 0, 0);
const FLinearColor FLinearColor::Red(1.f, 0, 0);
const FLinearColor FLinearColor::Green(0, 1.f, 0);
const FLinearColor FLinearColor::Blue(0, 0, 1.f);
const FLinearColor FLinearColor::Yellow(1.f, 1.f, 0);

const FColor FColor::White(255, 255, 255);
const FColor FColor::Black(0, 0, 0);
const FColor FColor::Transparent(0, 0, 0, 0);
const FColor FColor::Red(255, 0, 0);
const FColor FColor::Green(0, 255, 0);
const FColor FColor::Blue(0, 0, 255);
const FColor FColor::Yellow(255, 255, 0);
const FColor FColor::Cyan(0, 255, 255);
const FColor FColor::Magenta(255, 0, 255);
const FColor FColor::Orange(243, 156, 18);
const FColor FColor::Purple(169, 7, 228);
const FColor FColor::Turquoise(26, 188, 156);
const FColor FColor::Silver(189, 195, 199);
const FColor FColor::Emerald(46, 204, 113);

float FLinearColor::LinearToSRGB(float InC)
{
    if (InC <= 0.0031308f)
    {
        return InC * 12.92f;
    }
    else
    {
        return 1.055f * FMath::Pow(InC, 0.41666667f) - 0.055f;
    }
}

FColor FLinearColor::ToColorSRGB() const
{
    return FColor(
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(LinearToSRGB(R) * 255.0f + 0.5f), 0, 255)),
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(LinearToSRGB(G) * 255.0f + 0.5f), 0, 255)),
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(LinearToSRGB(B) * 255.0f + 0.5f), 0, 255)),
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(A * 255.0f + 0.5f), 0, 255))
        );
}

FColor FLinearColor::ToColorRawRGB8() const
{
    return FColor(
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(R * 255.0f + 0.5f), 0, 255)),
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(G * 255.0f + 0.5f), 0, 255)),
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(B * 255.0f + 0.5f), 0, 255)),
    static_cast<uint8>(FMath::Clamp(static_cast<int32>(A * 255.0f + 0.5f), 0, 255))
    );
}

FString FLinearColor::ToString() const
{
    return FString::Printf(TEXT("R=%3.3f G=%3.3f B=%3.3f A=%3.3f"), R, G, B, A);
}

bool FLinearColor::InitFromString(const FString& InSourceString)
{
    // The initialization is only successful if the X, Y, and Z values can all be parsed from the string
    const bool bSuccessful = FParse::Value(*InSourceString, TEXT("R=") , R) && FParse::Value(*InSourceString, TEXT("G="), G) &&
        FParse::Value(*InSourceString, TEXT("B="), B) && FParse::Value(*InSourceString, TEXT("A="), A);

    return bSuccessful;

}
