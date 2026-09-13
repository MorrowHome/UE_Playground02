#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassSpringActor.generated.h"

UCLASS()
class MASSSPRING_API AMassSpringActor : public AActor
{
    GENERATED_BODY()

public:
    AMassSpringActor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category = "MassSpring|Debug", meta = (ClampMin = "0.01"))
    float EdgeLengthMeters = 1.0f;

    UPROPERTY(EditAnywhere, Category = "MassSpring|Debug")
    FVector CenterMeters = FVector(0.0, 0.0, 2.0);

protected:
    virtual void BeginPlay() override;
};
