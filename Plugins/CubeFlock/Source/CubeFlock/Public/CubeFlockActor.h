#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeFlockActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;
struct FCubeFlockGPUState;

/** GPU Boids demo. Move the actor to move the flock's center. Play to simulate. */
UCLASS(BlueprintType, Blueprintable)
class CUBEFLOCK_API ACubeFlockActor : public AActor
{
    GENERATED_BODY()
public:
    ACubeFlockActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flock")
    TObjectPtr<UInstancedStaticMeshComponent> Cubes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="1", ClampMax="4096", ToolTip="Applied when Play starts. All-pairs GPU simulation; start with 2000."))
    int32 BirdCount = 2000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="4", ClampMax="100"))
    float CubeSize = 24.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="400", ClampMax="2200"))
    float FlockRadius = 1400.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="50", ClampMax="800"))
    float Speed = 330.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="50", ClampMax="600"))
    float NeighborRadius = 280.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="0", ClampMax="4"))
    float SeparationWeight = 1.7f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="0", ClampMax="4"))
    float AlignmentWeight = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="0", ClampMax="4"))
    float CohesionWeight = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock")
    int32 RandomSeed = 42;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock")
    TObjectPtr<UMaterialInterface> FlockMaterial;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock|Mouse")
    bool bFollowMouse = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock|Mouse", meta=(ClampMin="0", ClampMax="5"))
    float MouseAttraction = 2.4f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Flock|Mouse")
    FVector MouseTarget = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Flock|Mouse")
    bool bHasMouseTarget = false;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void BuildInstances();
    void UpdateMouseTarget();
    void ValidateMouseFollowing();
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> PositionTexture;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
    TSharedPtr<FCubeFlockGPUState, ESPMode::ThreadSafe> GPUState;
    int32 ActiveCount = 0;
    float SimulationTime = 0;
    bool bValidate = false;
    bool bValidateMouse = false;
    int32 MouseValidationPhase = 0;
    double MouseValidationStartDistance = 0;
    bool bValidationFirst = false;
    bool bValidationDone = false;
    TArray<FLinearColor> ValidationSnapshot;
};



