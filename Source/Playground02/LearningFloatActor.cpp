#include "LearningFloatActor.h"

ALearningFloatActor::ALearningFloatActor() {
	PrimaryActorTick.bCanEverTick = true;
	
	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	SetRootComponent(CubeMesh);
	CubeMesh->SetMobility(EComponentMobility::Movable);
	CubeMesh->SetSimulatePhysics(false);
	CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALearningFloatActor::BeginPlay() {
	Super::BeginPlay();
	
	StartLocation = GetActorLocation();
	ElapsedSeconds = 0.0f;
	UE_LOG(LogTemp, Display, TEXT("Floating cube started. Amplitude=%.1f cm, Frequency=%.2f Hz"), AmplitudeCm, FrequencyHz);
}

void ALearningFloatActor::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	if (!bEnableFloating) return;
	ElapsedSeconds += DeltaSeconds;
	const float SafeAmplitude = FMath::Max(AmplitudeCm, 0.0f);
	const float SafeFrequency = FMath::Max(FrequencyHz, 0.0f);
	const float Phase = 2.0f * PI * SafeFrequency * ElapsedSeconds;
	const float OffsetZ = SafeAmplitude * FMath::Sin(Phase);
	
	FVector NewLocation = StartLocation;
	NewLocation.Z += OffsetZ;
	SetActorLocation(NewLocation);
}

