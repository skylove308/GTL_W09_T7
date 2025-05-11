#pragma once
#include <filesystem>

#include "Animation/Skeleton.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

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
    Animation
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

struct FFbxLoadResult
{
    //TArray<USkeleton*> Skeletons;
    TArray<USkeletalMesh*> SkeletalMeshes;
    //TArray<UStaticMesh*> StaticMeshes;
    //TArray<UMaterial*> Materials;
    TArray<UAnimationAsset*> Animations;
};


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

    USkeletalMesh* GetSkeletalMesh(const FName& Name);
    UAnimationAsset* GetAnimationAsset(const FName& Name);

    void AddSkeletalMesh(const FName& Key, USkeletalMesh* Mesh);
    void AddAnimationAsset(const FName& InKey, UAnimationAsset* InValue);

private:
    void LoadFiles(uint8 ExtensionFlags);
    void LoadFile(std::filesystem::path Entry, uint8 ExtensionFlags);

    inline static TMap<FName, USkeletalMesh*> SkeletalMeshMap;
    //inline static TMap<FName, UMaterial*> MaterialMap;
    inline static TMap<FName, UAnimationAsset*> AnimationMap;
};
