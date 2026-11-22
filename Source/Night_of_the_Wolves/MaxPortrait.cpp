// Capture the running character for art review on the trail, including his actual idle pose.
#if !UE_BUILD_SHIPPING
#include "WolfCottageSet.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/PointLight.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
void PhotographMax(UWorld* World)
{
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    ACharacter* Max = PC ? PC->GetCharacter() : nullptr;
    if (!Max || World->WorldType != EWorldType::Game) return;

    const FVector Forward = Max->GetActorForwardVector();
    const FVector Right = Max->GetActorRightVector();
    const FVector Centre = Max->GetActorLocation();
    auto* Camera = World->SpawnActor<ACameraActor>();
    // The review key and rim lights exist only in this single-use review session.
    auto Light = [&](FVector Offset, FLinearColor Color, float Intensity)
    {
        auto* Lamp = World->SpawnActor<APointLight>();
        auto* Source = Lamp->PointLightComponent.Get();
        Source->SetMobility(EComponentMobility::Movable);
        Lamp->SetActorLocation(Centre + Offset);
        Source->SetLightColor(Color);
        Source->SetIntensityUnits(ELightUnits::Candelas);
        Source->SetAttenuationRadius(550.f);
        Source->SetSourceRadius(28.f);
        Source->SetIntensity(Intensity);
    };
    Light(Forward*145 - Right*96 + FVector(0,0,137), FLinearColor(1.f,.92f,.86f), 3.4f);
    Light(-Forward*52 + Right*100 + FVector(0,0,155), FLinearColor(.58f,.72f,1.f), 4.2f);
    Camera->GetCameraComponent()->SetFieldOfView(34);
    // Keep the approaching pack out of the three wardrobe inspection angles.
    for (TActorIterator<AWolfCottageSet> It(World); It; ++It) PC->HiddenActors.Add(*It);
    PC->SetViewTarget(Camera);
    if (PC->GetHUD()) PC->GetHUD()->bShowHUD = false;

    const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("MaxReview"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    auto Later = [World](float Delay, TFunction<void()>&& Callback)
    {
        FTimerHandle Timer;
        World->GetTimerManager().SetTimer(Timer, MoveTemp(Callback), Delay, false);
    };
    Later(.18f, [Camera, Max, Forward, Right]()
    {
        // The body head socket follows locomotion at every LOD. A facial
        // landmark may be stripped from the active render LOD.
        const FVector Focus = Max->GetMesh()->GetSocketLocation(TEXT("head")) + FVector(0,0,4);
        const FVector Close = Focus + Forward*109 - Right*67 + FVector(0,0,7);
        Camera->SetActorLocationAndRotation(Close, (Focus - Close).Rotation());
    });
    Later(1.f, [Folder]() { FScreenshotRequest::RequestScreenshot(Folder / TEXT("Portrait.png"),false,false); });
    Later(1.3f, [Camera, Centre, Forward, Right]()
    {
        Camera->GetCameraComponent()->SetFieldOfView(42);
        const FVector Wide = Centre + Forward*464 - Right*90 + FVector(0,0,20);
        Camera->SetActorLocationAndRotation(Wide, (Centre - Wide).Rotation());
    });
    Later(2.5f, [Folder]() { FScreenshotRequest::RequestScreenshot(Folder / TEXT("Outfit.png"),false,false); });
    Later(2.8f, [Camera, Centre, Forward, Right]()
    {
        const FVector Rear = Centre - Forward*425 + Right*128 + FVector(0,0,17);
        Camera->SetActorLocationAndRotation(Rear, (Centre - Rear).Rotation());
    });
    Later(3.8f, [Folder]() { FScreenshotRequest::RequestScreenshot(Folder / TEXT("Back.png"),false,false); });
    Later(4.2f, []() { FPlatformMisc::RequestExit(false); });
}
FAutoConsoleCommandWithWorld MaxPortrait(TEXT("Max.Portrait"),
    TEXT("Photograph Max on the trail with review lighting, then exit. Use -NoTrailSave."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&PhotographMax));
}
#endif
