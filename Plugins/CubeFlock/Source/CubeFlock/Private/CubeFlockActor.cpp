#include "CubeFlockActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "TextureResource.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogCubeFlock, Log, All);

namespace
{
constexpr uint32 TextureWidth = 64;
struct alignas(16) FBirdGPU
{
    FVector4f Position;
    FVector4f Velocity;
};
static_assert(sizeof(FBirdGPU) == 32);

TArray<FBirdGPU> MakeBirds(int32 Count, int32 Seed, float Radius, float Speed)
{
    FRandomStream Random(Seed);
    TArray<FBirdGPU> Birds;
    Birds.Reserve(Count);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        FVector P = Random.VRand() * (Radius * FMath::Pow(Random.FRand(), 1.0f / 3.0f) * 0.75f);
        P.Z *= 0.45f;
        const FVector V = Random.VRand() * Speed;
        Birds.Add({FVector4f(FVector3f(P), 0), FVector4f(FVector3f(V), 0)});
    }
    return Birds;
}
}

struct FCubeFlockGPUState
{
    TRefCountPtr<FRDGPooledBuffer> Birds;
};

class FCubeFlockCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FCubeFlockCS);
    SHADER_USE_PARAMETER_STRUCT(FCubeFlockCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(uint32, BirdCount)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, Time)
        SHADER_PARAMETER(float, Radius)
        SHADER_PARAMETER(float, MoveSpeed)
        SHADER_PARAMETER(float, NeighborDistance)
        SHADER_PARAMETER(float, Separation)
        SHADER_PARAMETER(float, Alignment)
        SHADER_PARAMETER(float, Cohesion)
        SHADER_PARAMETER(FVector3f, FollowTarget)
        SHADER_PARAMETER(float, FollowStrength)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FBird>, BirdsIn)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FBird>, BirdsOut)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, PositionsOut)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FCubeFlockCS, "/Plugin/CubeFlock/CubeFlock.usf", "MainCS", SF_Compute);

ACubeFlockActor::ACubeFlockActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    Cubes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cubes"));
    SetRootComponent(Cubes);
    Cubes->SetMobility(EComponentMobility::Movable);
    Cubes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Cubes->SetGenerateOverlapEvents(false);
    Cubes->SetCastShadow(false);
    Cubes->SetCanEverAffectNavigation(false);
    Cubes->SetEvaluateWorldPositionOffset(true);
    Cubes->SetBoundsScale(10.0f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Cubes->SetStaticMesh(Cube.Object);
}

void ACubeFlockActor::BuildInstances()
{
    Cubes->ClearInstances();
    Cubes->SetNumCustomDataFloats(4);
    const auto Birds = MakeBirds(FMath::Clamp(BirdCount, 1, 4096), RandomSeed,
        FMath::Clamp(FlockRadius, 400.0f, 2200.0f), Speed);
    TArray<FTransform> Transforms;
    Transforms.Reserve(Birds.Num());
    for (const auto& Bird : Birds)
        Transforms.Emplace(FQuat::Identity, FVector(Bird.Position.X, Bird.Position.Y, Bird.Position.Z),
            FVector(FMath::Clamp(CubeSize, 4.0f, 100.0f) / 100.0f));
    Cubes->AddInstances(Transforms, false, false, false);
    for (int32 I = 0; I < Birds.Num(); ++I)
    {
        Cubes->SetCustomDataValue(I, 0, float(I));
        Cubes->SetCustomDataValue(I, 1, Birds[I].Position.X);
        Cubes->SetCustomDataValue(I, 2, Birds[I].Position.Y);
        Cubes->SetCustomDataValue(I, 3, Birds[I].Position.Z);
    }
    if (FlockMaterial) Cubes->SetMaterial(0, FlockMaterial);
    Cubes->MarkRenderStateDirty();
}

void ACubeFlockActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!HasActorBegunPlay()) BuildInstances();
}

void ACubeFlockActor::BeginPlay()
{
    Super::BeginPlay();
    if (!FlockMaterial || GMaxRHIFeatureLevel < ERHIFeatureLevel::SM5 || IsRunningDedicatedServer())
    {
        UE_LOG(LogCubeFlock, Error, TEXT("Cube flock requires the generated flock material and an SM5+ GPU."));
        SetActorTickEnabled(false);
        return;
    }
    BuildInstances();
    ActiveCount = Cubes->GetInstanceCount();
    PositionTexture = NewObject<UTextureRenderTarget2D>(this);
    PositionTexture->bCanCreateUAV = true;
    PositionTexture->bAutoGenerateMips = false;
    PositionTexture->Filter = TF_Nearest;
    PositionTexture->InitCustomFormat(TextureWidth, TextureWidth, PF_A32B32G32R32F, true);
    PositionTexture->UpdateResourceImmediate(true);
    DynamicMaterial = UMaterialInstanceDynamic::Create(FlockMaterial, this);
    DynamicMaterial->SetTextureParameterValue(TEXT("Positions"), PositionTexture);
    DynamicMaterial->SetScalarParameterValue(TEXT("SimulationEnabled"), 1.0f);
    Cubes->SetMaterial(0, DynamicMaterial);
    GPUState = MakeShared<FCubeFlockGPUState, ESPMode::ThreadSafe>();
    auto InitialBirds = MakeBirds(ActiveCount, RandomSeed, FMath::Clamp(FlockRadius, 400.0f, 2200.0f), Speed);
    auto State = GPUState;
    ENQUEUE_RENDER_COMMAND(InitializeCubeFlock)(
        [State, Birds = MoveTemp(InitialBirds)](FRHICommandListImmediate& RHICmdList)
        {
            FRDGBuilder Graph(RHICmdList);
            auto Buffer = CreateStructuredBuffer(Graph, TEXT("CubeFlock.InitialBirds"), sizeof(FBirdGPU),
                Birds.Num(), Birds.GetData(), Birds.Num() * sizeof(FBirdGPU));
            Graph.QueueBufferExtraction(Buffer, &State->Birds);
            Graph.Execute();
        });
    bValidate = FParse::Param(FCommandLine::Get(), TEXT("BirdFlockValidate"));
    bValidateMouse = FParse::Param(FCommandLine::Get(), TEXT("BirdFlockMouseValidate"));
    UE_LOG(LogCubeFlock, Display, TEXT("Started %d GPU cube boids; seed=%d. No per-frame CPU position updates."), ActiveCount, RandomSeed);
}

void ACubeFlockActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GPUState || !PositionTexture) return;
    const float Dt = FMath::Clamp(DeltaSeconds, 0.0f, 1.0f / 30.0f);
    SimulationTime += Dt;
    UpdateMouseTarget();
    auto State = GPUState;
    auto* Target = PositionTexture->GameThread_GetRenderTargetResource();
    FCubeFlockCS::FParameters Values {};
    Values.BirdCount = ActiveCount;
    Values.DeltaTime = Dt;
    Values.Time = SimulationTime;
    Values.Radius = FMath::Clamp(FlockRadius, 400.0f, 2200.0f);
    Values.MoveSpeed = FMath::Clamp(Speed, 50.0f, 800.0f);
    Values.NeighborDistance = FMath::Clamp(NeighborRadius, 50.0f, 600.0f);
    Values.Separation = FMath::Clamp(SeparationWeight, 0.0f, 4.0f);
    Values.Alignment = FMath::Clamp(AlignmentWeight, 0.0f, 4.0f);
    Values.Cohesion = FMath::Clamp(CohesionWeight, 0.0f, 4.0f);
    Values.FollowTarget = FVector3f(MouseTarget);
    Values.FollowStrength = bFollowMouse && bHasMouseTarget ? FMath::Clamp(MouseAttraction, 0.0f, 5.0f) : 0.0f;
    ENQUEUE_RENDER_COMMAND(UpdateCubeFlock)(
        [State, Target, Values](FRHICommandListImmediate& RHICmdList)
        {
            if (!State->Birds || !Target->GetRenderTargetTexture()) return;
            FRDGBuilder Graph(RHICmdList);
            auto Input = Graph.RegisterExternalBuffer(State->Birds);
            auto Output = Graph.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(FBirdGPU), Values.BirdCount), TEXT("CubeFlock.NextBirds"));
            auto Texture = Graph.RegisterExternalTexture(CreateRenderTarget(Target->GetRenderTargetTexture(), TEXT("CubeFlock.Positions")));
            auto* Parameters = Graph.AllocParameters<FCubeFlockCS::FParameters>();
            *Parameters = Values;
            Parameters->BirdsIn = Graph.CreateSRV(Input);
            Parameters->BirdsOut = Graph.CreateUAV(Output);
            Parameters->PositionsOut = Graph.CreateUAV(Texture);
            TShaderMapRef<FCubeFlockCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
            FComputeShaderUtils::AddPass(Graph, RDG_EVENT_NAME("CubeFlock.Boids %u", Values.BirdCount),
                Shader, Parameters, FIntVector(FMath::DivideAndRoundUp(Values.BirdCount, 64u), 1, 1));
            Graph.QueueBufferExtraction(Output, &State->Birds);
            Graph.SetTextureAccessFinal(Texture, ERHIAccess::SRVMask);
            Graph.Execute();
        });

    ValidateMouseFollowing();

    // Opt-in integration test only. Normal gameplay never reads positions back.
    if (bValidate && !bValidationDone && SimulationTime > (bValidationFirst ? 4.0f : 1.0f))
    {
        TArray<FLinearColor> Pixels;
        FReadSurfaceDataFlags Flags(RCM_MinMax);
        Flags.SetLinearToGamma(false);
        const bool bRead = Target->ReadLinearColorPixels(Pixels, Flags);
        if (!bRead || Pixels.Num() < ActiveCount)
        {
            UE_LOG(LogCubeFlock, Error, TEXT("GPU_VALIDATION_FAILED: texture readback failed"));
            bValidationDone = true;
            FPlatformMisc::RequestExitWithStatus(false, 1);
            return;
        }
        if (!bValidationFirst)
        {
            ValidationSnapshot = MoveTemp(Pixels);
            bValidationFirst = true;
        }
        else
        {
            int32 Moved = 0;
            bool bFinite = true;
            float MaxRadius = 0;
            for (int32 I = 0; I < ActiveCount; ++I)
            {
                const FVector P(Pixels[I].R, Pixels[I].G, Pixels[I].B);
                const FVector Previous(ValidationSnapshot[I].R, ValidationSnapshot[I].G, ValidationSnapshot[I].B);
                bFinite &= !P.ContainsNaN() && Pixels[I].A == 1.0f;
                Moved += FVector::DistSquared(P, Previous) > 1.0f ? 1 : 0;
                MaxRadius = FMath::Max(MaxRadius, float(FVector(P.X, P.Y, P.Z / 0.45f).Length()));
            }
            const bool bPassed = bFinite && Moved == ActiveCount && MaxRadius < Values.Radius * 1.05f;
            UE_LOG(LogCubeFlock, Display, TEXT("GPU_VALIDATION_%s: count=%d moved=%d finite=%d maxRadius=%.1f"),
                bPassed ? TEXT("PASSED") : TEXT("FAILED"), ActiveCount, Moved, bFinite, MaxRadius);
            bValidationDone = true;
            if (!bPassed) FPlatformMisc::RequestExitWithStatus(false, 1);
            FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots/CubeFlock.png")), true, false);
        }
    }
    if (bValidate && bValidationDone && SimulationTime > 5.0f)
        FPlatformMisc::RequestExit(false);
}

void ACubeFlockActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GPUState)
    {
        auto State = MoveTemp(GPUState);
        ENQUEUE_RENDER_COMMAND(ReleaseCubeFlock)(
            [State](FRHICommandListImmediate&) { State->Birds.SafeRelease(); });
    }
    Super::EndPlay(EndPlayReason);
}



void ACubeFlockActor::UpdateMouseTarget()
{
    if (!bFollowMouse || bValidate) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->IsLocalController() || PC->IsInputKeyDown(EKeys::RightMouseButton)) return;
    float MouseX = 0, MouseY = 0;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
    bool bHasCursor = PC->GetMousePosition(MouseX, MouseY);
    if (bValidateMouse)
    {
        // Deterministic screen-space cursor input for the opt-in integration test.
        // It still uses the same deprojection and clamp path as the real mouse.
        const FVector Right = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
        const FVector TestPoint = GetActorLocation() + Right * (SimulationTime < 7.0f ? 700.0f : -700.0f);
        FVector2D Screen;
        bHasCursor = PC->ProjectWorldLocationToScreen(TestPoint, Screen);
        MouseX = Screen.X;
        MouseY = Screen.Y;
    }
    if (!bHasCursor || MouseX < 0 || MouseY < 0 || MouseX >= Width || MouseY >= Height) return;
    FVector RayOrigin, RayDirection;
    if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection)) return;
    // Intersect a camera-facing plane through the flock center, so horizontal
    // and vertical cursor motion both produce intuitive movement in the view.
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

void ACubeFlockActor::ValidateMouseFollowing()
{
    if (!bValidateMouse) return;
    const float Times[] = {1.0f, 6.0f, 8.0f, 13.0f};
    if (MouseValidationPhase < 4 && SimulationTime >= Times[MouseValidationPhase])
    {
        TArray<FLinearColor> Pixels;
        FReadSurfaceDataFlags Flags(RCM_MinMax);
        Flags.SetLinearToGamma(false);
        const bool bRead = PositionTexture->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels, Flags);
        bool bValid = bRead && Pixels.Num() >= ActiveCount && bHasMouseTarget;
        FVector Mean = FVector::ZeroVector;
        if (bValid)
        {
            for (int32 I = 0; I < ActiveCount; ++I)
            {
                const FVector P(Pixels[I].R, Pixels[I].G, Pixels[I].B);
                bValid &= !P.ContainsNaN() && Pixels[I].A == 1.0f
                    && FVector(P.X, P.Y, P.Z / 0.45).Length() <= FlockRadius * 1.05f;
                Mean += P;
            }
            Mean /= ActiveCount;
        }
        if ((MouseValidationPhase % 2) == 0)
        {
            MouseValidationStartDistance = FVector::Distance(Mean, MouseTarget);
        }
        else
        {
            const double EndDistance = FVector::Distance(Mean, MouseTarget);
            bValid &= EndDistance < MouseValidationStartDistance - 80.0;
            UE_LOG(LogCubeFlock, Display, TEXT("MOUSE_FOLLOW_LEG_%d_%s: target=%s startDistance=%.1f endDistance=%.1f count=%d"),
                (MouseValidationPhase + 1) / 2, bValid ? TEXT("PASSED") : TEXT("FAILED"),
                *MouseTarget.ToCompactString(), MouseValidationStartDistance, EndDistance, ActiveCount);
        }
        if (!bValid)
        {
            UE_LOG(LogCubeFlock, Error, TEXT("MOUSE_FOLLOW_VALIDATION_FAILED phase=%d"), MouseValidationPhase);
            FPlatformMisc::RequestExitWithStatus(false, 1);
        }
        ++MouseValidationPhase;
        if (MouseValidationPhase == 4)
        {
            UE_LOG(LogCubeFlock, Display, TEXT("MOUSE_FOLLOW_VALIDATION_PASSED: both screen targets approached on GPU"));
            FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots/CubeFlockMouse.png")), true, false);
        }
    }
    if (MouseValidationPhase == 4 && SimulationTime > 14.0f) FPlatformMisc::RequestExit(false);
}

