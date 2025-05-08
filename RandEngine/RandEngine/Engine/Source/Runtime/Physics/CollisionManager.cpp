#include "CollisionManager.h"

#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

#include "Engine/OverlapResult.h"
#include "Math/Quat.h"
#include "UObject/Casts.h"
#include "UObject/UObjectIterator.h"

/**
 * @brief 점 Point와 선분 SegmentStart-SegmentEnd 사이의 가장 가까운 점을 찾습니다.
 * (참고: UKismetMathLibrary::FindClosestPointOnSegment 를 사용할 수도 있습니다)
 * @param Point 대상 점 (월드 좌표계)
 * @param SegmentStart 선분 시작점 (월드 좌표계)
 * @param SegmentEnd 선분 끝점 (월드 좌표계)
 * @return 선분 위의 가장 가까운 점 (월드 좌표계)
 */
FVector ClosestPointOnLineSegment(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd)
{
    // 선분의 길이가 0인 경우 시작점 반환
    if (SegmentStart == SegmentEnd)
    {
        return SegmentStart;
    }

    FVector SegmentDir = SegmentEnd - SegmentStart;
    double SegmentLengthSq = SegmentDir.SquaredLength(); // double 사용 권장 (정밀도)

    // 수치 안정성을 위해 매우 작은 길이는 0으로 처리
    if (SegmentLengthSq < KINDA_SMALL_NUMBER)
    {
        return SegmentStart;
    }

    // 점을 선분이 정의하는 무한선에 투영합니다.
    // t = Dot(Point - Start, Dir) / Dot(Dir, Dir)
    double t = FVector::DotProduct(Point - SegmentStart, SegmentDir) / SegmentLengthSq;

    // t 값을 [0, 1] 범위로 클램핑하여 선분 위에 있도록 합니다.
    t = FMath::Clamp(t, 0.0, 1.0);

    // 선분 위의 가장 가까운 점 계산
    return SegmentStart + SegmentDir * static_cast<float>(t);
}

// 두 선분 (A-B, C-D) 사이의 최단 거리 제곱을 반환하는 함수 (구현 복잡)
float SquaredDistBetweenLineSegments(const FVector& A, const FVector& B, const FVector& C, const FVector& D)
{
    FVector u = B - A;
    FVector v = D - C;
    FVector w = A - C;
    
    float a = FVector::DotProduct(u, u);
    float b = FVector::DotProduct(u, v);
    float c = FVector::DotProduct(v, v);
    float d = FVector::DotProduct(u, w);
    float e = FVector::DotProduct(v, w);
    float F = a*c - b*b;
    
    float sc, tc;
    
    // 두 선이 거의 평행한 경우
    if (F < FLT_EPSILON)
    {
        sc = 0.0f;
        tc = (b > c ? d/b : e/c);
    }
    else
    {
        sc = (b*e - c*d) / F;
        tc = (a*e - b*d) / F;
    }
    
    // 파라미터 값을 [0,1] 범위로 클램핑
    sc = FMath::Clamp(sc, 0.0f, 1.0f);
    tc = FMath::Clamp(tc, 0.0f, 1.0f);
    
    // 각 선분에서 가장 가까운 점 계산
    FVector P = A + u * sc;
    FVector Q = C + v * tc;
    
    // 두 점 사이의 거리 제곱 반환
    return (P - Q).SquaredLength();
}

// 점 P와 OBB 사이의 가장 가까운 점을 찾는 함수
FVector ClosestPointOnOBB(const FVector& P, const UBoxComponent* Box)
{
    FMatrix WorldToBox = FMatrix::Inverse(Box->GetWorldMatrix()); // 가상 함수: 월드->로컬 변환 행렬
    FVector P_Local = WorldToBox.TransformPosition(P); // 점을 박스 로컬 공간으로 변환

    // 로컬 공간에서 각 축으로 클램핑
    FVector BoxExtent = Box->GetBoxExtent();
    FVector ClosestP_Local;
    ClosestP_Local.X = std::max(-BoxExtent.X, std::min(P_Local.X, BoxExtent.X));
    ClosestP_Local.Y = std::max(-BoxExtent.Y, std::min(P_Local.Y, BoxExtent.Y));
    ClosestP_Local.Z = std::max(-BoxExtent.Z, std::min(P_Local.Z, BoxExtent.Z));

    FMatrix BoxToWorld = Box->GetWorldMatrix(); // 가상 함수: 로컬->월드 변환 행렬
    
    return BoxToWorld.TransformPosition(ClosestP_Local); // 월드 좌표로 다시 변환
}

// SAT 검사를 위한 헬퍼 함수: OBB를 특정 축에 투영하여 최소/최대값 반환
void ProjectOBB(const UBoxComponent* Box, const FVector& Axis, float& OutMin, float& OutMax)
{
    FVector Axes[3] = { Box->GetForwardVector(), Box->GetRightVector(), Box->GetUpVector() };
    FVector HalfSize = Box->GetBoxExtent();

    // 박스 중심을 축에 투영
    float CenterProj = FVector::DotProduct(Box->GetWorldLocation(), Axis);

    // 박스 각 축 방향의 반경을 현재 축에 투영한 값의 절대값을 모두 더함
    float RadiusProj = FMath::Abs(FVector::DotProduct(Axes[0] * HalfSize.X, Axis)) +
                       FMath::Abs(FVector::DotProduct(Axes[1] * HalfSize.Y, Axis)) +
                       FMath::Abs(FVector::DotProduct(Axes[2] * HalfSize.Z, Axis));

    OutMin = CenterProj - RadiusProj;
    OutMax = CenterProj + RadiusProj;
}

// 두 구간 [minA, maxA], [minB, maxB]가 겹치는지 검사
bool OverlapOnAxis(float MinA, float MaxA, float MinB, float MaxB)
{
    return MaxA >= MinB && MinA <= MaxB;
}

/**
 * @brief 점 P와 원점에 중심을 둔 AABB 사이의 가장 가까운 점을 찾습니다.
 * @param P 대상 점 (로컬 좌표계)
 * @param Extent AABB의 절반 크기 (Half-dimensions)
 * @return AABB 위의 가장 가까운 점 (로컬 좌표계)
 */
inline FVector ClosestPointOnAABB(const FVector& P, const FVector& Extent)
{
    return FVector(
        FMath::Clamp(P.X, -Extent.X, Extent.X),
        FMath::Clamp(P.Y, -Extent.Y, Extent.Y),
        FMath::Clamp(P.Z, -Extent.Z, Extent.Z)
    );
}

/**
 * @brief 월드 좌표계의 점 P와 UBoxComponent로 표현된 OBB 사이의 가장 가까운 점을 찾습니다.
 * @param P 대상 점 (월드 좌표계)
 * @param BoxComponent OBB를 나타내는 박스 컴포넌트
 * @return OBB 위의 가장 가까운 점 (월드 좌표계)
 */
FVector ClosestPointOnOBB_FromComponent(const FVector& P, const UBoxComponent* BoxComponent)
{
    if (!BoxComponent)
    {
        // BoxComponent가 유효하지 않으면 입력 점 P를 그대로 반환하거나, 적절한 오류 처리
        return P;
    }

    // 박스의 월드 트랜스폼 가져오기
    const FMatrix BoxWorldMatrix = BoxComponent->GetWorldMatrix();
    const FMatrix BoxWorldMatrixInv = FMatrix::Inverse(BoxWorldMatrix);

    // 점 P를 월드 공간에서 박스의 로컬 공간으로 변환
    const FVector P_Local = BoxWorldMatrixInv.TransformPosition(P);

    // 박스의 스케일을 포함한 Extent 가져오기 (로컬 공간 기준)
    const FVector BoxExtent = BoxComponent->GetBoxExtent();

    // 로컬 공간에서 가장 가까운 점 찾기 (원점에 중심을 둔 AABB에 대한 연산)
    const FVector ClosestP_Local = ClosestPointOnAABB(P_Local, BoxExtent);

    // 로컬 공간의 가장 가까운 점을 다시 월드 공간으로 변환하여 반환
    return BoxWorldMatrix.TransformPosition(ClosestP_Local);
}

/**
 * @brief 선분(A-B)과 UBoxComponent로 표현된 OBB 사이의 최단 거리 제곱을 계산합니다.
 * 이 함수는 'Alternating Projections'와 유사한 반복적 접근 방식을 사용합니다.
 * 두 볼록 집합 사이의 가장 가까운 점을 찾는 데 사용될 수 있습니다.
 *
 * @param A 선분 시작점 (월드 좌표계)
 * @param B 선분 끝점 (월드 좌표계)
 * @param BoxComponent OBB를 나타내는 박스 컴포넌트
 * @param MaxIterations 최대 반복 횟수
 * @param ToleranceSq 수렴으로 간주할 거리 제곱 허용 오차
 * @return 선분과 OBB 사이의 최단 거리 제곱. 교차하거나 매우 가까우면 0에 가까운 값 반환.
 */
float SquaredDistSegmentOBB(const FVector& A, const FVector& B, const UBoxComponent* BoxComponent, int32 MaxIterations = 10, double ToleranceSq = 1e-6)
{
    // 선분 위의 초기 추정점 (예: 선분 중점 또는 시작점 A)
    FVector PointOnSegment = A; // 또는 (A + B) * 0.5;
    FVector PointOnBox;

    double LastDistSq = DBL_MAX;
    
    for (int32 i = 0; i < MaxIterations; ++i)
    {
        // 1. 현재 선분 위의 점에서 OBB 위의 가장 가까운 점 찾기
        PointOnBox = ClosestPointOnOBB_FromComponent(PointOnSegment, BoxComponent);

        // 2. 현재 OBB 위의 점에서 선분 위의 가장 가까운 점 찾기
        FVector NewPointOnSegment = ClosestPointOnLineSegment(PointOnBox, A, B);

        // 3. 수렴 확인: 이전 단계의 점과 새 점 사이의 거리 제곱 확인
        double CurrentDistSq = (PointOnBox - NewPointOnSegment).SquaredLength();
        double ImprovementSq = FMath::Abs(CurrentDistSq - LastDistSq);

        // 점이 거의 움직이지 않거나 거리가 더 이상 줄어들지 않으면 수렴으로 간주
        if ((PointOnSegment - NewPointOnSegment).SquaredLength() < ToleranceSq || ImprovementSq < ToleranceSq * 0.1 ) // 개선 정도도 확인
        {
            PointOnSegment = NewPointOnSegment; // 마지막 위치 업데이트
            LastDistSq = CurrentDistSq;
            break;
        }

        // 다음 반복을 위해 선분 위의 점 업데이트
        PointOnSegment = NewPointOnSegment;
        LastDistSq = CurrentDistSq; // 현재 거리 저장
    }

    // 최종적으로 찾은 가장 가까운 두 점 사이의 거리 제곱 반환
    // 마지막 반복 후 PointOnBox는 PointOnSegment에 대한 최단점이므로 다시 계산할 필요 없음
    return static_cast<float>(LastDistSq);
}

FCollisionManager::FCollisionManager()
{
    for (size_t i = 0; i <= NUM_TYPES; ++i)
    {
        for (size_t j = 0; j <= NUM_TYPES; ++j)
        {
            CollisionMatrix[i][j] = &FCollisionManager::Check_NotImplemented;
        }
    }

    CollisionMatrix[static_cast<size_t>(EShapeType::Box)][static_cast<size_t>(EShapeType::Box)] = &FCollisionManager::Check_Box_Box;
    CollisionMatrix[static_cast<size_t>(EShapeType::Box)][static_cast<size_t>(EShapeType::Sphere)] = &FCollisionManager::Check_Box_Sphere;
    CollisionMatrix[static_cast<size_t>(EShapeType::Box)][static_cast<size_t>(EShapeType::Capsule)] = &FCollisionManager::Check_Box_Capsule;
    CollisionMatrix[static_cast<size_t>(EShapeType::Sphere)][static_cast<size_t>(EShapeType::Box)] = &FCollisionManager::Check_Sphere_Box;
    CollisionMatrix[static_cast<size_t>(EShapeType::Sphere)][static_cast<size_t>(EShapeType::Sphere)] = &FCollisionManager::Check_Sphere_Sphere;
    CollisionMatrix[static_cast<size_t>(EShapeType::Sphere)][static_cast<size_t>(EShapeType::Capsule)] = &FCollisionManager::Check_Sphere_Capsule;
    CollisionMatrix[static_cast<size_t>(EShapeType::Capsule)][static_cast<size_t>(EShapeType::Box)] = &FCollisionManager::Check_Capsule_Box;
    CollisionMatrix[static_cast<size_t>(EShapeType::Capsule)][static_cast<size_t>(EShapeType::Sphere)] = &FCollisionManager::Check_Capsule_Sphere;
    CollisionMatrix[static_cast<size_t>(EShapeType::Capsule)][static_cast<size_t>(EShapeType::Capsule)] = &FCollisionManager::Check_Capsule_Capsule;
}

void FCollisionManager::CheckOverlap(const UWorld* World, const UPrimitiveComponent* Component, TArray<FOverlapResult>& OutOverlaps) const
{
    OutOverlaps.Empty();
    
    if (!Component || !Component->IsA<UShapeComponent>())
    {
        return;
    }

    const bool bComponentHasValidBox = Component->AABB.IsValidBox();
    
    for (const auto Iter : TObjectRange<UShapeComponent>())
    {
        if (!Iter || Iter->GetWorld() != World || Iter == Component)
        {
            continue;            
        }

        bool bCanSkip = true;
        
        if (Iter->AABB.IsValidBox() && bComponentHasValidBox)
        {
            if (FBoundingBox::CheckOverlap(Component->AABB, Iter->AABB))
            {
                bCanSkip = false;
            }
        }
        else
        {
            bCanSkip = false;
        }

        if (!bCanSkip)
        {
            FOverlapResult OverlapResult;
            if (IsOverlapped(Component, Iter, OverlapResult))
            {
                OutOverlaps.Add(OverlapResult);
            }
        }
    }
}

bool FCollisionManager::IsOverlapped(const UPrimitiveComponent* Component, const UPrimitiveComponent* OtherComponent, FOverlapResult& OutResult) const
{
    if (!Component || !OtherComponent)
    {
        return false;
    }
    if (!Component->IsA<UShapeComponent>() || !OtherComponent->IsA<UShapeComponent>())
    {
        return false;
    }

    const UShapeComponent* ShapeA = Cast<UShapeComponent>(Component);
    const UShapeComponent* ShapeB = Cast<UShapeComponent>(OtherComponent);

    const SIZE_T ShapeTypeA = static_cast<SIZE_T>(ShapeA->GetShapeType());
    const SIZE_T ShapeTypeB = static_cast<SIZE_T>(ShapeB->GetShapeType());

    const bool bOverlapped = (this->*CollisionMatrix[ShapeTypeA][ShapeTypeB])(ShapeA, ShapeB, OutResult);
    if (bOverlapped)
    {
        OutResult.Actor = OtherComponent->GetOwner();
        OutResult.Component = const_cast<UPrimitiveComponent*>(OtherComponent);
        OutResult.bBlockingHit = false;
        
        return true;
    }

    return false;
}

bool FCollisionManager::Check_NotImplemented(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    return false;
}

bool FCollisionManager::Check_Box_Box(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    UBoxComponent* BoxA = Cast<UBoxComponent>(A);
    UBoxComponent* BoxB = Cast<UBoxComponent>(B);
    
    FVector AxesA[3] = { BoxA->GetForwardVector(), BoxA->GetRightVector(), BoxA->GetUpVector() };
    FVector AxesB[3] = { BoxB->GetForwardVector(), BoxB->GetRightVector(), BoxB->GetUpVector() };
    FVector TestAxes[15]; // 검사할 축 (최대 15개)

    int32 AxisIndex = 0;
    
    // 1. Box A의 3개 축
    for (int32 i = 0; i < 3; ++i)
    {
        TestAxes[AxisIndex++] = AxesA[i];
    }
    
    // 2. Box B의 3개 축
    for (int32 i = 0; i < 3; ++i)
    {
        TestAxes[AxisIndex++] = AxesB[i];
    }
    
    // 3. Box A의 축과 Box B의 축 간의 외적 (Cross Product) 9개
    for (int32 i = 0; i < 3; ++i)
    {
        for (int32 j = 0; j < 3; ++j)
        {
            FVector Cross = FVector::CrossProduct(AxesA[i], AxesB[j]);
            // 외적이 0 벡터에 가까우면 (축이 평행하면) 검사할 필요 없음
            if (Cross.SquaredLength() > 1e-6f)
            { // 작은 값 (epsilon)으로 비교
                TestAxes[AxisIndex++] = Cross; //.Normalize(); // 정규화는 이론적으로 필요하나, OverlapOnAxis 결과에 영향 안 줌
            }
        }
    }

    // 모든 축에 대해 검사
    for (int32 i = 0; i < AxisIndex; ++i)
    {
        float minA, maxA, minB, maxB;
        ProjectOBB(BoxA, TestAxes[i], minA, maxA);
        ProjectOBB(BoxB, TestAxes[i], minB, maxB);

        if (!OverlapOnAxis(minA, maxA, minB, maxB))
        {
            return false; // 분리 축 발견! 충돌하지 않음.
        }
    }

    // 모든 축에서 겹쳤다면 충돌한 것임.
    return true;
}

bool FCollisionManager::Check_Box_Sphere(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    UBoxComponent* Box = Cast<UBoxComponent>(A);
    USphereComponent* Sphere = Cast<USphereComponent>(B);
    
    // 스피어 중심 P에서 OBB 위의 가장 가까운 점 Q 찾기
    FVector ClosestPoint = ClosestPointOnOBB(Sphere->GetWorldLocation(), Box);

    // P와 Q 사이의 거리 제곱 계산
    FVector Diff = Sphere->GetWorldLocation() - ClosestPoint;
    float DistSq = Diff.SquaredLength();

    // 거리 제곱이 스피어 반지름 제곱보다 작거나 같으면 충돌
    float RadiusSq = Sphere->GetRadius() * Sphere->GetRadius();
    return DistSq <= RadiusSq;
}

bool FCollisionManager::Check_Box_Capsule(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    UBoxComponent* Box = Cast<UBoxComponent>(A);
    UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(B);
    
    FVector StartCap, EndCap;
    Capsule->GetEndPoints(StartCap, EndCap);

    // 캡슐 선분과 OBB 사이의 최단 거리 제곱 계산
    float DistSq = SquaredDistSegmentOBB(StartCap, EndCap, Box);

    // 캡슐 반지름 제곱 계산
    float RadiusSq = Capsule->GetRadius() * Capsule->GetRadius();

    // 최단 거리 제곱이 캡슐 반지름 제곱보다 작거나 같으면 충돌
    return DistSq <= RadiusSq;
}

bool FCollisionManager::Check_Sphere_Box(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    return Check_Box_Sphere(B, A, OutResult);
}

bool FCollisionManager::Check_Sphere_Sphere(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    USphereComponent* SphereA = Cast<USphereComponent>(A);
    USphereComponent* SphereB = Cast<USphereComponent>(B);

    float RadiusSum = SphereA->GetRadius() + SphereB->GetRadius();

    FVector Diff = SphereA->GetWorldLocation() - SphereB->GetWorldLocation();
    float DistSq = Diff.SquaredLength();
    
    return DistSq <= (RadiusSum * RadiusSum);
}

bool FCollisionManager::Check_Sphere_Capsule(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    USphereComponent* Sphere = Cast<USphereComponent>(A);
    UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(B);

    FVector StartCap, EndCap;
    Capsule->GetEndPoints(StartCap, EndCap);

    // 스피어 중심(Sp.Location)에서 캡슐 선분(StartCap-EndCap)까지 가장 가까운 점 찾기
    FVector ClosestPointOnSegment = ClosestPointOnLineSegment(Sphere->GetWorldLocation(), StartCap, EndCap);

    // 스피어 중심과 가장 가까운 점 사이의 거리 제곱 계산
    FVector Diff = Sphere->GetWorldLocation() - ClosestPointOnSegment;
    float DistSq = Diff.SquaredLength();

    // 캡슐 반지름과 스피어 반지름의 합 계산
    float TotalRadius = Capsule->GetRadius() + Sphere->GetRadius();
    float TotalRadiusSq = TotalRadius * TotalRadius;

    // 거리 제곱이 반지름 합 제곱보다 작거나 같으면 충돌
    return DistSq <= TotalRadiusSq;
}

bool FCollisionManager::Check_Capsule_Box(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    return Check_Box_Capsule(B, A, OutResult);   
}

bool FCollisionManager::Check_Capsule_Sphere(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    return Check_Sphere_Capsule(B, A, OutResult);  
}

bool FCollisionManager::Check_Capsule_Capsule(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const
{
    UCapsuleComponent* CapsuleA = Cast<UCapsuleComponent>(A);
    UCapsuleComponent* CapsuleB = Cast<UCapsuleComponent>(B);
    
    FVector StartA, EndA, StartB, EndB;
    CapsuleA->GetEndPoints(StartA, EndA);
    CapsuleB->GetEndPoints(StartB, EndB);

    // 두 선분 (StartA-EndA, StartB-EndB) 사이의 최단 거리 제곱 계산
    // 주의: SquaredDistBetweenLineSegments 함수는 직접 구현해야 합니다.
    float DistSq = SquaredDistBetweenLineSegments(StartA, EndA, StartB, EndB);

    // 두 반지름의 합 계산
    float TotalRadius = CapsuleA->GetRadius() + CapsuleB->GetRadius();
    float TotalRadiusSq = TotalRadius * TotalRadius;

    // 최단 거리 제곱이 반지름 합 제곱보다 작거나 같으면 충돌
    return DistSq <= TotalRadiusSq;
}
