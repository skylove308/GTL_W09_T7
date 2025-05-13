#include "Skeleton.h"

#include "UObject/Casts.h"

void FAnimationPoseData::Resize(int32 NumBones)
{
    LocalTransforms.SetNum(NumBones);
    GlobalTransforms.SetNum(NumBones);
    SkinningMatrices.SetNum(NumBones);
    BoneTransformDirtyFlags.SetNum(NumBones);
}

void FAnimationPoseData::MarkAllDirty()
{
    for (int32 i = 0; i < BoneTransformDirtyFlags.Num(); ++i)
    {
        BoneTransformDirtyFlags[i] = true;
    }
    if (BoneTransformDirtyFlags.Num() > 0)
    {
        bAnyBoneTransformDirty = true;
    }
}

USkeleton::USkeleton()
{
}

UObject* USkeleton::Duplicate(UObject* InOuter)
{
    ThisClass* newSkeleton = Cast<ThisClass>(Super::Duplicate(InOuter));
    newSkeleton->BoneTree  = BoneTree;
    newSkeleton->BoneParentMap = BoneParentMap;
    newSkeleton->BoneNameToIndex =BoneNameToIndex;
    newSkeleton->ReferenceSkeleton = ReferenceSkeleton;
    newSkeleton->LinkupCache = LinkupCache; 
    newSkeleton->CurrentPose = CurrentPose;
    

    return newSkeleton;
}

void USkeleton::AddBone(const FName Name, int32 ParentIdx, const FMatrix& InGlobalBindPose, const FMatrix& InTransformMatrix) // 파라미터 이름 명시
{
    int32 CurrentBoneIndex = BoneTree.Num(); // 새로 추가될 본의 인덱스
    BoneNameToIndex.Add(Name, CurrentBoneIndex);

    FBoneNode NewBoneNode; // 지역 변수
    NewBoneNode.Name = Name;
    NewBoneNode.ParentIndex = ParentIdx;

    // 1. 로컬 바인드 포즈 계산 -> NewBoneNode.BindTransform 에 저장
    if (ParentIdx != INDEX_NONE)
    {
        // 부모의 글로벌 바인드 포즈는 RefBonePose에 이미 저장되어 있어야 함
        if (ReferenceSkeleton.RefBonePose.IsValidIndex(ParentIdx))
        {
            const FMatrix& ParentGlobalBindPose = ReferenceSkeleton.RefBonePose[ParentIdx];
            FMatrix InverseParentGlobalBindPose = FMatrix::Inverse(ParentGlobalBindPose);

            // 행 우선: ChildLocal = ChildGlobal * ParentGlobalInv
            NewBoneNode.BindTransform = InGlobalBindPose * InverseParentGlobalBindPose;
        }
        else
        {
            NewBoneNode.BindTransform = InGlobalBindPose; //오류 상황
        }
    }
    else
    {
        // 루트 본: 로컬 바인드 포즈 = 글로벌 바인드 포즈
        NewBoneNode.BindTransform = InGlobalBindPose;
    }

    // 2. 글로벌 바인드 포즈의 역행렬 계산 -> NewBoneNode.InverseBindTransform 에 저장
    NewBoneNode.InverseBindTransform = FMatrix::Inverse(InGlobalBindPose);
    NewBoneNode.GeometryOffsetMatrix = InTransformMatrix;
    if (FMath::IsNearlyZero(NewBoneNode.InverseBindTransform.Determinant()))
    {
        NewBoneNode.InverseBindTransform = FMatrix::Identity; // 방어 코드
    }

    // 3. 완성된 NewBoneNode를 BoneTree에 추가
    BoneTree.Add(NewBoneNode);

    // 참조 스켈레톤 업데이트
    // ReferenceSkeleton.BoneInfo는 FBoneNode의 배열이므로, NewBoneNode의 복사본을 추가합니다.
    // 이 FBoneNode에는 로컬 BindTransform과 InverseGlobalBindTransform이 모두 포함됩니다.
    ReferenceSkeleton.BoneInfo.Add(NewBoneNode);

    ReferenceSkeleton.NameToIndexMap.Add(Name, ReferenceSkeleton.BoneInfo.Num() - 1);

    // RefBonePose에는 여전히 원본 글로벌 바인드 포즈를 저장 (GetGlobalBindTransform에서 직접 사용 위함)
    ReferenceSkeleton.RefBonePose.Add(InGlobalBindPose);

    // 현재 포즈 크기 조정
    CurrentPose.Resize(BoneTree.Num());
    bProcessingOrderCacheDirty = true; // 본 구조 변경 시 캐시 더티 처리
    CurrentPose.MarkAllDirty();      // 새로 본이 추가/변경되었으므로 모든 포즈를 다시 계산해야 함
}
void USkeleton::AddBone(const FName Name, const FName ParentName, const FMatrix BindTransform, const FMatrix& InTransformMatrix)
{
    int32 ParentIdx = INDEX_NONE;

    // 부모 이름이 유효하면 해당 인덱스 찾기
    if (!ParentName.IsNone())
    {
        const uint32* ParentIdxPtr = BoneNameToIndex.Find(ParentName);
        if (ParentIdxPtr)
        {
            ParentIdx = static_cast<int32>(*ParentIdxPtr);
        }
    }

    // 부모 관계 맵에 저장
    BoneParentMap.Add(Name, ParentName);

    // 기존 AddBone 호출
    AddBone(Name, ParentIdx, BindTransform, InTransformMatrix);
}

void USkeleton::MarkBoneAndChildrenDirty(int32 BoneIndex)
{
    if (!BoneTree.IsValidIndex(BoneIndex) || !CurrentPose.BoneTransformDirtyFlags.IsValidIndex(BoneIndex))
    {
        return;
    }

    TArray<int32> Queue;
    Queue.Add(BoneIndex);
    CurrentPose.bAnyBoneTransformDirty = true;

    int32 Head = 0;
    while (Head < Queue.Num())
    {
        int32 CurrentDirtyBoneIndex = Queue[Head++];
        if (CurrentPose.BoneTransformDirtyFlags.IsValidIndex(CurrentDirtyBoneIndex) &&
            !CurrentPose.BoneTransformDirtyFlags[CurrentDirtyBoneIndex]) // 아직 dirty가 아니면
        {
            CurrentPose.BoneTransformDirtyFlags[CurrentDirtyBoneIndex] = true;
            if (BoneTree.IsValidIndex(CurrentDirtyBoneIndex)) { // BoneTree 접근 전 유효성 검사
                for (int32 ChildIdx : BoneTree[CurrentDirtyBoneIndex].ChildBoneIndices)
                {
                    Queue.Add(ChildIdx); // 자식들도 큐에 추가하여 dirty 처리
                }
            }
        }
    }
}

void USkeleton::FinalizeBoneHierarchy()
{
    // 각 본의 ChildBoneIndices 채우기
    for (int32 i = 0; i < BoneTree.Num(); ++i)
    {
        BoneTree[i].ChildBoneIndices.Empty(); // 기존 자식 목록 비우기
    }
    for (int32 i = 0; i < BoneTree.Num(); ++i)
    {
        int32 ParentIdx = BoneTree[i].ParentIndex;
        if (ParentIdx != INDEX_NONE && BoneTree.IsValidIndex(ParentIdx))
        {
            BoneTree[ParentIdx].ChildBoneIndices.Add(i);
        }
    }
    CalculateAndCacheProcessingOrder_Internal(); // 처리 순서 캐시 업데이트
    CurrentPose.MarkAllDirty();                  // 전체 업데이트 필요 표시
}

uint32 USkeleton::GetBoneIndex(const FName Name) const
{
    const uint32* it = BoneNameToIndex.Find(Name);
    return (it != nullptr) ? *it : INDEX_NONE;
}

FMatrix USkeleton::GetGlobalBindTransform(int32 BoneIdx) const
{
    if (ReferenceSkeleton.RefBonePose.IsValidIndex(BoneIdx))
    {
        return ReferenceSkeleton.RefBonePose[BoneIdx];
    }

    return CalculateGlobalBindTransform(BoneIdx);
}

FMatrix USkeleton::CalculateGlobalBindTransform(int32 BoneIdx) const
{
    FMatrix Global = BoneTree[BoneIdx].BindTransform;
    int32 Parent = BoneTree[BoneIdx].ParentIndex;
    while (Parent >= 0)
    {
        // 행 우선(vM): ChildGlobal = ChildLocal * ParentGlobal
        // 이 루프는 자식에서 부모로 올라가면서 로컬 변환들을 누적 곱함:
        // Global_new = Global_previously_calculated_for_child * Local_parent
        // 예: Bone2_Global = L2
        //      Bone1_Global_temp = L2 * L1  (Bone1이 Bone2의 부모)
        //      Bone0_Global_temp = (L2*L1) * L0 (Bone0이 Bone1의 부모)
        // 결과적으로 G = L_child * L_parent1 * L_parent2 * ... * L_root 형태가 됨.
        // 이는 올바른 글로벌 변환 계산 방식임 (루트부터 순차적으로 적용되는 것과 동일).
        Global = BoneTree[Parent].BindTransform * Global;
        Parent = BoneTree[Parent].ParentIndex;
    }
    return Global;
}

FMatrix USkeleton::GetInverseBindTransform(int32 BoneIdx) const
{
    if (BoneTree.IsValidIndex(BoneIdx))
    {
        return BoneTree[BoneIdx].InverseBindTransform; // 저장된 값 사용
    }
    return FMatrix::Identity;
}

FMatrix USkeleton::GetGeometryOffsetTransform(int32 BoneIdx) const
{
    if (BoneTree.IsValidIndex(BoneIdx))
    {
        return BoneTree[BoneIdx].GeometryOffsetMatrix; // 저장된 값 사용
    }
    return FMatrix::Identity;
}

FMatrix USkeleton::CalculateSkinningMatrix(int32 BoneIdx, const FMatrix& AnimationMatrix) const
{
    // 언리얼 좌표계에 맞는 원래 순서로 복원
    // 지오메트리 오프셋 → 인버스 바인드 포즈 → 애니메이션 행렬
    return GetGeometryOffsetTransform(BoneIdx) * GetInverseBindTransform(BoneIdx) * AnimationMatrix;
    // return GetInverseBindTransform(BoneIdx) * AnimationMatrix;
}

void USkeleton::UpdateCurrentPose(const TArray<FMatrix>& LocalAnimationTransforms)
{
    // 현재 사용안함
    // 로컬 변환 저장 
    for (int32 i = 0; i < FMath::Min(LocalAnimationTransforms.Num(), CurrentPose.LocalTransforms.Num()); ++i)
    {
        CurrentPose.LocalTransforms[i] = LocalAnimationTransforms[i];
    }

    // 글로벌 변환 계산
    for (int32 i = 0; i < CurrentPose.LocalTransforms.Num(); ++i)
    {
        if (BoneTree[i].ParentIndex == INDEX_NONE)
        {
            CurrentPose.GlobalTransforms[i] = CurrentPose.LocalTransforms[i];
        }
        else
        {
            CurrentPose.GlobalTransforms[i] = CurrentPose.LocalTransforms[i] * CurrentPose.GlobalTransforms[BoneTree[i].ParentIndex];
        }

        // 스키닝 행렬 계산
        CurrentPose.SkinningMatrices[i] = CalculateSkinningMatrix(i, CurrentPose.GlobalTransforms[i]);
    }
}

TArray<int32> USkeleton::GetProcessingOrder()
{
    return CachedProcessingOrder;
}

void USkeleton::CalculateAndCacheProcessingOrder_Internal() const
{
    CachedProcessingOrder.Empty();
    
    if (BoneTree.IsEmpty())
    {
        bProcessingOrderCacheDirty = false;
        return;
    }

    CachedProcessingOrder.Reserve(BoneTree.Num());
    // CachedProcessingOrder.Reserve(BoneTree.Num()); // 선택 사항

    TArray<uint8> Processed; // 로컬 변수 사용
    Processed.Init(false, BoneTree.Num());
    TArray<int32> Queue; // 로컬 변수 사용
    Queue.Reserve(BoneTree.Num());

    // 1. 루트 본들을 큐에 추가
    for (int32 i = 0; i < BoneTree.Num(); ++i)
    {
        if (BoneTree[i].ParentIndex == INDEX_NONE)
        {
            Queue.Add(i);
            // 루트 본도 초기에 Processed로 마킹 가능 (큐에 중복 추가 방지)
            // Processed[i] = true; // 만약 큐에 넣을 때 바로 마킹한다면, 아래 while 루프 시작 시 체크 불필요
        }
    }

    int32 Head = 0;
    while (Head < Queue.Num())
    {
        int32 CurrentIndex = Queue[Head++];

        // 큐에서 꺼낼 때 이미 처리되었는지 다시 한번 확인 (사이클이나 중복 삽입 방지)
        if (Processed[CurrentIndex])
        {
            continue;
        }

        CachedProcessingOrder.Add(CurrentIndex);
        Processed[CurrentIndex] = true;

        // 2. ChildBoneIndices를 사용하여 자식들을 큐에 추가 (훨씬 효율적)
        if (BoneTree.IsValidIndex(CurrentIndex)) // CurrentIndex 유효성 검사
        {
            const FBoneNode& CurrentNodeData = BoneTree[CurrentIndex];
            for (int32 ChildIdx : CurrentNodeData.ChildBoneIndices)
            {
                // 자식 인덱스가 유효하고 아직 처리되지 않았다면 큐에 추가
                if (BoneTree.IsValidIndex(ChildIdx) && !Processed[ChildIdx])
                {
                    Queue.Add(ChildIdx);
                    // 여기서 Processed[ChildIdx] = true; 로 미리 마킹하면
                    // while 루프 시작 시의 if (Processed[CurrentIndex]) continue; 에서 걸러짐.
                }
            }
        }
    }

    // 3. 오류 처리: 모든 본이 처리되었는지 확인
    if (CachedProcessingOrder.Num() != BoneTree.Num())
    {
        for (int32 i = 0; i < BoneTree.Num(); ++i)
        {
            if (!Processed[i])
            {
                CachedProcessingOrder.Add(i); // 순서는 보장되지 않지만, 모든 본 포함
            }
        }
    }
    bProcessingOrderCacheDirty = false;
}

void USkeleton::EnsureProcessingOrderCache() const
{
    if (bProcessingOrderCacheDirty || CachedProcessingOrder.IsEmpty())
    {
        CalculateAndCacheProcessingOrder_Internal();
    }
}
