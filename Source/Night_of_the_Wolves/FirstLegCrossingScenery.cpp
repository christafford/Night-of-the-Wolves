#include "FirstLegCrossing.h"
#include "DialogueVoiceComponent.h"
#include "DialogueFaceAnimInstance.h"
#include "WolfPackComponent.h"
#include "FirstLegCrossingLayout.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

using namespace FirstLegLayout;

namespace
{
template <typename T> T* Asset(const TCHAR* Path)
{
	ConstructorHelpers::FObjectFinder<T> Finder(Path);
	return Finder.Object;
}
}

AFirstLegCrossing::AFirstLegCrossing()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Route"));
	Cube = Asset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube"));
	Round = Asset<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere"));
	Pole = Asset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder"));
	GoldPouch = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/FirstLeg/Gold"));
	ShapeMat = Asset<UMaterialInterface>(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	Bark = Asset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Presentation/M_TreeBark"));
	Trail = Asset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Cascades/M_DampTread"));
	CreekValley = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_CreekValley"));
	StreamStoneA = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_CreekStoneA/StaticMeshes/SM_CreekStoneA"));
	StreamStoneB = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_CreekStoneB/StaticMeshes/SM_CreekStoneB"));
	StreamStoneC = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_CreekStoneC/StaticMeshes/SM_CreekStoneC"));
	NurseLog = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_NurseLog/StaticMeshes/SM_NurseLog"));
	Water = Asset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Crossing/M_RunningWater"));
	WarmGlow = Asset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Presentation/M_FireGlow"));
	MoonGlow = Asset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Presentation/M_Moon"));
	Snag = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/ForestTrees/dead_tree"));
	Fir = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_WesternFir/StaticMeshes/SM_WesternFir"));
	Pine = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_ShorePine/StaticMeshes/SM_ShorePine"));
	Fern = Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_SwordFern/StaticMeshes/SM_SwordFern"));
	Pack = CreateDefaultSubobject<UWolfPackComponent>(TEXT("HuntingPack"));
	BanditRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BanditRoot"));
	BanditRoot->SetupAttachment(RootComponent);
	Bandit = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BanditBody"));
	Bandit->SetupAttachment(BanditRoot);
	Bandit->SetRelativeRotation(FRotator(0,-90,0));
	Bandit->SetSkeletalMesh(Asset<USkeletalMesh>(TEXT("/Game/NightOfTheWolves/Max/Wardrobe/SK_MaxExposedBody")));

	UMaterialInterface* BodySkin = Asset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Bandit/MI_VillaBodySkin"));
	for (int32 Slot = 0; Slot < Bandit->GetNumMaterials(); ++Slot)
		Bandit->SetMaterial(Slot, BodySkin);

	BanditFace = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BanditFace"));
	BanditFace->SetupAttachment(Bandit);
	BanditFace->SetSkeletalMesh(Asset<USkeletalMesh>(TEXT("/Game/NightOfTheWolves/Bandit/Villa_Head")));
	BanditFace->SetAnimInstanceClass(UDialogueFaceAnimInstance::StaticClass());

	BanditClothes = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CampaignDress"));
	BanditClothes->SetupAttachment(Bandit);
	BanditClothes->SetSkeletalMesh(Asset<USkeletalMesh>(TEXT("/Game/NightOfTheWolves/Bandit/SK_BanditCampaignDress")));
	for (USkeletalMeshComponent* Part : {Bandit.Get(), BanditFace.Get(), BanditClothes.Get()})
	{
		Part->SetLightingChannels(true,false,true);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}

	auto* Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroadBrimHat"));
	Hat->SetupAttachment(BanditFace, TEXT("head"));
	Hat->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Bandit/SM_VillaHat")));
	Hat->SetLightingChannels(true,false,true);
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hat->SetCanEverAffectNavigation(false);
	BanditHair = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Animated bandit hair"));
	BanditHair->SetupAttachment(BanditFace);
	BanditHair->SetSkeletalMesh(Asset<USkeletalMesh>(TEXT("/Game/NightOfTheWolves/Bandit/SK_VillaHair")));
	BanditHair->SetLightingChannels(true,false,true);
	BanditHair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BanditHair->SetCanEverAffectNavigation(false);
	// Soft skylight under the brim also picks out the crossed leather on the shirt.
	auto* Bounce = CreateDefaultSubobject<UPointLightComponent>(TEXT("BanditSkyBounce"));
	Bounce->SetupAttachment(BanditRoot);
	Bounce->SetRelativeLocation(FVector(116,-74,181));
	Bounce->SetMobility(EComponentMobility::Movable);
	Bounce->SetLightingChannels(false,false,true);
	Bounce->SetIntensityUnits(ELightUnits::Candelas);
	Bounce->SetIntensity(6.f);
	Bounce->SetAttenuationRadius(280.f);
	Bounce->SetSourceRadius(30.f);
	Bounce->SetSoftSourceRadius(55.f);
	Bounce->SetLightColor(FLinearColor(.88f,.90f,1.f));
	Bounce->SetCastShadows(false);
	Bounce->SetIndirectLightingIntensity(0.f);
	Bounce->SetVolumetricScatteringIntensity(0.f);
	Bounce->SetAffectTranslucentLighting(false);
	Bounce->SetSpecularScale(.35f);

	AimAnim = Asset<UAnimSequence>(TEXT("/Game/NightOfTheWolves/Bandit/A_BanditDemand"));
	WalkAnim = Asset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Pistol/Walk/MF_Pistol_Walk_Fwd"));
	StoopAnim = Asset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land"));
	DeathAnim = Asset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01"));
	Shot = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/Crossing/DistantShot"));
	Coins = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/Crossing/PouchCoins"));
	Stream = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/Crossing/CreekCurrent"));
	DemandVoice = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/BanditDemand"));
	GoldVoice = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/BanditGold"));
	StayVoice = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/BanditStay"));
	Howl = Asset<USoundBase>(TEXT("/Game/NightOfTheWolves/Presentation/wolf_howls"));
}

void AFirstLegCrossing::BeginPlay()
{
	Super::BeginPlay();
	for (USoundBase* Recording : {DemandVoice.Get(), GoldVoice.Get(), StayVoice.Get()})
		UGameplayStatics::PrimeSound(Recording);
	BanditClothes->SetLeaderPoseComponent(Bandit);
	BanditHair->SetLeaderPoseComponent(BanditFace);
	BanditFace->AddTickPrerequisiteComponent(Bandit);
	auto Tint = [this](FLinearColor Color) {
		auto* Mat = UMaterialInstanceDynamic::Create(ShapeMat,this);
		Mat->SetVectorParameterValue(TEXT("Color"),Color);
		return Mat;
	};
	RopeMat=Tint({.47f,.32f,.16f});
	FoamMat=Tint({.21f,.32f,.36f});
	BlackMat=Tint({.06f,.054f,.054f});
	GoldMat=Tint({.93f,.56f,.07f});
	// Older trail maps have a fence at the former end. The large rock walls now bound the creek.
	for (TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
	{
		if (It->GetActorLocation().Equals(FVector(9000,-500,150),1.f) && It->GetActorScale3D().Equals(FVector(1,44,3),.01f))
		{
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}
	BuildBanks();
	BuildProps();
	Restore(0);
}

UStaticMeshComponent* AFirstLegCrossing::Part(UStaticMesh* Mesh, UMaterialInterface* Material, USceneComponent* Parent)
{
	auto* Component=NewObject<UStaticMeshComponent>(this);
	Component->SetStaticMesh(Mesh);
	if (Material) Component->SetMaterial(0,Material);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetupAttachment(Parent ? Parent : RootComponent.Get());
	Component->SetCanEverAffectNavigation(false);
	AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

void AFirstLegCrossing::Stamp(UStaticMesh* Mesh, UMaterialInterface* Material, FVector P, FVector S, FRotator R,bool bSolid,bool bVisible)
{
	if (!Mesh) return;
	FBatch* Batch=Batches.FindByPredicate([=](const FBatch& B){return B.Mesh==Mesh && B.Material==Material && B.Solid==bSolid && B.Visible==bVisible;});
	if (!Batch)
	{
		auto* Instances=NewObject<UInstancedStaticMeshComponent>(this);
		Instances->SetupAttachment(RootComponent);
		Instances->SetStaticMesh(Mesh);
		if (Material) Instances->SetMaterial(0,Material);
		Instances->SetVisibility(bVisible);
		Instances->SetCastShadow(bVisible);
		Instances->SetVisibleInRayTracing(bVisible);
		Instances->SetCollisionProfileName(bSolid ? TEXT("BlockAll") : TEXT("NoCollision"));
		Instances->SetCanEverAffectNavigation(false);
		if (Mesh==Fern)
		{
			Instances->SetCullDistances(3500,6000);
			Instances->SetCastShadow(false);
		}
		else if (Mesh==Fir || Mesh==Pine)
			Instances->SetCullDistances(15000,25000);
		else if (Mesh==StreamStoneA || Mesh==StreamStoneB || Mesh==StreamStoneC || Mesh==NurseLog)
			Instances->SetCullDistances(6500,11500);
		else if (Mesh==Snag)
			Instances->SetCullDistances(8000,14000);
		AddInstanceComponent(Instances);
		Instances->RegisterComponent();
		Batch=&Batches.Add_GetRef({Mesh,Material,bSolid,bVisible,Instances});
	}
	Batch->Instances->AddInstance(FTransform(R,P,S));
}

void AFirstLegCrossing::LogBeam(FVector A,FVector B,float Radius,UMaterialInterface* Material,bool bSolid)
{
	Stamp(Pole,Material,(A+B)*.5,FVector(Radius/50,Radius/50,FVector::Distance(A,B)/100),
		FRotationMatrix::MakeFromZ(B-A).Rotator(),bSolid);
}

void AFirstLegCrossing::BuildBanks()
{
	FRandomStream Random(2741);
	Stamp(CreekValley, nullptr, FVector::ZeroVector, FVector::OneVector);
	UStaticMesh* Stones[] = {StreamStoneA.Get(), StreamStoneB.Get(), StreamStoneC.Get()};
	auto Stone = [this, &Stones, &Random](FVector Position, FVector Scale, float MaxTilt = 18.f)
	{
		UStaticMesh* Mesh = Stones[Random.RandRange(0, 2)];
		Stamp(Mesh, nullptr, Position, Scale, {Random.FRandRange(-MaxTilt, MaxTilt),
			Random.FRandRange(0, 360), Random.FRandRange(-MaxTilt, MaxTilt)});
	};
	// Preserve the original walking and falling surfaces. The bank mesh follows
	// the channel continuously between the coarse collision shelves.
	for (int32 Row = -12; Row <= 12; ++Row)
	{
		const float Y = -1100 + Row * 460.f;
		const float Bend = StreamBend(Y);
		for (int32 Side : {-1, 1})
		{
			const float Edge = (Side < 0 ? 9800.f : 10900.f) + Bend;
			const float Width = Side < 0 ? 1250.f : 2400.f;
			Stamp(Cube, nullptr, {Edge + Side * Width/2, Y, -118}, {Width/100, 4.62, 3.20}, FRotator::ZeroRotator, true, false);
			Stamp(Cube, nullptr, {Edge + Side * Width/2, Y, 45}, {Width/100, 4.62, .06}, FRotator::ZeroRotator, true, false);

			// Cobble bars sit at the toes of alternating banks and break up the wet edge.
			const int32 Count = Random.RandRange(2, 6);
			for (int32 J = 0; J < Count; ++J)
			{
				const float CrossY = Y + Random.FRandRange(-175, 175);
				const float Lip = (Side < 0 ? 9800.f : 10900.f) + StreamBend(CrossY);
				const float TowardWater = Random.FRandRange(140, 255);
				const float Size = Random.FRandRange(.6, 2.0);
				Stone({Lip - Side*TowardWater, CrossY, -201 + (230-TowardWater)*.5},
					{Size, Size*1.12, Size*.78});
			}
			// Exposed roots reach from the lip down to the wash line, away from handholds.
			if ((Row + Side) % 3 == 0)
			{
				const float RootY = Y + Random.FRandRange(-130, 130);
				const FVector Top(Edge + Side*35, RootY, 44);
				const FVector Fork(Edge - Side*88, RootY+67, -6);
				const FVector Toe(Edge - Side*159, RootY+131, -51);
				LogBeam(Top, Fork, 5, Bark);
				LogBeam(Fork, Toe, 3.6, Bark);
				LogBeam(Toe, {Edge-Side*185, RootY+181, -117}, 2.0, Bark);
				LogBeam(Fork, {Edge-Side*148, RootY+21, -58}, 2.1, Bark);
			}

			for (int32 J = 0; J < 8; ++J)
			{
				const float CrossY = Y + Random.FRandRange(-210, 210);
				if (CrossY > -2750 && CrossY < 300) continue;
				const FVector P(Edge + Side*Random.FRandRange(240, 1100), CrossY, 43);
				if (J < 2)
					Stamp(J==0 ? Fir.Get() : Pine.Get(), nullptr, P, FVector(Random.FRandRange(.66,1.16)), {0,Random.FRandRange(0,360),0});
				else if (J == 2 && Row % 3 == 0)
					Stamp(NurseLog, nullptr, P, FVector(Random.FRandRange(.65,1.1)), {0,Random.FRandRange(0,360),0});
				else
					Stamp(Fern, nullptr, P, FVector(Random.FRandRange(1.0,2.0)), {0,Random.FRandRange(0,360),0});
			}
		}
		Stamp(Cube, Water, {10350+Bend,Y,-214}, {12.65,4.63,.10});
		for (int32 I=0; I<4; ++I)
		{
			const float X = 10350+Bend+Random.FRandRange(-360,360);
			const float RY = Y+Random.FRandRange(-210,210);
			const float Size = Random.FRandRange(.70,2.15);
			Stone({X,RY,-224}, {Size*.8,Size,Size*.79}, 8);
			if (I == 0)
				Stamp(Round, FoamMat, {X+4,RY+40,-207}, {.16,.61,.022}, {0,8,0});
		}
	}

	// The playable road keeps its established walkable width. Only its thin
	// top layer is drawn, allowing the river to cut through the wider foundation.
	auto RoadSegments = [this, &Random, &Stone](const FVector* Points, int32 Count)
	{
		for (int32 I=1; I<Count; ++I)
		{
			const FVector A=Points[I-1], B=Points[I], D=B-A, Center=(A+B)*.5;
			const FRotator Yaw(0,D.Rotation().Yaw,0);
			Stamp(Cube,nullptr,Center-FVector(0,0,100),{(D.Size2D()+400)/100,50,1.94},Yaw,true,false);
			Stamp(Cube,nullptr,Center-FVector(0,0,37),{(D.Size2D()+25)/100,6.15,.74},Yaw,true,false);
			Stamp(Cube,Trail,Center-FVector(0,0,1.4),{D.Size2D()/100,6.15,.028},Yaw);
			const FVector Normal=FVector::CrossProduct(D.GetSafeNormal2D(),FVector::UpVector);
			for (int32 J=0; J<18; ++J)
			{
				const int32 Side=J%2 ? 1 : -1;
				const FVector Base=FMath::Lerp(A,B,(J+.5f)/18);
				const FVector P=Base+Normal*(Side*Random.FRandRange(690,2200));
				UStaticMesh* Tree=(J%6==0) ? Snag.Get() : ((J%3==0) ? Pine.Get() : Fir.Get());
				const float Scale=Tree==Snag ? Random.FRandRange(1.2,2.6) : Random.FRandRange(.67,1.3);
				Stamp(Tree,nullptr,P-FVector(0,0,10),FVector(Scale),{0,Random.FRandRange(0,360),0});
				for (int32 F=0; F<4; ++F)
				{
					FVector Clump=Base+Normal*Side*Random.FRandRange(320,840);
					Clump.X+=Random.FRandRange(-60,60);
					Clump.Z-=6;
					Stamp(Fern,nullptr,Clump,FVector(Random.FRandRange(.7,1.6)),{0,Random.FRandRange(0,360),0});
				}
				const FVector Q=Base+Normal*(Side*Random.FRandRange(390,750));
				const FVector S(Random.FRandRange(.8,2.2),Random.FRandRange(.85,2.4),Random.FRandRange(.8,2.3));
				const FRotator R(10,Random.FRandRange(0,360),8);
				Stamp(Round,nullptr,Q-FVector(0,0,20),S,R,true,false);
				Stone(Q-FVector(0,0,8), FVector(S.X*1.32,S.Y*1.12,S.Z*1.45));
			}
		}
	};
	RoadSegments(Road,UE_ARRAY_COUNT(Road));
	RoadSegments(FarRoad,UE_ARRAY_COUNT(FarRoad));

	// Close the route with a wooded rise beyond the checkpoint. Staggered tree
	// groups leave a view into the last bend from the bandit's clearing.
	Stamp(Cube,nullptr,{16980,900,-250},{87,92,5},FRotator::ZeroRotator,true,false);
	for (int32 Wall=0; Wall<3; ++Wall)
	{
		for (int32 I=0; I<30; ++I)
		{
			const FVector P=Wall==0 ? FVector(18880,-3100+I*285,230) :
				FVector(12800+I*220,(Wall==1 ? -3050 : 4800),195);
			Stamp(Round,nullptr,P,{Random.FRandRange(4.0,5.2),Random.FRandRange(5.2,6.4),Random.FRandRange(8.8,11.3)},
				{Random.FRandRange(-12,12),Random.FRandRange(0,360),Random.FRandRange(-10,10)},true,false);
		}
	}
	for (int32 I=0; I<230; ++I)
	{
		const float X = Random.FRandRange(12300,21100);
		const float Y = Random.FRandRange(-4850,5650);
		if (X < 18800 && Y > -2300 && Y < 3900) continue;
		const float Plateau = FMath::GetMappedRangeValueClamped(FVector2D(15000,18600),FVector2D(50,130),X)-8;
		const float Rim = FMath::Clamp(FMath::Max((FMath::Abs(Y-825)-2350)/1800,(X-18150)/1250),0.f,1.65f);
		const float Z = Plateau+Rim*Rim*(360+76*FMath::Sin(Y*.0027f+X*.0014f)+60*FMath::Cos(Y*.0019f-X*.0021f));
		const FVector P(X,Y,Z-8);
		Stamp(I%3 ? Fir.Get() : Pine.Get(), nullptr, P, FVector(Random.FRandRange(.65,1.32)), {0,Random.FRandRange(0,360),0});
		if (I%6 == 0)
			Stone(P+FVector(100,70,12),FVector(Random.FRandRange(2.1,4.9)));
	}

	// The bridge remnants sit beside the swing, leaving its entire diagonal flight clear.
	for (int32 Side : {-1,1})
	{
		float End=(Side<0 ? 9780.f : 10990.f);
		for (int32 I=0;I<5;++I)
			Stamp(Cube,Bark,{End+Side*I*52.f,-1510,56},{.52,2.45,.16},{0,Random.FRandRange(-4,4),0},true);
		for (int32 BankSide : {-1,1})
		{
			float Y=-1510+BankSide*130.f;
			LogBeam({End+Side*193,Y,52},{End+Side*193,Y,154},7,Bark);
			LogBeam({End+Side*194,Y,146},{End-Side*45,Y,122},5,Bark);
			LogBeam({End+Side*70,Y,-200},{End-Side*55,Y,47},11,Bark);
		}
	}
	LogBeam({9770,-1820,0},{9890,-1690,1020},83,Bark);
	LogBeam({9890,-1690,1000},{10020,-1430,1760},52,Bark);
	LogBeam({9970,-1520,1460},Anchor,39,Bark);
	LogBeam(Anchor,Anchor+FVector(140,120,45),15,Bark);
	LogBeam({9875,-1700,980},{9670,-1980,1240},21,Bark);
	// A small auxiliary fill light keeps the landing lip readable through the trees.
	for (const FVector& P : {Landing+FVector(0,0,310),BanditMark+FVector(-80,0,290)})
	{
		auto* Light=NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent); Light->SetRelativeLocation(P);
		Light->SetIntensity(1350); Light->SetAttenuationRadius(950); Light->SetCastShadows(false);
		Light->SetLightColor(FLinearColor(.54,.7,1)); AddInstanceComponent(Light); Light->RegisterComponent();
	}
	if (Stream)
	{
		auto* Audio=NewObject<UAudioComponent>(this);
		Audio->SetupAttachment(RootComponent); Audio->SetRelativeLocation({10400,-1150,-150});
		Audio->bAutoActivate=false; Audio->bOverrideAttenuation=true;
		Audio->AttenuationOverrides.AttenuationShapeExtents=FVector(480);
		Audio->AttenuationOverrides.FalloffDistance=5400;
		Audio->SetSound(Stream); Audio->SetVolumeMultiplier(.61);
		AddInstanceComponent(Audio); Audio->RegisterComponent(); Audio->Play();
	}
}

void AFirstLegCrossing::BuildProps()
{
	Cord=Part(Pole,RopeMat);
	Bag=Part(GoldPouch,nullptr);
	for (int32 I=0;I<9;++I)
	{
		auto* Coin=Part(Pole,GoldMat); Coin->SetRelativeScale3D({.074,.074,.014}); SpilledCoins.Add(Coin);
	}
	for (int32 I=0;I<12;++I)
	{
		auto* Marker=Part(Round,MoonGlow); Marker->SetWorldScale3D({.045,.075,.048}); TargetRing.Add(Marker);
	}
	Speech=NewObject<UDialogueVoiceComponent>(this, TEXT("BanditVoice"));
	Speech->SetupAttachment(BanditRoot); Speech->SetRelativeLocation(FVector(0,0,155));
	Speech->bAutoActivate=false; Speech->bOverrideAttenuation=true;
	Speech->AttenuationOverrides.AttenuationShapeExtents=FVector(500);
	// His last order carries down the open trail as Max runs out of the clearing.
	Speech->AttenuationOverrides.FalloffDistance=4300;
	Speech->SetVolumeMultiplier(.72f);
	AddInstanceComponent(Speech); Speech->RegisterComponent();
	Speech->BindFace(BanditFace);
	// The source pistol animation has a prop socket; the MetaHuman hand omits
	// that helper. Use its measured transform in hand space to retain the grip.
	auto* Grip = NewObject<USceneComponent>(this, TEXT("HeldSidearm"));
	Grip->SetupAttachment(Bandit, TEXT("hand_r"));
	Grip->SetRelativeLocationAndRotation(FVector(-7.31,3.65,-1.43),FRotator(-5.78,176.37,3.24));
	AddInstanceComponent(Grip);
	Grip->RegisterComponent();
	auto* Handle = Part(Round,Bark,Grip);
	Handle->SetRelativeLocationAndRotation(FVector(0,0,.5),FRotator(-16,0,0));
	Handle->SetRelativeScale3D(FVector(.052,.037,.123));
	auto* Frame = Part(Cube,BlackMat,Grip);
	Frame->SetRelativeLocation(FVector(4,0,5));
	Frame->SetRelativeScale3D(FVector(.12,.034,.049));
	auto* Cylinder = Part(Pole,BlackMat,Grip);
	Cylinder->SetRelativeLocationAndRotation(FVector(7,0,5.5),FRotator(90,0,0));
	Cylinder->SetRelativeScale3D(FVector(.058,.058,.061));
	auto* Barrel = Part(Pole,BlackMat,Grip);
	Barrel->SetRelativeLocationAndRotation(FVector(16,0,7),FRotator(90,0,0));
	Barrel->SetRelativeScale3D(FVector(.025,.025,.145));
	Pack->SpawnMembers();
}
