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

    FString FBXPath = TEXT("Assets/FBX/Aurora.FBX");

    // 2. 메시 데이터 구조체 생성
    FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();

    FbxScene* Scene = nullptr;
    FbxMesh* Mesh = nullptr;

    // 3. FBX 로딩 (T-Pose로 변환 포함)
    if (FFbxLoader::LoadFBX(FBXPath, *RenderData, true,&Scene,&Mesh))
    {

        TArray<FSkeletonBone> Bones;
        ExtractSkeleton(Mesh, Bones);
        

        UE_LOG(ELogLevel::Display, TEXT("Bone 개수: %d"), Bones.Num());

        for (const FSkeletonBone& Bone : Bones)
        {
            UE_LOG(ELogLevel::Display, TEXT("Bone: %s (Parent: %d)"), *Bone.Name, Bone.ParentIndex);
        }

        RecalculateGlobalPoses(Bones);

        int32 PickedBone = FindBoneByName(Bones, TEXT("spine_01")); // 또는 index 직접
        if (PickedBone != -1)
        {
            //RotateBone(Bones, PickedBone, FbxVector4(0, 90, 0)); // Y축 30도 회전
            ReskinVerticesCPU(Mesh, Bones, RenderData->Vertices);
        }

        FFbxLoader::ComputeBoundingBox(RenderData->Vertices, RenderData->BoundingBoxMin, RenderData->BoundingBoxMax);


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

        //FVertexInfo VertexInfo;
        //FEngineLoop::Renderer.BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
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
