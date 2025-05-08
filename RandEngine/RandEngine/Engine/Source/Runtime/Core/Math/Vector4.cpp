#include "Vector4.h"

#include "Misc/Parse.h"

FString FVector4::ToString() const
{
    // FString::Printf를 사용하여 포맷팅된 문자열 생성
    // TEXT() 매크로는 리터럴 문자열을 TCHAR 타입으로 만들어줍니다.
    return FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f W=%3.3f"), X, Y, Z, W);

    // 필요에 따라 소수점 정밀도 지정 가능: 예) "X=%.2f Y=%.2f Z=%.2f"
    // return FString::Printf(TEXT("X=%.2f Y=%.2f Z=%.2f"), x, y, z);
}

bool FVector4::InitFromString(const FString& InSourceString)
{
    X = Y = Z = 0;
    W = 1.0f;

    // The initialization is only successful if the X, Y, and Z values can all be parsed from the string
    const bool bSuccessful = FParse::Value(*InSourceString, TEXT("X=") , X) &&
            FParse::Value(*InSourceString, TEXT("Y="), Y)   &&
            FParse::Value(*InSourceString, TEXT("Z="), Z);

    // W is optional, so don't factor in its presence (or lack thereof) in determining initialization success
    FParse::Value(*InSourceString, TEXT("W="), W);

    return bSuccessful;

}
