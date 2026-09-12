#include "CubeFlockDemoGameMode.h"
#include "GameFramework/SpectatorPawn.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "CubeFlockActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

ACubeFlockDemoPlayerController::ACubeFlockDemoPlayerController()
{
    bShowMouseCursor = true;
    PrimaryActorTick.bCanEverTick = true;
}

void ACubeFlockDemoPlayerController::BeginPlay()
{
    Super::BeginPlay();
    EndCameraLook();
}

void ACubeFlockDemoPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    SimulationTime += DeltaSeconds;
    UpdateNiagaraMouseFollow(DeltaSeconds);
}

void ACubeFlockDemoPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ACubeFlockDemoPlayerController::BeginCameraLook);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ACubeFlockDemoPlayerController::EndCameraLook);
}

void ACubeFlockDemoPlayerController::BeginCameraLook()
{
    float X = 0, Y = 0;
    bRestoreCursor = GetMousePosition(X, Y);
    SavedCursor = FVector2D(X, Y);
    ResetIgnoreLookInput();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}

void ACubeFlockDemoPlayerController::EndCameraLook()
{
    ResetIgnoreLookInput();
    SetIgnoreLookInput(true);
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    if (bRestoreCursor)
    {
        SetMouseLocation(FMath::RoundToInt(SavedCursor.X), FMath::RoundToInt(SavedCursor.Y));
        bRestoreCursor = false;
    }
}

bool ACubeFlockDemoPlayerController::ProjectMouseToFlockPlane(const FVector& FlockOrigin, float FlockRadius, FVector& OutLocalTarget) const
{
    if (!IsLocalController() || IsInputKeyDown(EKeys::RightMouseButton)) return false;
    float MouseX = 0, MouseY = 0;
    int32 Width = 0, Height = 0;
    GetViewportSize(Width, Height);
    if (!GetMousePosition(MouseX, MouseY)) return false;
    if (MouseX < 0 || MouseY < 0 || MouseX >= Width || MouseY >= Height) return false;
    FVector RayOrigin, RayDirection;
    if (!DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection)) return false;
    FVector CameraLocation;
    FRotator CameraRotation;
    GetPlayerViewPoint(CameraLocation, CameraRotation);
    const FVector Normal = CameraRotation.Vector();
    const double Denominator = FVector::DotProduct(RayDirection, Normal);
    if (FMath::Abs(Denominator) < 0.0001) return false;
    const double Distance = FVector::DotProduct(FlockOrigin - RayOrigin, Normal) / Denominator;
    if (Distance <= 0) return false;
    FVector Local = RayOrigin + RayDirection * Distance - FlockOrigin;
    const double Radius = FMath::Clamp(FlockRadius, 400.0, 2200.0);
    const double Extent = FVector(Local.X / Radius, Local.Y / Radius, Local.Z / (Radius * 0.45)).Length();
    if (Extent > 0.72) Local *= 0.72 / Extent;
    if (Local.ContainsNaN()) return false;
    OutLocalTarget = Local;
    return true;
}

void ACubeFlockDemoPlayerController::UpdateNiagaraMouseFollow(float /*DeltaSeconds*/)
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (TObjectIterator<UNiagaraComponent> It; It; ++It)
    {
        UNiagaraComponent* Comp = *It;
        if (!Comp || Comp->GetWorld() != World || !Comp->GetAsset()) continue;
        const FString AssetName = Comp->GetAsset()->GetName();
        if (!AssetName.Contains(TEXT("NS_CubeFlock"))) continue;

        const FVector Origin = Comp->GetComponentLocation();
        constexpr float FlockRadius = 1400.0f;
        FVector Target;
        float StrengthKnob = 1.2f;
        if (ProjectMouseToFlockPlane(Origin, FlockRadius, Target))
        {
            LastNiagaraTarget = Target;
            bHasNiagaraTarget = true;
            StrengthKnob = 2.4f;
        }
        else if (bHasNiagaraTarget)
        {
            Target = LastNiagaraTarget;
            StrengthKnob = 2.4f;
        }
        else
        {
            const float Radius = FlockRadius * 0.42f;
            Target = FVector(
                FMath::Cos(SimulationTime * 0.23f) * Radius,
                FMath::Sin(SimulationTime * 0.23f) * Radius,
                FMath::Sin(SimulationTime * 0.37f) * Radius * 0.18f);
            StrengthKnob = 1.2f;
        }

        Comp->SetVariableVec3(FName(TEXT("User.FollowTarget")), Target);
        Comp->SetVariableFloat(FName(TEXT("User.FollowStrength")), StrengthKnob * 280.0f);
    }
}

ACubeFlockDemoGameMode::ACubeFlockDemoGameMode()
{
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    PlayerControllerClass = ACubeFlockDemoPlayerController::StaticClass();
    HUDClass = ACubeFlockDemoHUD::StaticClass();
}

void ACubeFlockDemoHUD::DrawHUD()
{
    Super::DrawHUD();
    DrawRect(FLinearColor(0.012f, 0.02f, 0.04f, 0.85f), 24, 24, 660, 112);
    int32 Count = 0;
    bool bNiagara = false;
    for (TActorIterator<ACubeFlockActor> It(GetWorld()); It; ++It)
    {
        Count += It->Cubes->GetInstanceCount();
        if (It->bFollowMouse && It->bHasMouseTarget && PlayerOwner)
        {
            FVector2D Screen;
            if (PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + It->MouseTarget, Screen))
            {
                const FLinearColor Color(1.0f, 0.65f, 0.12f);
                DrawLine(Screen.X - 12, Screen.Y, Screen.X + 12, Screen.Y, Color, 2);
                DrawLine(Screen.X, Screen.Y - 12, Screen.X, Screen.Y + 12, Color, 2);
                DrawText(TEXT("TARGET"), Color, Screen.X + 16, Screen.Y + 8);
            }
        }
    }
    for (TObjectIterator<UNiagaraComponent> It; It; ++It)
    {
        UNiagaraComponent* Comp = *It;
        if (!Comp || Comp->GetWorld() != GetWorld() || !Comp->GetAsset()) continue;
        if (!Comp->GetAsset()->GetName().Contains(TEXT("NS_CubeFlock"))) continue;
        bNiagara = true;
        Count = FMath::Max(Count, 2000);
        if (ACubeFlockDemoPlayerController* PC = Cast<ACubeFlockDemoPlayerController>(PlayerOwner))
        {
            if (PC->bHasNiagaraTarget)
            {
                FVector2D Screen;
                if (PlayerOwner->ProjectWorldLocationToScreen(Comp->GetComponentLocation() + PC->LastNiagaraTarget, Screen))
                {
                    const FLinearColor Color(1.0f, 0.55f, 0.15f);
                    DrawLine(Screen.X - 12, Screen.Y, Screen.X + 12, Screen.Y, Color, 2);
                    DrawLine(Screen.X, Screen.Y - 12, Screen.X, Screen.Y + 12, Color, 2);
                    DrawText(TEXT("TARGET"), Color, Screen.X + 16, Screen.Y + 8);
                }
            }
        }
    }

    DrawText(FString::Printf(TEXT("%s  /  %d BOIDS"), bNiagara ? TEXT("NIAGARA CUBE FLOCK") : TEXT("GPU CUBE FLOCK"), Count),
        FLinearColor(0.25f, 0.85f, 1.0f), 42, 36, nullptr, 1.5f);
    DrawText(TEXT("Move mouse: guide flock   Hold right mouse: look around"), FLinearColor::White, 42, 70);
    DrawText(TEXT("WASD: fly   Q/E: down/up   Esc: stop PIE"), FLinearColor(0.7f, 0.8f, 0.9f), 42, 94);
}
