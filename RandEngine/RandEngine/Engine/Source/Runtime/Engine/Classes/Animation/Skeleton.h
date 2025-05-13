#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

struct FBoneNode
{
    FName Name;
    int32 ParentIndex;
    FMatrix BindTransform; // Bind pose transform (로컬 바인드 포즈)
    FMatrix InverseBindTransform; // Bind pose transform (로컬 바인드 포즈)
    FMatrix GeometryOffsetMatrix;
    TArray<int32> ChildBoneIndices;
    // 리타겟팅 모드 - Animation 주차에 필요 시 사용
    // TEnumAsByte<EBoneTranslationRetargetingMode::Type> TranslationRetargetingMode;

    FBoneNode()
        : ParentIndex(INDEX_NONE)
        , BindTransform(FMatrix::Identity)
        , InverseBindTransform(FMatrix::Identity)
        , GeometryOffsetMatrix(FMatrix::Identity)
        // , TranslationRetargetingMode(EBoneTranslationRetargetingMode::Animation)
    {
    }
};

struct FReferenceSkeleton
{
    TArray<FBoneNode> BoneInfo;

    // 각 본의 글로벌 바인드 포즈
    TArray<FMatrix> RefBonePose;

    TMap<FName, int32> NameToIndexMap;
};

struct FSkeletonToMeshLinkup
{
    // Skeleton Bone Index → Mesh Bone Index
    TArray<int32> SkeletonToMeshTable;

    // Mesh Bone Index → Skeleton Bone Index
    TArray<int32> MeshToSkeletonTable;
};

// Pose data of current animation
struct FAnimationPoseData
{
    // 각 본의 로컬 변환 행렬 (애니메이션 적용 후)
    TArray<FMatrix> LocalTransforms;

    // 각 본의 글로벌 변환 행렬 (애니메이션 적용 후)
    TArray<FMatrix> GlobalTransforms;

    // 스키닝 행렬 (애니메이션 행렬 * 인버스 바인드 포즈 행렬)
    TArray<FMatrix> SkinningMatrices;

    TArray<uint8> BoneTransformDirtyFlags;

    bool bAnyBoneTransformDirty;

    void Resize(int32 NumBones);
    void MarkAllDirty();
};

class USkeleton : public UObject
{
    DECLARE_CLASS(USkeleton, UObject)
public:
    TArray<FBoneNode> BoneTree;

    TMap<FName, FName> BoneParentMap;

    // 본 이름에서 인덱스로의 맵
    TMap<FName, uint32> BoneNameToIndex;

    // 참조 스켈레톤
    FReferenceSkeleton ReferenceSkeleton;

    // Mesh - Skeleton index cache 나중에 필요 시 사용
    TArray<FSkeletonToMeshLinkup> LinkupCache;

    // 현재 애니메이션 포즈 데이터 - Unreal기준 USkeleton에 없으나 임시 사용
    // 본 변형으로 자세가 변경된 경우 여기서 참조
    FAnimationPoseData CurrentPose;

public:
    USkeleton();
    ~USkeleton() = default;
    void Clear()
    {
        BoneTree.Empty();
        LinkupCache.Empty();
        BoneParentMap.Empty();
        BoneNameToIndex.Empty();
    }
    UObject* Duplicate(UObject* InOuter) override;
    // 본 추가 (인덱스 기반)
    void AddBone(const FName Name, int32 ParentIdx, const FMatrix& InGlobalBindPose, const FMatrix& InTransformMatrix);

    // 본 추가 (이름 기반)
    void AddBone(const FName Name, const FName ParentName, const FMatrix BindTransform, const FMatrix& InTransformMatrix);

    // Dirty Flag 관련 함수
    void MarkBoneAndChildrenDirty(int32 BoneIndex); // 특정 본과 그 자식들을 Dirty로 표시
    void FinalizeBoneHierarchy(); // AddBone이 모두 끝난 후 호출하여 ChildBoneIndices 채우기 등

    // 본 이름으로 인덱스 가져오기
    uint32 GetBoneIndex(const FName Name) const;

    // 글로벌 바인드 트랜스폼 계산
    FMatrix GetGlobalBindTransform(int32 BoneIdx) const;

    // 글로벌 바인드 트랜스폼 계산 (내부 함수)
    FMatrix CalculateGlobalBindTransform(int32 BoneIdx) const;

    // 인버스 바인드 포즈 행렬 가져오기
    FMatrix GetInverseBindTransform(int32 BoneIdx) const;

    FMatrix GetGeometryOffsetTransform(int32 BoneIdx) const;

    // 스키닝 행렬 계산 (애니메이션 행렬 * 인버스 바인드 포즈 행렬)
    FMatrix CalculateSkinningMatrix(int32 BoneIdx, const FMatrix& AnimationMatrix) const;

    // 현재 애니메이션 포즈 업데이트
    void UpdateCurrentPose(const TArray<FMatrix>& LocalAnimationTransforms);

    TArray<int32> GetProcessingOrder();
    // 메시-스켈레톤 링크업 데이터 찾기 또는 추가 
    /*const FSkeletonToMeshLinkup& FindOrAddMeshLinkupData(const UObject* InMesh);*/

    // 메시 본 인덱스 → 스켈레톤 본 인덱스 변환
    /*int32 GetSkeletonBoneIndexFromMeshBoneIndex(const UObject* InMesh, int32 MeshBoneIndex);*/

    // 스켈레톤 본 인덱스 → 메시 본 인덱스 변환
    /*int32 GetMeshBoneIndexFromSkeletonBoneIndex(const UObject* InMesh, int32 SkeletonBoneIndex);*/
    mutable TArray<int32> CachedProcessingOrder;
private:
    mutable bool bProcessingOrderCacheDirty;
private:
    void CalculateAndCacheProcessingOrder_Internal() const; // private 멤버 함수로 선언
    void EnsureProcessingOrderCache() const;
};
