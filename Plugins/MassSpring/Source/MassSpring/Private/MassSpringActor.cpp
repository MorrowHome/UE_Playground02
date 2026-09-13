#include "MassSpringActor.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

AMassSpringActor::AMassSpringActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMassSpringActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Display, TEXT("MassSpring BeginPlay: %s"), *GetName());
}

void AMassSpringActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector CenterCm = GetActorLocation() + CenterMeters * 100.0;
    const FVector HalfExtentCm(FMath::Max(EdgeLengthMeters, 0.01f) * 50.0f);
    DrawDebugBox(GetWorld(), CenterCm, HalfExtentCm, FColor::Green,
                 false, -1.0f, 0, 2.0f);
}
