#pragma once
#include <filesystem>

#include "Animation/Skeleton.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UStaticMesh;
class UAnimationAsset;

enum class EExtensionType : uint8
{
    None = 0,
    Obj = 1ULL << 0,
    Fbx = 1ULL << 1,

    All = ~0ULL, 
};

enum class EAssetType : uint8
{
    StaticMesh,
    SkeletalMesh,
    Texture2D,
    Material,
    Animation,
    Skeleton,
};

struct FAssetInfo
{
    FName AssetName;      // Asset의 이름
    FName PackagePath;    // Asset의 패키지 경로
    EAssetType AssetType; // Asset의 타입
    uint32 Size;          // Asset의 크기 (바이트 단위)
};

struct FAssetRegistry
{
    TMap<FName, FAssetInfo> PathNameToAssetInfo;
};

struct FObjLoadResult
{
    UStaticMesh* StaticMesh;
    TArray<UMaterial*> Materials;
};

struct FFbxLoadResult
{
    TArray<USkeleton*> Skeletons;
    TArray<USkeletalMesh*> SkeletalMeshes;
    TArray<UAnimationAsset*> Animations;
    TArray<UMaterial*> Materials;
};

// TODO 파일로 존재하는 UAsset을 관리하는 Manager이지만, 현재 해당 역할을 벗어나 작동한다. 
class UAssetManager : public UObject
{
    DECLARE_CLASS(UAssetManager, UObject)

private:
    std::unique_ptr<FAssetRegistry> AssetRegistry;

public:
    UAssetManager() = default;
    ~UAssetManager();

    static bool IsInitialized();

    /** UAssetManager를 가져옵니다. */
    static UAssetManager& Get();

    /** UAssetManager가 존재하면 가져오고, 없으면 nullptr를 반환합니다. */
    static UAssetManager* GetIfInitialized();
    
    void InitAssetManager();

    const TMap<FName, FAssetInfo>& GetAssetRegistry();

    UMaterial* GetMaterial(const FName& Name);
    //USkeletalMesh* GetSkeleton(const FName& Name);
    USkeletalMesh* GetSkeletalMesh(const FName& Name);
    UAnimationAsset* GetAnimationAsset(const FName& Name);
    UStaticMesh* GetStaticMesh(const FName& Name);

    void AddMaterial(UMaterial* InMaterial);

    // 사용 안하는 거 헷갈릴까 주석처리함. 사용하려면 써도 됨.
    //void AddMaterial(const FName& Key, UMaterial* InMaterial);
    // void AddSkeletalMesh(const FName& Key, USkeletalMesh* Mesh);
    // void AddAnimationAsset(const FName& InKey, UAnimationAsset* InValue);
    // void AddStaticMesh(const FName& InKey, UStaticMesh* InValue);

private:
    void LoadFiles(uint8 ExtensionFlags);
    void LoadFile(std::filesystem::path Entry, uint8 ExtensionFlags = static_cast<uint8>(EExtensionType::All));

    inline static TMap<FName, UMaterial*> MaterialMap;
    inline static TMap<FName, UStaticMesh*> StaticMeshMap;
    //inline static TMap<FName, USkeleton*> SkeletonMap;
    inline static TMap<FName, USkeletalMesh*> SkeletalMeshMap;
    inline static TMap<FName, UAnimationAsset*> AnimationMap;
};
