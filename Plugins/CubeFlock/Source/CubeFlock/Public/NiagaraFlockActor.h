#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraFlockActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** Drives NS_CubeFlock User.FollowTarget from the mouse (same deprojection as CubeFlockActor). */
UCLASS(BlueprintType, Blueprintable)
class CUBEFLOCK_API ANiagaraFlockActor : public AActor
{
	GENERATED_BODY()
public:
	ANiagaraFlockActor();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flock")
	TObjectPtr<UNiagaraComponent> Niagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock")
	TObjectPtr<UNiagaraSystem> FlockSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock|Mouse")
	bool bFollowMouse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock|Mouse", meta=(ClampMin="0", ClampMax="5"))
	float MouseAttraction = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flock", meta=(ClampMin="400", ClampMax="2200"))
	float FlockRadius = 1400.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Flock|Mouse")
	FVector MouseTarget = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Flock|Mouse")
	bool bHasMouseTarget = false;

protected:
	virtual void BeginPlay() override;
	void UpdateMouseTarget();
	void PushNiagaraParameters(float DeltaSeconds);
	float SimulationTime = 0;
};
