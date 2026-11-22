#include "WolfTrailHUD.h"
#include "WolfTrailGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AWolfTrailHUD::AWolfTrailHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Wolf(TEXT("/Game/NightOfTheWolves/Presentation/wolf_attack.wolf_attack"));
	if (Wolf.Succeeded()) WolfAttackTexture = Wolf.Object;
}

void AWolfTrailHUD::DrawHUD()
{
	Super::DrawHUD();
	const AWolfTrailGameMode* Mode = Cast<AWolfTrailGameMode>(UGameplayStatics::GetGameMode(this));
	if (!Mode || !Canvas) return;
	if (Mode->bWolfCaptured || Mode->bShotByHermit)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), 0, 0, Canvas->SizeX, Canvas->SizeY);
		if (Mode->bWolfCaptured && WolfAttackTexture)
		{
			const float Elapsed = GetWorld()->GetTimeSeconds() - Mode->WolfCaptureTime;
			const float Alpha = FMath::Clamp(Elapsed / 0.45f, 0.f, 1.f);
			const float Size = Canvas->SizeY * FMath::Lerp(0.45f, 1.55f, FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f));
			const float Aspect = static_cast<float>(WolfAttackTexture->GetSizeX()) / FMath::Max(1, WolfAttackTexture->GetSizeY());
			DrawTexture(WolfAttackTexture, (Canvas->SizeX - Size * Aspect) * 0.5f, (Canvas->SizeY - Size) * 0.5f,
				Size * Aspect, Size, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		}
		const FString Message(TEXT("You died"));
		float Width, Height;
		GetTextSize(Message, Width, Height, GEngine->GetLargeFont(), 2.f);
		DrawText(Message, FLinearColor::White, (Canvas->SizeX - Width) * 0.5f, (Canvas->SizeY - Height) * 0.5f, GEngine->GetLargeFont(), 2.f);
		if (Mode->bShotByHermit) DrawText(TEXT("BANG! (placeholder)"), FLinearColor::White, 40, 80, GEngine->GetMediumFont());
	}
	else if (Mode->bSheltered)
	{
		DrawText(TEXT("Cabin secured - temporary shelter (M3)"), FLinearColor::White, 40, 80, GEngine->GetMediumFont());
	}
	else if (Mode->bHermitReacting)
	{
		DrawText(TEXT("Hermit: Huh?! (placeholder)"), FLinearColor::White, 40, 80, GEngine->GetMediumFont());
	}
	else if (!Mode->InteractionPrompt.IsEmpty())
	{
		DrawText(Mode->InteractionPrompt, FLinearColor::White, 40, 80, GEngine->GetMediumFont());
	}
	else if (Mode->bReachedTrailEnd)
	{
		DrawText(TEXT("Trail end reached - keep moving"), FLinearColor::White, 40, 40, GEngine->GetMediumFont());
	}
}
