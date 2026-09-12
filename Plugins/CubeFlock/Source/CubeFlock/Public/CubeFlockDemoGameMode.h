#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "CubeFlockDemoGameMode.generated.h"

UCLASS()
class CUBEFLOCK_API ACubeFlockDemoPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ACubeFlockDemoPlayerController();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Flock|Mouse")
    FVector LastNiagaraTarget = FVector::ZeroVector;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Flock|Mouse")
    bool bHasNiagaraTarget = false;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
private:
    void BeginCameraLook();
    void EndCameraLook();
    void UpdateNiagaraMouseFollow(float DeltaSeconds);
    bool ProjectMouseToFlockPlane(const FVector& FlockOrigin, float FlockRadius, FVector& OutLocalTarget) const;

    FVector2D SavedCursor = FVector2D::ZeroVector;
    bool bRestoreCursor = false;
    float SimulationTime = 0;
};

UCLASS()
class CUBEFLOCK_API ACubeFlockDemoHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};

UCLASS()
class CUBEFLOCK_API ACubeFlockDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACubeFlockDemoGameMode();
};
