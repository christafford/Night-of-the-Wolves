#include "WolfTrailHUD.h"
#include "Night_of_the_WolvesPlayerController.h"
#include "WolfTrailGameMode.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#if !UE_BUILD_SHIPPING
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#endif
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor Linen(.91f,.90f,.79f);
const FLinearColor Amber(.99f,.66f,.29f);
const FLinearColor Mist(.59f,.69f,.70f);
const FLinearColor Ink(.025f,.042f,.047f,.91f);
}

AWolfTrailHUD::AWolfTrailHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Wolf(TEXT("/Game/NightOfTheWolves/Presentation/wolf_attack"));
	WolfAttackTexture = Wolf.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Title(TEXT("/Game/NightOfTheWolves/Presentation/T_OpeningTitle"));
	OpeningTitleTexture = Title.Object;
}

void AWolfTrailHUD::DrawOpeningTitle(float Opacity)
{
	if (Opacity <= 0.f) return;
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	if (W <= 0.f || H <= 0.f) return;

	const FLinearColor Matte(0.f, 0.f, 0.f, Opacity);
	if (!OpeningTitleTexture)
	{
		DrawRect(Matte, 0, 0, W, H);
		const float Scale = FMath::Min(W / 1280.f, H / 720.f);
		const FLinearColor LetterColor(.9f, .89f, .82f, Opacity);
		CenterText(TEXT("Night of the Wolves"), W * .5f, H * .44f, 2.8f * Scale, LetterColor);
		CenterText(TEXT("A Stafford Laboratory Production"), W * .5f, H * .61f, 1.2f * Scale, LetterColor);
		return;
	}

	// Crop the forest to fit common aspect ratios. At extreme ratios, only
	// the surplus region receives a film matte, so each pixel dissolves once.
	const float Aspect = FMath::Clamp(W / H, 1.22f, 2.44f);
	const float FrameW = FMath::Min(W, H * Aspect);
	const float FrameH = FrameW / Aspect;
	const float Left = (W - FrameW) * .5f;
	const float Top = (H - FrameH) * .5f;
	if (Left > .01f)
	{
		DrawRect(Matte, 0, 0, Left, H);
		DrawRect(Matte, W - Left, 0, Left, H);
	}
	if (Top > .01f)
	{
		DrawRect(Matte, 0, 0, W, Top);
		DrawRect(Matte, 0, H - Top, W, Top);
	}

	const float ArtAspect = static_cast<float>(OpeningTitleTexture->GetSizeX()) / OpeningTitleTexture->GetSizeY();
	const float U = FMath::Min(1.f, Aspect / ArtAspect);
	const float V = FMath::Min(1.f, ArtAspect / Aspect);
	DrawTexture(OpeningTitleTexture, Left, Top, FrameW, FrameH, (1.f-U)*.5f, (1.f-V)*.5f, U, V,
		FLinearColor(1.f, 1.f, 1.f, Opacity), BLEND_Translucent);
}

void AWolfTrailHUD::CenterText(const FString& Str, float X, float Y, float Size, FLinearColor Color)
{
	float W,H;
	GetTextSize(Str,W,H,GEngine->GetMediumFont(),Size);
	DrawText(Str,Color,X-W*.5f,Y-H*.5f,GEngine->GetMediumFont(),Size);
}

void AWolfTrailHUD::Ring(FVector2D At,float R,FLinearColor C,float Width)
{
	for (int32 i=0;i<64;++i)
	{
		float A=UE_TWO_PI*i/64, B=UE_TWO_PI*(i+1)/64;
		DrawLine(At.X+R*FMath::Cos(A),At.Y+R*FMath::Sin(A),At.X+R*FMath::Cos(B),At.Y+R*FMath::Sin(B),C,Width);
	}
}

void AWolfTrailHUD::Icon(FName Item,FVector2D At,float Size,FLinearColor C)
{
	auto Line = [&](std::initializer_list<FVector2D> Points)
	{
		const FVector2D* Last = nullptr;
		for (const auto& P : Points)
		{
			if (Last) DrawLine(At.X+Last->X*Size,At.Y+Last->Y*Size,At.X+P.X*Size,At.Y+P.Y*Size,C,FMath::Max(1.,Size*.085));
			Last=&P;
		}
	};
	if (Item == TEXT("Shovel"))
	{
		Line({{-.7,.8},{-.8,.35},{-.46,.0},{-.12,.31},{-.45,.67},{-.7,.8}});
		Line({{-.28,.15},{.46,-.56},{.39,-.8},{.54,-.93},{.84,-.64},{.69,-.5},{.46,-.56}});
	}
	else if (Item == TEXT("Rope"))
	{
		for (int i=0;i<3;++i) Ring(At+FVector2D(i*.14-.16,0)*Size,Size*.55,C,Size*.055);
		Line({{.55,.1},{.81,.35},{.75,.69},{.91,.77}});
	}
	else if (Item == TEXT("Gold"))
	{
		Line({{-.4,-.85},{.35,-.8},{.15,-.44},{.62,.12},{.65,.54},{.4,.75},{-.4,.75},{-.67,.48},{-.59,.05},{-.17,-.43},{-.4,-.85}});
		Line({{-.26,-.38},{.29,-.38},{.51,-.16}});
		Ring(At+FVector2D(.02,.26)*Size,Size*.24,C,Size*.06);
	}
	else if (Item == TEXT("Firebrand"))
	{
		Line({{-.47,.87},{-.31,.94},{.23,-.25},{-.04,-.35},{-.47,.87}});
		Line({{-.04,-.35},{-.19,-.71},{.02,-.59},{.18,-1.0},{.26,-.71},{.47,-.87},{.39,-.43},{.23,-.25}});
	}
	else if (Item.IsNone())
	{
		Line({{-.43,.58},{-.82,-.1},{-.71,-.22},{-.36,.05},{-.53,-.58},{-.39,-.67},{-.16,-.17},{-.24,-.82},{-.08,-.82},{.04,-.23},{.13,-.83},{.28,-.76},{.25,-.19},{.43,-.56},{.58,-.47},{.47,.20},{.17,.68},{-.25,.72},{-.43,.58}});
	}
	else Ring(At,Size*.57,C,Size*.08);
}

void AWolfTrailHUD::Wheel(ANight_of_the_WolvesPlayerController& PC,const FWolfInventory& Bag)
{
	PC.UpdatePointerSelection();
	const float W=Canvas->SizeX,H=Canvas->SizeY;
	const float S=FMath::Clamp(H/780.f,.65f,1.7f);
	const FVector2D Center(W*.5f,H*.5f);
	const float R=FMath::Min(H*.235f,W*.2f);
	const int32 Start=PC.WheelPage()*PC.SlotsPerPage;
	const int32 N=FMath::Min(PC.SlotsPerPage,Bag.Items.Num()+1-Start);
	DrawRect(FLinearColor(.008,.019,.023,.91),0,0,W,H);
	CenterText(TEXT("INVENTORY"),Center.X,H*.13,1.65*S,Linen);
	Ring(Center,R,FLinearColor(.23,.30,.31,.8),1.5*S);
	for (int32 i=0;i<N;++i)
	{
		const int32 Index=Start+i;
		const FName Item=Index==0 ? NAME_None : Bag.Items[Index-1];
		const float A=UE_TWO_PI*i/N;
		FVector2D At=Center+FVector2D(FMath::Sin(A),-FMath::Cos(A))*R;
		const bool Selected=Index==PC.Highlight();
		const float Radius=S*29;
		FCanvasNGonItem Face(At,FVector2D(Radius),32,Ink);
		Canvas->DrawItem(Face);
		Ring(At,Radius,Selected?Amber:Mist,(Selected?2.6:1.2)*S);
		Icon(Item,At,16*S,Selected?Amber:Linen);
		if (Bag.Equipped==Item) DrawRect(Amber,At.X-5*S,At.Y+Radius-4*S,10*S,2*S);
	}
	const int32 Index=PC.Highlight();
	const FName Item=Index==0 ? NAME_None : Bag.Items[Index-1];
	CenterText(Bag.DisplayName(Item),Center.X,Center.Y,1.25*S,Linen);
	CenterText(Bag.Equipped==Item?TEXT("EQUIPPED"):TEXT("STORED"),Center.X,Center.Y+28*S,.73*S,Mist);
	if (Bag.bBurning && Item!=Bag.Equipped)
		CenterText(TEXT("Switching items will extinguish the firebrand."),Center.X,H*.84,.95*S,Amber);
	const bool Paged=Bag.Items.Num()+1>PC.SlotsPerPage;
	const FString Paging=Paged?FString::Printf(TEXT("    Page %d/%d  %s"),PC.WheelPage()+1,FMath::DivideAndRoundUp(Bag.Items.Num()+1,PC.SlotsPerPage),PC.UsingPad()?TEXT("RB"):TEXT("PgUp / PgDn")):FString();
	CenterText((PC.UsingPad()?FString(TEXT("D-pad  Select     A  Equip     B  Cancel")):FString(TEXT("Point or scroll to select     Click / Enter  Equip     Esc  Cancel")))+Paging,Center.X,H*.93,.9*S,Mist);
}

void AWolfTrailHUD::DrawHUD()
{
	Super::DrawHUD();
	auto* Mode=GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	auto* PC=Cast<ANight_of_the_WolvesPlayerController>(PlayerOwner);
	if (!Mode || !PC || !Canvas) return;
	if (Mode->IsOpeningCinematicPlaying())
	{
		DrawOpeningTitle(Mode->GetOpeningTitleOpacity());
		return;
	}
	const float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Clamp(H/780.f,.65f,1.7f);
	#if !UE_BUILD_SHIPPING
	static const bool bReview=FParse::Param(FCommandLine::Get(),TEXT("FirstLegPhotos"));
	if (bReview && PC->GetPawn())
	{
		const FVector P=PC->GetPawn()->GetActorLocation();
		const FString& Prompt=Mode->InteractionPrompt;
		const int32 CP=Mode->Checkpoint;
		FString Shot;
		if (GetWorld()->GetTimeSeconds()>1 && CP==0 && P.X < -5500) Shot=TEXT("01-Trail");
		if (CP==1 && P.Y < -600) Shot=TEXT("02-Turnoff");
		if (CP==2 && Mode->Inventory.bBurning && P.X < 2940 && P.X>2700 && PC->GetOverlay()==ANight_of_the_WolvesPlayerController::EOverlay::None) Shot=TEXT("03-Hearth");
		if (CP==3 && Mode->Inventory.bBurning && PC->GetOverlay()==ANight_of_the_WolvesPlayerController::EOverlay::None) Shot=TEXT("04-Creek");
		if (CP==3 && Mode->Inventory.bBurning && PC->GetOverlay()==ANight_of_the_WolvesPlayerController::EOverlay::Inventory && PC->Highlight()!=Mode->Inventory.Items.Find(TEXT("Firebrand"))+1) Shot=TEXT("05-Inventory");
		if (CP==3 && Mode->IsTraversalLocked() && P.X>10100) Shot=TEXT("06-Cableway");
		if (CP==4 && Prompt.Contains(TEXT("belongings"))) Shot=TEXT("07-Bandit");
		if (CP==4 && Prompt.Contains(TEXT("Throw gold"))) Shot=TEXT("08-Gold");
		if (CP==5) Shot=TEXT("09-Bend");
		if (!Shot.IsEmpty() && !ReviewShots.Contains(Shot) && !FScreenshotRequest::IsScreenshotRequested())
		{
			const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("FirstLegReview"));
			IFileManager::Get().MakeDirectory(*Folder,true);
			FScreenshotRequest::RequestScreenshot(Folder/(Shot+TEXT(".png")),false,false);
			ReviewShots.Add(Shot);
		}
	}
#endif
	auto Caption=[&](FString Message,float Y,FLinearColor C,float Scale)
	{
		if (Message.IsEmpty()) return;
		if (PC->UsingPad())
		{
			Message.ReplaceInline(TEXT("Press E"),TEXT("Press X"),ESearchCase::CaseSensitive);
			Message.ReplaceInline(TEXT("Press F"),TEXT("Press RT"),ESearchCase::CaseSensitive);
			Message.ReplaceInline(TEXT("E  "),TEXT("X  "),ESearchCase::CaseSensitive);
			Message.ReplaceInline(TEXT("F  "),TEXT("RT  "),ESearchCase::CaseSensitive);
			Message.ReplaceInline(TEXT("Tab  "),TEXT("LB  "),ESearchCase::CaseSensitive);
			Message.ReplaceInline(TEXT("W A S D  Move      Shift  Sprint"),TEXT("Left stick  Move      L3  Sprint"));
			Message.ReplaceInline(TEXT("[E]"),TEXT("[X]"));
			Message.ReplaceInline(TEXT("[F]"),TEXT("[RT]"));
		}
		float Tw,Th; GetTextSize(Message,Tw,Th,GEngine->GetMediumFont(),Scale*S);
		const float Fit=FMath::Min(1.f,(W-40*S)/FMath::Max(Tw,1.f));
		DrawRect(Ink,(W-Tw*Fit)*.5f-13*S,Y-Th*Fit*.5f-6*S,Tw*Fit+26*S,Th*Fit+12*S);
		CenterText(Message,W*.5f,Y,Scale*S*Fit,C);
	};
	if (PC->GetOverlay()==ANight_of_the_WolvesPlayerController::EOverlay::Inventory) { Wheel(*PC,Mode->Inventory); return; }
	if (PC->GetOverlay()==ANight_of_the_WolvesPlayerController::EOverlay::ConfirmNew)
	{
		DrawRect(Ink,0,0,W,H);
		CenterText(TEXT("Start again at the trailhead?"),W*.5,H*.43,1.5*S,Linen);
		CenterText(TEXT("Your current checkpoint will be cleared."),W*.5,H*.51,S,Mist);
		CenterText(PC->UsingPad()?TEXT("A   Start again       B   Back"):TEXT("Enter   Start again       Esc   Back"),W*.5,H*.61,S,Amber);
		return;
	}
	if (PC->GetOverlay()==ANight_of_the_WolvesPlayerController::EOverlay::Pause)
	{
		DrawRect(Ink,0,0,W,H);
		CenterText(TEXT("Paused"),W*.5,H*.42,2*S,Linen);
		CenterText(PC->UsingPad()?TEXT("A   Resume       Y   Retry checkpoint       X   New trail"):TEXT("Enter   Resume       R   Retry checkpoint       N   New trail"),W*.5,H*.56,S,Amber);
		return;
	}
	if (Mode->IsDead())
	{
		if (Mode->IsWolfPouncing() || Mode->IsRiverDrowning()) return;
		DrawRect(FLinearColor(.015,.015,.019,.87),0,0,W,H);
		if (Mode->bWolfCaptured && WolfAttackTexture)
		{
			float Alpha=FMath::Clamp((GetWorld()->GetTimeSeconds()-Mode->WolfCaptureTime)/.45f,0.f,1.f);
			float Size=H*FMath::Lerp(.45f,1.45f,Alpha);
			float Width=Size*WolfAttackTexture->GetSizeX()/WolfAttackTexture->GetSizeY();
			DrawTexture(WolfAttackTexture,(W-Width)*.5,(H-Size)*.5,Width,Size,0,0,1,1,FLinearColor::White,BLEND_Translucent);
		}
		CenterText(TEXT("You died"),W*.5,H*.49,2.5*S,Linen);
		Caption(Mode->DeathCause,H*.59,Mist,.95f);
		return;
	}
	if (Mode->IsWatchingCreekPack()) return;
	Caption(Mode->InteractionPrompt,H*.77,Linen,1.08f);
	Caption(Mode->ThreatCue,H*.12,Mist,.95f);
	if (PC->NoticeUntil > GetWorld()->GetTimeSeconds()) Caption(PC->ItemNotice(),H*.85,Amber,.85f);
	if (Mode->Checkpoint>0 || !Mode->Inventory.Items.IsEmpty())
	{
		FString Held=Mode->Inventory.DisplayName(Mode->Inventory.Equipped);
		DrawText((PC->UsingPad()?TEXT("LB   "):TEXT("Tab   "))+Held,Linen,W*.035,H*.92,GEngine->GetMediumFont(),.85*S);
	}
}
