#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LearningFloatActor.generated.h"

class UStaticMeshComponent; // 前置申明，告诉编译器这个类型完整存在，完整定义在 .cpp include

UCLASS()
class PLAYGROUND02_API ALearningFloatActor : public AActor {
	GENERATED_BODY()

public:
	ALearningFloatActor();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Learning")
	TObjectPtr<UStaticMeshComponent> CubeMesh; // TObjectPtr 是 UE 的对象引用类型，保存组件引用
	UPROPERTY(EditAnywhere, Category = "Learning|Motion")
	bool bEnableFloating = true;
	UPROPERTY(EditAnywhere, Category = "Learning|Motion", meta = (ClampMin = 0.0f, Units = "cm"))
	float AmplitudeCm = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Learning|Motion", meta = (ClampMin = 0.0f))
	float FrequencyHz = 0.5f;

private:
	FVector StartLocation = FVector::ZeroVector;
	float ElapsedSeconds = 0.0f;
};
