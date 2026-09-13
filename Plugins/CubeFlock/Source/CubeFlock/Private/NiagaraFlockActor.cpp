#include "NiagaraFlockActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

ANiagaraFlockActor::ANiagaraFlockActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	SetRootComponent(Niagara);
	Niagara->SetAutoActivate(true);
	// This module registers global shaders at PostConfigInit. Loading a Niagara
	// system here can run its PostLoad before the Niagara module has started.
	DefaultFlockSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/NiagaraFlockDemo/NS_CubeFlock.NS_CubeFlock")));
}

void ANiagaraFlockActor::BeginPlay()
{
	Super::BeginPlay();
	if (!FlockSystem)
	{
		FlockSystem = Niagara->GetAsset();
		if (!FlockSystem)
		{
			FlockSystem = DefaultFlockSystem.LoadSynchronous();
		}
	}
	if (FlockSystem && Niagara->GetAsset() != FlockSystem)
	{
		Niagara->SetAsset(FlockSystem);
	}
	if (FlockSystem)
	{
		Niagara->Activate(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NiagaraFlockActor %s has no Niagara system."), *GetName());
	}
}

void ANiagaraFlockActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SimulationTime += DeltaSeconds;
	UpdateMouseTarget();
	PushNiagaraParameters(DeltaSeconds);
}

void ANiagaraFlockActor::UpdateMouseTarget()
{
	if (!bFollowMouse) return;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController() || PC->IsInputKeyDown(EKeys::RightMouseButton)) return;
	float MouseX = 0, MouseY = 0;
	int32 Width = 0, Height = 0;
	PC->GetViewportSize(Width, Height);
	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	if (!PC->GetMousePosition(MouseX, MouseY)) return;
	if (MouseX < 0 || MouseY < 0 || MouseX >= Width || MouseY >= Height) return;
	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection)) return;
	const FVector Normal = CameraRotation.Vector();
	const double Denominator = FVector::DotProduct(RayDirection, Normal);
	if (FMath::Abs(Denominator) < 0.0001) return;
	const double Distance = FVector::DotProduct(GetActorLocation() - RayOrigin, Normal) / Denominator;
	if (Distance <= 0) return;
	FVector Local = RayOrigin + RayDirection * Distance - GetActorLocation();
	const double Radius = FMath::Clamp(FlockRadius, 400.0f, 2200.0f);
	const double Extent = FVector(Local.X / Radius, Local.Y / Radius, Local.Z / (Radius * 0.45)).Length();
	if (Extent > 0.72) Local *= 0.72 / Extent;
	if (Local.ContainsNaN()) return;
	MouseTarget = Local;
	bHasMouseTarget = true;
}

void ANiagaraFlockActor::PushNiagaraParameters(float /*DeltaSeconds*/)
{
	if (!Niagara || !Niagara->GetAsset()) return;

	FVector Target = MouseTarget;
	float Strength = 0.0f;
	if (bFollowMouse && bHasMouseTarget)
	{
		Strength = FMath::Clamp(MouseAttraction, 0.0f, 5.0f);
	}
	else
	{
		const float Radius = FMath::Clamp(FlockRadius, 400.0f, 2200.0f) * 0.42f;
		Target = FVector(
			FMath::Cos(SimulationTime * 0.23f) * Radius,
			FMath::Sin(SimulationTime * 0.23f) * Radius,
			FMath::Sin(SimulationTime * 0.37f) * Radius * 0.18f);
		Strength = 1.2f;
	}

	const float NiagaraStrength = Strength * 280.0f;
	// User.FollowTarget is Vector3f (local space). Also set Position alias if present.
	Niagara->SetVariableVec3(FName(TEXT("User.FollowTarget")), Target);
	Niagara->SetVariableFloat(FName(TEXT("User.FollowStrength")), NiagaraStrength);
}
