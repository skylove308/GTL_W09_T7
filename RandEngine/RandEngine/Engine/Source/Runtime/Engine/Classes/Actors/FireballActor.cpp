#include "FireballActor.h"

#include "Components/Light/PointLightComponent.h"

#include "Components/ProjectileMovementComponent.h"

#include "Components/SphereComp.h"
#include "Engine/AssetManager.h"

AFireballActor::AFireballActor()
{
    SphereComp = AddComponent<USphereComp>("USphereComp_0");
    
    SphereComp->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/Sphere.obj"));
  
    PointLightComponent = AddComponent<UPointLightComponent>("UPointLightComponent_0");
    
    PointLightComponent->SetLightColor(FLinearColor::Red);
    
    ProjectileMovementComponent = AddComponent<UProjectileMovementComponent>("UProjectileMovementComponent_0");
    PointLightComponent->AttachToComponent(RootComponent);

    ProjectileMovementComponent->SetGravity(0);
    ProjectileMovementComponent->SetVelocity(FVector(100, 0, 0));
    ProjectileMovementComponent->SetInitialSpeed(100);
    ProjectileMovementComponent->SetMaxSpeed(100);
    ProjectileMovementComponent->SetLifetime(10);
}

AFireballActor::~AFireballActor()
{
}

void AFireballActor::BeginPlay()
{
}
