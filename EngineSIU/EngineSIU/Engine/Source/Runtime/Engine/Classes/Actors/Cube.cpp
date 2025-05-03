#include "Cube.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/FObjLoader.h"

#include "GameFramework/Actor.h"
#include "Engine/FBXLoader.h"

ACube::ACube()
{
    //StaticMeshComponent->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Reference/Reference.obj"));

    FString FBXPath = TEXT("Assets/FBX/Unreal_Mannequin.fbx");

    // 2. 메시 데이터 구조체 생성
    FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();

    FbxScene* Scene = nullptr;
    FbxMesh* Mesh = nullptr;

    // 3. FBX 로딩 (T-Pose로 변환 포함)
    if (FFbxLoader::LoadFBX(FBXPath, *RenderData, true))
    {

        ExtractSkeleton(Mesh, Bones);
        RecalculateGlobalPoses(Bones);

        // 4. UStaticMesh 생성
        UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr);
        StaticMesh->SetData(RenderData);

        // 5. 메시 컴포넌트에 할당
        //UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(MyActor);
        //MeshComponent->StaticMesh = StaticMesh;

        // 6. 씬에 붙이기 (또는 등록)
        //MeshComponent->RegisterComponent();
        //MyActor->AddComponent(MeshComponent);
        StaticMeshComponent->SetStaticMesh(StaticMesh);
    }
    else
    {
        UE_LOG(ELogLevel::Display, TEXT("FBX Load 실패: %s"), *FBXPath);
    }
    
}

void ACube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //SetActorRotation(GetActorRotation() + FRotator(0, 0, 1));

}
