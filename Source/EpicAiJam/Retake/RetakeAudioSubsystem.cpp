#include "Retake/RetakeAudioSubsystem.h"

#include "Animation/AnimInstance.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/ExplosiveProp.h"
#include "Retake/Gate.h"
#include "Retake/Lever.h"
#include "Retake/PressurePlate.h"
#include "Retake/PushCrate.h"
#include "Retake/RetakeAudioSet.h"
#include "Retake/RetakeSaveGame.h"
#include "Retake/StuntGuard.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "TimerManager.h"

URetakeAudioSet* URetakeSettings::LoadAudioSet()
{
	const URetakeSettings* Settings = GetDefault<URetakeSettings>();
	return Settings ? Settings->AudioSet.LoadSynchronous() : nullptr;
}

bool URetakeSettings::ApplyCharacterLook(ACharacter* Character, UStaticMeshComponent* Placeholder, UStaticMeshComponent* Helmet, bool bPlayerSuit)
{
	const URetakeSettings* S = GetDefault<URetakeSettings>();
	USkeletalMesh* Mesh = S ? S->CharacterMesh.LoadSynchronous() : nullptr;
	if (!Character || !Mesh)
	{
		return false;
	}
	USkeletalMeshComponent* Skel = Character->GetMesh();
	Skel->SetSkeletalMesh(Mesh);
	// Mannequin convention: feet at the capsule bottom, facing +X.
	Skel->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	if (UClass* Anim = S->CharacterAnimClass.LoadSynchronous())
	{
		Skel->SetAnimInstanceClass(Anim);
	}
	if (bPlayerSuit)
	{
		for (int32 i = 0; i < S->StuntSuitMaterials.Num(); ++i)
		{
			if (UMaterialInterface* M = S->StuntSuitMaterials[i].LoadSynchronous())
			{
				Skel->SetMaterial(i, M);
			}
		}
	}
	if (Helmet)
	{
		Helmet->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("head"));
		UStaticMesh* HelmetAsset = S->HelmetMesh.LoadSynchronous();
		Helmet->SetStaticMesh(HelmetAsset);
		if (HelmetAsset)
		{
			const FVector Size = HelmetAsset->GetBoundingBox().GetSize();
			const float Scale = S->HelmetSize / FMath::Max(1.f, Size.GetMax());
			Helmet->SetRelativeScale3D(FVector(Scale));
			Helmet->SetRelativeLocationAndRotation(S->HelmetOffset, S->HelmetRotation);
		}
	}
	if (Placeholder)
	{
		Placeholder->SetVisibility(false);
	}
	return true;
}

bool URetakeAudioSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void URetakeAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Set = URetakeSettings::LoadAudioSet();
	ApplySavedVolumes();

	if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(&InWorld))
	{
		Director->OnStateChanged.AddDynamic(this, &URetakeAudioSubsystem::HandleStateChanged);
		Director->OnTakeStarted.AddDynamic(this, &URetakeAudioSubsystem::HandleTakeStarted);
		Director->OnTakeCut.AddDynamic(this, &URetakeAudioSubsystem::HandleTakeCut);
		Director->OnSceneFailed.AddDynamic(this, &URetakeAudioSubsystem::HandleSceneFailed);
		Director->OnWrap.AddDynamic(this, &URetakeAudioSubsystem::HandleWrap);
	}
	else if (Set)
	{
		PlayMusic(Set->MusicMenu); // no director = main menu
	}

	for (TActorIterator<ARetakeActivator> It(&InWorld); It; ++It)
	{
		It->OnActivated.AddDynamic(this, &URetakeAudioSubsystem::HandleActivated);
		It->OnDeactivated.AddDynamic(this, &URetakeAudioSubsystem::HandleDeactivated);
	}
	for (TActorIterator<AGate> It(&InWorld); It; ++It)
	{
		It->OnOpening.AddDynamic(this, &URetakeAudioSubsystem::HandleGateOpening);
		It->OnClosing.AddDynamic(this, &URetakeAudioSubsystem::HandleGateClosing);
	}
	for (TActorIterator<AExplosiveProp> It(&InWorld); It; ++It)
	{
		It->OnArmed.AddDynamic(this, &URetakeAudioSubsystem::HandleArmed);
		It->OnExploded.AddDynamic(this, &URetakeAudioSubsystem::HandleExploded);
	}
	for (TActorIterator<AStuntGuard> It(&InWorld); It; ++It)
	{
		It->OnSpotted.AddDynamic(this, &URetakeAudioSubsystem::HandleSpotted);
		It->OnCaughtSomeone.AddDynamic(this, &URetakeAudioSubsystem::HandleCaught);
	}
	for (TActorIterator<APushCrate> It(&InWorld); It; ++It)
	{
		It->OnPushStarted.AddDynamic(this, &URetakeAudioSubsystem::HandleCratePush);
	}
}

void URetakeAudioSubsystem::Deinitialize()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
	}
	Super::Deinitialize();
}

USoundClass* URetakeAudioSubsystem::ClassFor(ECategory Category) const
{
	if (!Set)
	{
		return nullptr;
	}
	switch (Category)
	{
	case ECategory::Music: return Set->MusicClass;
	case ECategory::Voice: return Set->VoiceClass;
	default:               return Set->SFXClass;
	}
}

void URetakeAudioSubsystem::PlayNamedVoice(const FString& Name, float Delay)
{
	const FString Path = FString::Printf(TEXT("/Game/Retake/Audio/Sounds/%s.%s"), *Name, *Name);
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		PlaySound(Sound, ECategory::Voice, Delay);
	}
}

void URetakeAudioSubsystem::PlaySound(USoundBase* Sound, ECategory Category, float Delay)
{
	UWorld* World = GetWorld();
	if (!Sound || !World)
	{
		return; // empty slot = silent
	}
	if (Delay > 0.f)
	{
		FTimerHandle Handle;
		TWeakObjectPtr<URetakeAudioSubsystem> WeakThis(this);
		TWeakObjectPtr<USoundBase> WeakSound(Sound);
		World->GetTimerManager().SetTimer(Handle, [WeakThis, WeakSound, Category]()
		{
			if (WeakThis.IsValid() && WeakSound.IsValid())
			{
				WeakThis->PlaySound(WeakSound.Get(), Category, 0.f);
			}
		}, Delay, false);
		return;
	}
	if (UAudioComponent* Comp = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, false, true))
	{
		Comp->SoundClassOverride = ClassFor(Category);
		Comp->Play();
	}
}

void URetakeAudioSubsystem::PlayMusic(USoundBase* Music)
{
	if (Music == CurrentMusic)
	{
		return;
	}
	CurrentMusic = Music;
	if (MusicComponent)
	{
		MusicComponent->FadeOut(0.8f, 0.f);
		MusicComponent = nullptr;
	}
	if (!Music || !GetWorld())
	{
		return;
	}
	MusicComponent = UGameplayStatics::CreateSound2D(GetWorld(), Music, 1.f, 1.f, 0.f, nullptr, true, true);
	if (MusicComponent)
	{
		MusicComponent->SoundClassOverride = ClassFor(ECategory::Music);
		MusicComponent->FadeIn(0.8f);
	}
}

void URetakeAudioSubsystem::ApplySavedVolumes()
{
	if (const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate())
	{
		SetVolumes(GetWorld(), Save->MusicVolume, Save->SfxVolume, Save->VoiceVolume, false);
	}
}

void URetakeAudioSubsystem::SetVolumes(const UObject* WorldContext, float Music, float Sfx, float Voice, bool bSave)
{
	if (bSave)
	{
		if (URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate())
		{
			Save->MusicVolume = Music;
			Save->SfxVolume = Sfx;
			Save->VoiceVolume = Voice;
			Save->SaveNow();
		}
	}
	const URetakeAudioSet* AudioSet = URetakeSettings::LoadAudioSet();
	if (!AudioSet || !AudioSet->VolumeMix || !WorldContext)
	{
		return;
	}
	auto Apply = [&](USoundClass* Class, float Volume)
	{
		if (Class)
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContext, AudioSet->VolumeMix, Class, Volume, 1.f, 0.f, true);
		}
	};
	Apply(AudioSet->MusicClass, Music);
	Apply(AudioSet->SFXClass, Sfx);
	Apply(AudioSet->VoiceClass, Voice);
	UGameplayStatics::PushSoundMixModifier(WorldContext, AudioSet->VolumeMix);
}

void URetakeAudioSubsystem::PlayUIClick()
{
	if (Set) PlaySound(Set->SfxUIClick, ECategory::Sfx);
}

// ------------------------------------------------------------------------------------ director

void URetakeAudioSubsystem::HandleStateChanged(ERetakeSceneState NewState, ERetakeSceneState OldState)
{
	if (!Set) return;
	switch (NewState)
	{
	case ERetakeSceneState::Briefing:     PlayMusic(Set->MusicBriefing); break;
	case ERetakeSceneState::Rolling:      PlayMusic(Set->MusicRollingLoop); break;
	case ERetakeSceneState::Cut:
	case ERetakeSceneState::SceneFailed:  PlayMusic(nullptr); break;
	case ERetakeSceneState::DirectorsCut: PlayMusic(Set->MusicDirectorsCut); break;
	case ERetakeSceneState::Results:
		PlayMusic(Set->MusicResults);
		PlaySound(Set->VoicePrintIt, ECategory::Voice);
		break;
	default: break;
	}
}

void URetakeAudioSubsystem::HandleTakeStarted(int32 TakeNumber)
{
	if (!Set) return;
	PlaySound(Set->SfxClapperboard, ECategory::Sfx);
	USoundBase* TakeLine = nullptr;
	switch (TakeNumber)
	{
	case 1: TakeLine = bRestartedFromTop ? Set->VoiceFromTheTop.Get() : nullptr; break;
	case 2: TakeLine = Set->VoiceTakeTwo; break;
	case 3: TakeLine = Set->VoiceTakeThree; break;
	case 4: TakeLine = Set->VoiceTakeFour; break;
	case 5: TakeLine = Set->VoiceTakeFive; break;
	default: break;
	}
	bRestartedFromTop = false;
	PlaySound(TakeLine, ECategory::Voice);
	// "Action!" right after the take call-out.
	PlaySound(Set->VoiceAction, ECategory::Voice, TakeLine ? 0.9f : 0.15f);
}

void URetakeAudioSubsystem::HandleTakeCut(int32 TakeNumber, ERetakeCutReason Reason)
{
	if (!Set) return;
	PlaySound(Set->VoiceCut, ECategory::Voice);
	if (Reason == ERetakeCutReason::Killed)
	{
		PlaySound(Set->VoiceNewStuntDouble, ECategory::Voice, 0.8f);
	}
}

void URetakeAudioSubsystem::HandleSceneFailed()
{
	bRestartedFromTop = true; // the next "take 1" gets "From the top!"
}

void URetakeAudioSubsystem::HandleWrap()
{
	if (!Set) return;
	PlaySound(Set->SfxGoal, ECategory::Sfx);
	PlaySound(Set->VoiceThatsAWrap, ECategory::Voice, 0.3f);
}

// ------------------------------------------------------------------------------------ set pieces

void URetakeAudioSubsystem::HandleActivated(ARetakeActivator* Activator)
{
	if (!Set) return;
	PlaySound(Cast<ALever>(Activator) ? Set->SfxLever.Get() : Set->SfxPlateOn.Get(), ECategory::Sfx);
}

void URetakeAudioSubsystem::HandleDeactivated(ARetakeActivator* Activator)
{
	if (!Set) return;
	PlaySound(Cast<ALever>(Activator) ? Set->SfxLever.Get() : Set->SfxPlateOff.Get(), ECategory::Sfx);
}

void URetakeAudioSubsystem::HandleGateOpening(AGate*)            { if (Set) PlaySound(Set->SfxGateOpen, ECategory::Sfx); }
void URetakeAudioSubsystem::HandleGateClosing(AGate*)            { if (Set) PlaySound(Set->SfxGateClose, ECategory::Sfx); }
void URetakeAudioSubsystem::HandleArmed(AExplosiveProp*)         { if (Set) PlaySound(Set->SfxExplosiveArm, ECategory::Sfx); }
void URetakeAudioSubsystem::HandleExploded(AExplosiveProp*)      { if (Set) PlaySound(Set->SfxExplosion, ECategory::Sfx); }
void URetakeAudioSubsystem::HandleSpotted(AStuntGuard*, AActor*) { if (Set) PlaySound(Set->SfxGuardAlert, ECategory::Sfx); }
void URetakeAudioSubsystem::HandleCaught(AStuntGuard*, AActor*)  { if (Set) PlaySound(Set->SfxGuardCatch, ECategory::Sfx); }
void URetakeAudioSubsystem::HandleCratePush(APushCrate*)         { if (Set) PlaySound(Set->SfxCratePush, ECategory::Sfx); }
