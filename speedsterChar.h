// speedsterChar.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "speedsterChar.generated.h"

// Forward declarations
class UNiagaraComponent;
class UMaterialInterface;
class UCurveFloat;
class UMaterialParameterCollection;
class UMaterialInstanceDynamic;

// Struct for Ghost Trail Data
USTRUCT()
struct FGhostTrailData
{
    GENERATED_BODY()

    UPROPERTY()
    USkeletalMeshComponent* GhostMeshComponent;

    UPROPERTY()
    float ElapsedTime;
};

UCLASS()
class YOURGAME_API AspeedsterChar : public ACharacter
{
    GENERATED_BODY()

public:
    // Constructor
    AspeedsterChar();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Movement properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float InitialSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SpeedIncreaseRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float Acceleration;

    // Current speed (internal)
    float CurrentSpeed;

    // Wall running properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
    float WallDetectionDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
    float WallRunGravityScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
    float WallRunSpeedMultiplier;

    // Flags
    bool bIsWallRunning;

    // Combat properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float ZipSpeed;

    // Functions
    void IncreaseSpeed(float DeltaTime);
    void CheckForWall();
    void StartWallRun(const FVector& WallNormal);
    void StopWallRun();
    void PerformAttack();

    // Input functions
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartJump();
    void StopJump();
    void OnAttackPressed();

    // Lightning Trail Effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UNiagaraSystem* LightningTrailSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
    UNiagaraComponent* LightningTrailComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    float LightningActivationSpeed;

    // Mirage/Distortion Effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UMaterialInterface* DistortionMaterial;

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicDistortionMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    float DistortionEffectDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UCurveFloat* DistortionIntensityCurve;

    // Timer handle for distortion effect
    FTimerHandle DistortionEffectTimerHandle;
    float DistortionEffectElapsedTime;
    bool bIsDistortionEffectActive;

    // Ghost Trail Effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Trail")
    UMaterialInterface* GhostMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Trail")
    int32 MaxGhosts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Trail")
    float GhostSpawnInterval;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Trail")
    float GhostLifetime;

    // Internal variables for ghost trail
    float GhostSpawnTimer;
    TArray<FGhostTrailData> Ghosts;
    bool bGhostTrailActive;

    // Speed threshold for activating effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    float EffectActivationSpeed;

    // Post-process component for distortion effect
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
    UPostProcessComponent* PostProcessComponent;

    // Niagara Component for Speed Aura Effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UNiagaraSystem* SpeedAuraSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
    UNiagaraComponent* SpeedAuraComponent;

    // Flags for effects
    bool bEffectsActive;

    // Functions for effects
    void StartDistortionEffect();
    void UpdateDistortionEffect();
    void ActivateSpeedEffects();
    void DeactivateSpeedEffects();
    void ActivateGhostTrail();
    void DeactivateGhostTrail();
    void UpdateGhostTrail(float DeltaTime);
    void SpawnGhost();

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Setup player input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
