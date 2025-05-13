#include "AssetManager.h"
#include "Engine.h"

#include <filesystem>

#include "FbxLoader.h"
#include "Engine/FObjLoader.h"
#include "Animation/AnimationAsset.h"

UAssetManager::~UAssetManager()
{
    for (auto& [Name, Object] : AnimationMap)
    {
        if (Object)
        {
            delete Object;
            Object = nullptr;
        }
    }

    AnimationMap.Empty();

    for (auto& [Name, Object] : SkeletalMeshMap)
    {
        if (Object)
        {
            delete Object;
            Object = nullptr;
        }
    }
    SkeletalMeshMap.Empty();

    for (auto& [Name, Object] : MaterialMap)
    {
        if (Object)
        {
            delete Object;
            Object = nullptr;
        }
    }
    MaterialMap.Empty();
}

bool UAssetManager::IsInitialized()
{
    return GEngine && GEngine->AssetManager;
}

UAssetManager& UAssetManager::Get()
{
    if (UAssetManager* Singleton = GEngine->AssetManager)
    {
        return *Singleton;
    }
    else
    {
        UE_LOG(ELogLevel::Error, "Cannot use AssetManager if no AssetManagerClassName is defined!");
        assert(0);
        return *new UAssetManager; // never calls this
    }
}

UAssetManager* UAssetManager::GetIfInitialized()
{
    return GEngine ? GEngine->AssetManager : nullptr;
}

void UAssetManager::InitAssetManager()
{
    AssetRegistry = std::make_unique<FAssetRegistry>();

    LoadFiles(~static_cast<uint8>(EExtensionType::Fbx));
}

const TMap<FName, FAssetInfo>& UAssetManager::GetAssetRegistry()
{
    return AssetRegistry->PathNameToAssetInfo;
}

UMaterial* UAssetManager::GetMaterial(const FName& Name)
{
    std::filesystem::path path = std::filesystem::path(GetData(Name.ToString()));
    FName NameWithoutExt = FName(std::filesystem::path(path).replace_extension().c_str());
    
    if (MaterialMap.Contains(NameWithoutExt))
    {
        return MaterialMap[NameWithoutExt];
    }
    if (MaterialMap.Contains(path.c_str()))
    {
        return MaterialMap[path.c_str()];
    }
    
    return nullptr;
}

// USkeletalMesh* UAssetManager::GetSkeleton(const FName& Name)
// {
//     
// }

USkeletalMesh* UAssetManager::GetSkeletalMesh(const FName& Name)
{
    std::filesystem::path path = std::filesystem::path(GetData(Name.ToString()));
    FName NameWithoutExt = FName(std::filesystem::path(path).replace_extension().c_str());
    
    if (SkeletalMeshMap.Contains(NameWithoutExt))
    {
        return SkeletalMeshMap[NameWithoutExt];
    }
    if (SkeletalMeshMap.Contains(path.c_str()))
    {
        return SkeletalMeshMap[path.c_str()];
    }
    LoadFile(path, static_cast<uint8>(EExtensionType::Fbx));

    if (SkeletalMeshMap.Contains(NameWithoutExt))
    {
        return SkeletalMeshMap[NameWithoutExt];
    }
    if (SkeletalMeshMap.Contains(path.c_str()))
    {
        return SkeletalMeshMap[path.c_str()];
    }
    return nullptr;
}

UAnimationAsset* UAssetManager::GetAnimationAsset(const FName& Name)
{
    std::filesystem::path path = std::filesystem::path(GetData(Name.ToString()));
    FName NameWithoutExt = FName(std::filesystem::path(path).replace_extension().c_str());
    if (AnimationMap.Contains(NameWithoutExt))
    {
        return AnimationMap[NameWithoutExt];
    }
    if (AnimationMap.Contains(path.c_str()))
    {
        return AnimationMap[path.c_str()];
    }
    
    LoadFile(path, static_cast<uint8>(EExtensionType::Fbx));

    if (AnimationMap.Contains(NameWithoutExt))
    {
        return AnimationMap[NameWithoutExt];
    }
    if (AnimationMap.Contains(path.c_str()))
    {
        return AnimationMap[path.c_str()];
    }
    return nullptr;
}

UStaticMesh* UAssetManager::GetStaticMesh(const FName& Name)
{
    std::filesystem::path path = std::filesystem::path(GetData(Name.ToString()));
    FName NameWithoutExt = FName(std::filesystem::path(path).replace_extension().c_str());
    if (StaticMeshMap.Contains(NameWithoutExt))
    {
        return StaticMeshMap[NameWithoutExt];
    }
    if (StaticMeshMap.Contains(path.c_str()))
    {
        return StaticMeshMap[path.c_str()];
    }
    
    LoadFile(path);

    if (StaticMeshMap.Contains(NameWithoutExt))
    {
        return StaticMeshMap[NameWithoutExt];
    }
    if (StaticMeshMap.Contains(path.c_str()))
    {
        return StaticMeshMap[path.c_str()];
    }
    return nullptr;
}

void UAssetManager::AddMaterial(UMaterial* InMaterial)
{
    FString BaseAssetName = InMaterial->GetMaterialInfo().MaterialName;
            
    FAssetInfo AssetInfo = {};
    AssetInfo.PackagePath = "";
    AssetInfo.Size = 0;
    AssetInfo.AssetName = FName(BaseAssetName);
    AssetInfo.AssetType = EAssetType::Material;
    AssetRegistry->PathNameToAssetInfo.Add(AssetInfo.AssetName, AssetInfo);

    
    FString Key = AssetInfo.AssetName.ToString();
    MaterialMap.Add(Key, InMaterial);
}

// void UAssetManager::AddMaterial(const FName& Key, UMaterial* InMaterial)
// {
//     MaterialMap.Add(Key, InMaterial);
// }

// void UAssetManager::AddSkeletalMesh(const FName& Key, USkeletalMesh* Mesh)
// {
//     SkeletalMeshMap.Add(Key, Mesh);
// }
//
// void UAssetManager::AddAnimationAsset(const FName& InKey, UAnimationAsset* InValue)
// {
//     AnimationMap.Add(InKey, InValue);
// }
//
// void UAssetManager::AddStaticMesh(const FName& InKey, UStaticMesh* InValue)
// {
//     StaticMeshMap.Add(InKey, InValue);
// }

void UAssetManager::LoadFiles(uint8 ExtensionFlags)
{
    const std::string BasePathName = "Contents/";

    for (const auto& Entry : std::filesystem::recursive_directory_iterator(BasePathName))
    {
        LoadFile(Entry, ExtensionFlags);
    }
}

void UAssetManager::LoadFile(std::filesystem::path Entry, uint8 ExtensionFlags)
{
    if (Entry.extension() == ".obj" && (ExtensionFlags & static_cast<uint8>(EExtensionType::Obj)))
    {
        // 경로, 이름 준비
        const FString FilePath = Entry.parent_path().string() + "/" + Entry.filename().string();
        const FString FileNameWithoutExt = Entry.stem().filename().string();
        
        FObjLoadResult Result = {};
        if (!FObjManager::LoadOBJ(FilePath.ToWideString(), Result))
        {
            return;
        }

        // AssetInfo 기본 필드 세팅
        FAssetInfo AssetInfo = {};
        AssetInfo.PackagePath = FName(Entry.parent_path().wstring());
        AssetInfo.Size = static_cast<uint32>(std::filesystem::file_size(Entry));

        //for (const auto& StaticMesh : Result.StaticMesh)
        if (Result.StaticMesh != nullptr)
        {
            FAssetInfo Info = AssetInfo;
            Info.AssetName = FName(Entry.filename().string());
            Info.AssetType = EAssetType::StaticMesh; // obj 파일은 무조건 StaticMesh
            
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);
        
            FString MeshName = Info.PackagePath.ToString() + "/" + Info.AssetName.ToString();
            StaticMeshMap.Add(MeshName, Result.StaticMesh);
        }

        for (const auto& Material : Result.Materials)
        {
            FString BaseAssetName = Material->GetMaterialInfo().MaterialName;
            
            FAssetInfo Info = AssetInfo;
            Info.AssetName = FName(BaseAssetName);
            Info.AssetType = EAssetType::Material;
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);

            FString Key = Info.AssetName.ToString();
            MaterialMap.Add(Key, Material);
        }
    }
    else if (Entry.extension() == ".fbx" && (ExtensionFlags & static_cast<uint8>(EExtensionType::Fbx)))
    {
        // 경로, 이름 준비
        const FString FilePath = Entry.parent_path().string() + "/" + Entry.filename().string();
        const FString FileNameWithoutExt = Entry.stem().filename().string();

        // FBX 로더로 파일 읽기

        FFbxLoadResult Result;
        if (!FManagerFBX::LoadFBX(FilePath.ToWideString(), Result))
        {
            return;
        }

        // AssetInfo 기본 필드 세팅
        FAssetInfo AssetInfo = {};
        AssetInfo.PackagePath = FName(Entry.parent_path().wstring());
        AssetInfo.Size = static_cast<uint32>(std::filesystem::file_size(Entry));

        // 로드된 Skeleton 등록
        for (int32 i = 0; i < Result.Skeletons.Num(); ++i)
        {
            USkeleton* Skeleton = Result.Skeletons[i];
            FString BaseAssetName = FileNameWithoutExt;
                
            FAssetInfo Info = AssetInfo;
            Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
            Info.AssetType = EAssetType::SkeletalMesh;
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);

            FString Key = Info.PackagePath.ToString() + "/" + Info.AssetName.ToString();
            // SkeletonMap.Add(Key, Skeleton);
        }
        
        // 로드된 SkeletalMesh 등록
        for (int32 i = 0; i < Result.SkeletalMeshes.Num(); ++i)
        {
            USkeletalMesh* SkeletalMesh = Result.SkeletalMeshes[i];
            FString BaseAssetName = FileNameWithoutExt;
                
            FAssetInfo Info = AssetInfo;
            Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
            Info.AssetType = EAssetType::SkeletalMesh;
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);

            FString Key = Info.PackagePath.ToString() + "/" + Info.AssetName.ToString();
            SkeletalMeshMap.Add(Key, SkeletalMesh);
        }
            
        //로드된 Animation 등록
        for (int32 i = 0; i < Result.Animations.Num(); ++i)
        {
            UAnimationAsset* AnimationAsset = Result.Animations[i];
            FString BaseAssetName = FileNameWithoutExt + "_" + AnimationAsset->GetName();
                
            FAssetInfo Info = AssetInfo;
            Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
            Info.AssetType = EAssetType::Animation;
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);
            
            FString Key = Info.PackagePath.ToString() + "/" + Info.AssetName.ToString();
            AnimationMap.Add(Key, AnimationAsset);
        }

        for (int32 i = 0; i < Result.Materials.Num(); ++i)
        {
            UMaterial* Material = Result.Materials[i];
            FString BaseAssetName = Material->GetMaterialInfo().MaterialName;
            
            FAssetInfo Info = AssetInfo;
            Info.AssetName = FName(BaseAssetName);
            Info.AssetType = EAssetType::Material;
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);

            FString Key = Info.AssetName.ToString();
            MaterialMap.Add(Key, Material);
        }
    }
}
