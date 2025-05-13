#include "Cube.h"

#include "Components/Mesh/StaticMeshComponent.h"
#include "Engine/AssetManager.h"


#include "GameFramework/Actor.h"

ACube::ACube()
{
    StaticMeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/CubePrimitive.obj"));
}

void ACube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetActorRotation(GetActorRotation() + FRotator(0, 0, 1));
}
