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

USkeletalMesh* UAssetManager::GetSkeletalMesh(const FName& Name)
{
    std::filesystem::path path = std::filesystem::path(GetData(Name.ToString()));
    FName NameWithoutExt = FName(std::filesystem::path(path).replace_extension().c_str());
    
    if (SkeletalMeshMap.Contains(NameWithoutExt))
    {
        return SkeletalMeshMap[NameWithoutExt];
    }
    LoadFile(path, static_cast<uint8>(EExtensionType::Fbx));

    if (SkeletalMeshMap.Contains(NameWithoutExt))
    {
        return SkeletalMeshMap[NameWithoutExt];
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
    
    LoadFile(path, static_cast<uint8>(EExtensionType::Fbx));

    if (AnimationMap.Contains(NameWithoutExt))
    {
        return AnimationMap[NameWithoutExt];
    }
    return nullptr;
}

void UAssetManager::AddSkeletalMesh(const FName& Key, USkeletalMesh* Mesh)
{
    SkeletalMeshMap.Add(Key, Mesh);
}

void UAssetManager::AddAnimationAsset(const FName& InKey, UAnimationAsset* InValue)
{
    AnimationMap.Add(InKey, InValue);
}

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
        FAssetInfo NewAssetInfo;
        NewAssetInfo.AssetName = FName(Entry.filename().string());
        NewAssetInfo.PackagePath = FName(Entry.parent_path().string());
        NewAssetInfo.AssetType = EAssetType::StaticMesh; // obj 파일은 무조건 StaticMesh
        NewAssetInfo.Size = static_cast<uint32>(std::filesystem::file_size(Entry));
            
        AssetRegistry->PathNameToAssetInfo.Add(NewAssetInfo.AssetName, NewAssetInfo);
            
        FString MeshName = NewAssetInfo.PackagePath.ToString() + "/" + NewAssetInfo.AssetName.ToString();
        FObjManager::CreateStaticMesh(MeshName);
        // ObjFileNames.push_back(UGTLStringLibrary::StringToWString(Entry.path().string()));
        // FObjManager::LoadObjStaticMeshAsset(UGTLStringLibrary::StringToWString(Entry.path().string()));
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
            FString BaseAssetName = FileNameWithoutExt + "_Skeleton";
                
            FAssetInfo Info = AssetInfo;
            Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
            Info.AssetType = EAssetType::Animation;
            AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);
            
            FString Key = Info.PackagePath.ToString() + "/" + Info.AssetName.ToString();
            AnimationMap.Add(Key, AnimationAsset);
        }
    }
}
