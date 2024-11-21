// MySpeedsterCharacter.cpp

#include "MySpeedsterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/PostProcessComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

AMySpeedsterCharacter::AMySpeedsterCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Initialize movement properties
    InitialSpeed = 600.f;
    MaxSpeed = 2000.f;
    SpeedIncreaseRate = 50.f;
    Acceleration = 500.f;
    CurrentSpeed = InitialSpeed;

    // Initialize wall running properties
    WallDetectionDistance = 100.f;
    WallRunGravityScale = 0.f;
    WallRunSpeedMultiplier = 1.2f;

    bIsWallRunning = false;

    // Initialize combat properties
    ZipSpeed = 1500.f;

    // Initialize effect properties
    LightningActivationSpeed = 1000.f;
    DistortionEffectDuration = 0.5f;
    bIsDistortionEffectActive = false;

    // Initialize ghost trail properties
    MaxGhosts = 12;
    GhostSpawnInterval = 0.05f;
    GhostLifetime = 0.6f;
    GhostSpawnTimer = 0.0f;
    bGhostTrailActive = false;

    EffectActivationSpeed = 800.f;
    bEffectsActive = false;

    // Initialize components
    LightningTrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LightningTrail"));
    LightningTrailComponent->SetupAttachment(GetRootComponent());
    LightningTrailComponent->SetAutoActivate(false);

    SpeedAuraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SpeedAura"));
    SpeedAuraComponent->SetupAttachment(GetRootComponent());
    SpeedAuraComponent->SetAutoActivate(false);

    PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
    PostProcessComponent->bUnbound = true;
    PostProcessComponent->SetupAttachment(RootComponent);
}

void AMySpeedsterCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Assign Lightning Trail System
    if (LightningTrailSystem)
    {
        LightningTrailComponent->SetAsset(LightningTrailSystem);
    }

    // Assign Speed Aura System
    if (SpeedAuraSystem)
    {
        SpeedAuraComponent->SetAsset(SpeedAuraSystem);
    }

    // Create dynamic material instance for distortion effect
    if (DistortionMaterial)
    {
        DynamicDistortionMaterial = UMaterialInstanceDynamic::Create(DistortionMaterial, this);
        PostProcessComponent->AddOrUpdateBlendable(DynamicDistortionMaterial);
    }

    // Ensure the GhostMaterial is set
    if (!GhostMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("GhostMaterial is not set on %s"), *GetName());
    }

    // Set initial movement speed
    GetCharacterMovement()->MaxWalkSpeed = InitialSpeed;
}

void AMySpeedsterCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Increase speed over time
    IncreaseSpeed(DeltaTime);

    // Check for wall running
    if (GetCharacterMovement()->IsFalling())
    {
        CheckForWall();
    }

    // Handle effects based on speed
    float CurrentSpeedValue = GetVelocity().Size();

    // Lightning Trail Effect
    if (CurrentSpeedValue > LightningActivationSpeed)
    {
        if (!LightningTrailComponent->IsActive())
        {
            LightningTrailComponent->Activate();
        }
    }
    else
    {
        if (LightningTrailComponent->IsActive())
        {
            LightningTrailComponent->Deactivate();
        }
    }

    // Speed Aura and Ghost Trail Effects
    bool bShouldActivateEffects = CurrentSpeedValue > EffectActivationSpeed;

    if (bShouldActivateEffects && !bEffectsActive)
    {
        ActivateSpeedEffects();
    }
    else if (!bShouldActivateEffects && bEffectsActive)
    {
        DeactivateSpeedEffects();
    }

    // Update Ghost Trail
    if (bGhostTrailActive)
    {
        UpdateGhostTrail(DeltaTime);
    }
}

void AMySpeedsterCharacter::IncreaseSpeed(float DeltaTime)
{
    CurrentSpeed += SpeedIncreaseRate * DeltaTime;
    CurrentSpeed = FMath::Clamp(CurrentSpeed, InitialSpeed, MaxSpeed);
    GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;

    // Optionally adjust Field of View (FOV) or add visual effects here
}

void AMySpeedsterCharacter::CheckForWall()
{
    FVector Start = GetActorLocation();
    FVector ForwardDirection = GetActorForwardVector();
    FVector End = Start + (ForwardDirection * WallDetectionDistance);

    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);

    // Perform line trace to detect walls
    bool bHitWall = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility,
        CollisionParams
    );

    if (bHitWall)
    {
        FVector WallNormal = HitResult.Normal;
        float WallAngle = FVector::DotProduct(WallNormal, FVector::UpVector);

        // Check if the wall is suitable for wall running
        if (WallAngle < 0.1f)
        {
            StartWallRun(WallNormal);
        }
    }
    else if (bIsWallRunning)
    {
        StopWallRun();
    }
}

void AMySpeedsterCharacter::StartWallRun(const FVector& WallNormal)
{
    if (!bIsWallRunning)
    {
        bIsWallRunning = true;

        // Adjust gravity
        GetCharacterMovement()->GravityScale = WallRunGravityScale;

        // Adjust character orientation to align with the wall
        FRotator WallRotation = UKismetMathLibrary::MakeRotFromX(-WallNormal);
        SetActorRotation(WallRotation);

        // Optionally lock player input or adjust controls here
    }

    // Move the character along the wall
    FVector WallRunDirection = FVector::CrossProduct(WallNormal, FVector::UpVector);
    AddMovementInput(WallRunDirection, WallRunSpeedMultiplier);
}

void AMySpeedsterCharacter::StopWallRun()
{
    bIsWallRunning = false;

    // Restore gravity
    GetCharacterMovement()->GravityScale = 1.0f;

    // Optionally reset character orientation here
}

void AMySpeedsterCharacter::PerformAttack()
{
    // Define the range and radius for detecting enemies
    float AttackRange = 1000.f;
    float AttackRadius = 200.f;

    FVector StartLocation = GetActorLocation();
    FVector ForwardVector = GetActorForwardVector();
    FVector EndLocation = StartLocation + (ForwardVector * AttackRange);

    TArray<FHitResult> HitResults;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(AttackRadius);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    // Perform a sphere trace to find potential targets
    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        StartLocation,
        EndLocation,
        FQuat::Identity,
        ECC_Pawn,
        CollisionShape,
        QueryParams
    );

    if (bHit)
    {
        // Find the nearest target in the player's facing direction
        AActor* NearestEnemy = nullptr;
        float MinDistance = FLT_MAX;

        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor->Tags.Contains("Enemy"))
            {
                float Distance = FVector::DistSquared(GetActorLocation(), HitActor->GetActorLocation());
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    NearestEnemy = HitActor;
                }
            }
        }

        if (NearestEnemy)
        {
            // Zip to the enemy
            FVector Direction = (NearestEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            LaunchCharacter(Direction * ZipSpeed, true, true);

            // Apply damage to the enemy
            UGameplayStatics::ApplyDamage(NearestEnemy, 50.f, GetController(), this, nullptr);
        }
    }
}

void AMySpeedsterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Axis bindings
    PlayerInputComponent->BindAxis("MoveForward", this, &AMySpeedsterCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AMySpeedsterCharacter::MoveRight);

    // Action bindings
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMySpeedsterCharacter::StartJump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMySpeedsterCharacter::StopJump);
    PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AMySpeedsterCharacter::OnAttackPressed);
}

void AMySpeedsterCharacter::MoveForward(float Value)
{
    if (Controller && Value != 0.0f)
    {
        // Get forward direction
        FRotator Rotation = Controller->GetControlRotation();
        FRotator YawRotation(0, Rotation.Yaw, 0);
        FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // Move character
        AddMovementInput(Direction, Value);
    }
}

void AMySpeedsterCharacter::MoveRight(float Value)
{
    if (Controller && Value != 0.0f)
    {
        // Get right direction
        FRotator Rotation = Controller->GetControlRotation();
        FRotator YawRotation(0, Rotation.Yaw, 0);
        FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // Move character
        AddMovementInput(Direction, Value);
    }
}

void AMySpeedsterCharacter::StartJump()
{
    Jump();
}

void AMySpeedsterCharacter::StopJump()
{
    StopJumping();
}

void AMySpeedsterCharacter::OnAttackPressed()
{
    PerformAttack();
}

void AMySpeedsterCharacter::StartDistortionEffect()
{
    if (bIsDistortionEffectActive)
    {
        return; // Prevent overlapping effects
    }

    bIsDistortionEffectActive = true;
    DistortionEffectElapsedTime = 0.0f;

    // Get the character's screen position
    FVector2D ScreenPosition;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC && PC->ProjectWorldLocationToScreen(GetActorLocation(), ScreenPosition))
    {
        // Get viewport size
        int32 ViewportX, ViewportY;
        PC->GetViewportSize(ViewportX, ViewportY);

        // Normalize screen position to [0,1] range
        FVector2D EffectCenter(ScreenPosition.X / ViewportX, ScreenPosition.Y / ViewportY);

        // Set EffectCenter parameter
        DynamicDistortionMaterial->SetVectorParameterValue(FName("EffectCenter"), FLinearColor(EffectCenter.X, EffectCenter.Y, 0, 0));
    }

    // Start timer to update the effect
    GetWorldTimerManager().SetTimer(DistortionEffectTimerHandle, this, &AMySpeedsterCharacter::UpdateDistortionEffect, 0.01f, true);
}

void AMySpeedsterCharacter::UpdateDistortionEffect()
{
    DistortionEffectElapsedTime += 0.01f; // Increment time by timer interval

    if (DistortionEffectElapsedTime >= DistortionEffectDuration)
    {
        // Stop the effect
        GetWorldTimerManager().ClearTimer(DistortionEffectTimerHandle);
        DynamicDistortionMaterial->SetScalarParameterValue(FName("DistortionStrength"), 0.0f);
        bIsDistortionEffectActive = false;
        return;
    }

    // Get distortion strength from the curve
    float DistortionStrength = DistortionIntensityCurve->GetFloatValue(DistortionEffectElapsedTime / DistortionEffectDuration);

    // Update the material parameter
    DynamicDistortionMaterial->SetScalarParameterValue(FName("DistortionStrength"), DistortionStrength);
}

void AMySpeedsterCharacter::ActivateSpeedEffects()
{
    bEffectsActive = true;
    SpeedAuraComponent->Activate();
    ActivateGhostTrail();
    StartDistortionEffect();
}

void AMySpeedsterCharacter::DeactivateSpeedEffects()
{
    bEffectsActive = false;
    SpeedAuraComponent->Deactivate();
    DeactivateGhostTrail();
    StartDistortionEffect(); // Trigger distortion effect when stopping
}

void AMySpeedsterCharacter::ActivateGhostTrail()
{
    bGhostTrailActive = true;
    GhostSpawnTimer = 0.0f;
}

void AMySpeedsterCharacter::DeactivateGhostTrail()
{
    bGhostTrailActive = false;

    // Clean up existing ghosts
    for (FGhostTrailData& GhostData : Ghosts)
    {
        if (GhostData.GhostMeshComponent)
        {
            GhostData.GhostMeshComponent->DestroyComponent();
        }
    }
    Ghosts.Empty();
}

void AMySpeedsterCharacter::UpdateGhostTrail(float DeltaTime)
{
    GhostSpawnTimer += DeltaTime;

    // Spawn new ghost if interval reached
    if (GhostSpawnTimer >= GhostSpawnInterval)
    {
        SpawnGhost();
        GhostSpawnTimer = 0.0f;
    }

    // Update existing ghosts
    for (int32 i = Ghosts.Num() - 1; i >= 0; --i)
    {
        FGhostTrailData& GhostData = Ghosts[i];
        GhostData.ElapsedTime += DeltaTime;

        // Calculate new opacity
        float LifeRatio = GhostData.ElapsedTime / GhostLifetime;
        float NewOpacity = FMath::Clamp(1.0f - LifeRatio, 0.0f, 1.0f);

        // Update material opacity
        UMaterialInstanceDynamic* GhostMID = Cast<UMaterialInstanceDynamic>(GhostData.GhostMeshComponent->GetMaterial(0));
        if (GhostMID)
        {
            GhostMID->SetScalarParameterValue(TEXT("GhostOpacity"), NewOpacity);
        }

        // Remove ghost if lifetime exceeded
        if (GhostData.ElapsedTime >= GhostLifetime)
        {
            GhostData.GhostMeshComponent->DestroyComponent();
            Ghosts.RemoveAt(i);
        }
    }
}

void AMySpeedsterCharacter::SpawnGhost()
{
    if (!GetMesh())
    {
        return;
    }

    // Create a new Skeletal Mesh Component
    USkeletalMeshComponent* GhostMesh = NewObject<USkeletalMeshComponent>(this);
    if (GhostMesh)
    {
        GhostMesh->RegisterComponent();
        GhostMesh->SetSkeletalMesh(GetMesh()->SkeletalMesh);
        GhostMesh->SetAnimInstanceClass(GetMesh()->GetAnimInstance()->GetClass());
        GhostMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

        // Copy pose
        GhostMesh->SetWorldTransform(GetMesh()->GetComponentTransform());
        GhostMesh->GetAnimInstance()->CopyPoseFromMesh(GetMesh());

        // Attach to root component
        GhostMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

        // Create a dynamic material instance
        UMaterialInstanceDynamic* GhostMID = UMaterialInstanceDynamic::Create(GhostMaterial, this);
        if (GhostMID)
        {
            GhostMID->SetScalarParameterValue(TEXT("GhostOpacity"), 1.0f);
            GhostMesh->SetMaterial(0, GhostMID);
        }

        // Disable collision
        GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        // Add to ghosts array
        FGhostTrailData NewGhost;
        NewGhost.GhostMeshComponent = GhostMesh;
        NewGhost.ElapsedTime = 0.0f;
        Ghosts.Add(NewGhost);

        // Ensure we don't exceed MaxGhosts
        if (Ghosts.Num() > MaxGhosts)
        {
            Ghosts[0].GhostMeshComponent->DestroyComponent();
            Ghosts.RemoveAt(0);
        }
    }
}
