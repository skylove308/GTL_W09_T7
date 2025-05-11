#include "Cube.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/Mesh/StaticMeshComponent.h"

#include "Engine/FObjLoader.h"
#include "Engine/FbxLoader.h"

#include "GameFramework/Actor.h"

ACube::ACube()
{
    StaticMeshComponent->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/CubePrimitive.obj"));
}

void ACube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetActorRotation(GetActorRotation() + FRotator(0, 0, 1));
}
