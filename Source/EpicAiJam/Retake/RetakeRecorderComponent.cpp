#include "Retake/RetakeRecorderComponent.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Retake/RetakeCharacter.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogRetake, Log, All);

URetakeRecorderComponent::URetakeRecorderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Sample after movement has been applied this frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void URetakeRecorderComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ARetakeCharacter* Character = Cast<ARetakeCharacter>(GetOwner()))
	{
		Character->OnActionPerformed.AddDynamic(this, &URetakeRecorderComponent::HandleOwnerAction);
		if (bDebugAutoRecord)
		{
			Character->OnCutRequested.AddDynamic(this, &URetakeRecorderComponent::HandleOwnerCut);
		}
	}

	if (bDebugAutoRecord)
	{
		StartRecording();
	}
}

void URetakeRecorderComponent::StartRecording()
{
	CurrentTake = FTakeData();
	CurrentTake.TakeNumber = NextTakeNumber++;
	CurrentTake.Frames.Reserve(static_cast<int32>(SampleRate * 60.f));
	Accumulator = 0.f;
	PendingActions = 0;
	bRecording = true;
	SetComponentTickEnabled(true);

	SampleFrame(); // frame 0 at t = 0
}

FTakeData URetakeRecorderComponent::StopRecording()
{
	if (!bRecording)
	{
		return LastTake;
	}

	bRecording = false;
	SetComponentTickEnabled(false);

	CurrentTake.Duration = CurrentTake.Frames.Num() > 0 ? CurrentTake.Frames.Last().Time : 0.f;
	LastTake = MoveTemp(CurrentTake);
	CurrentTake = FTakeData();

	UE_LOG(LogRetake, Log, TEXT("CUT! Take %d: %d frames, %.2f s"), LastTake.TakeNumber, LastTake.Frames.Num(), LastTake.Duration);
	return LastTake;
}

void URetakeRecorderComponent::RecordAction(ERetakeAction Action)
{
	if (bRecording)
	{
		PendingActions |= static_cast<uint8>(Action);
	}
}

void URetakeRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bRecording)
	{
		return;
	}

	Accumulator += DeltaTime;
	while (Accumulator >= SampleInterval)
	{
		Accumulator -= SampleInterval;
		SampleFrame();
	}
}

void URetakeRecorderComponent::SampleFrame()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FTakeFrame& Frame = CurrentTake.Frames.AddDefaulted_GetRef();
	// Time is derived from the frame index, so it is exactly 30 Hz regardless of frame rate.
	Frame.Time = (CurrentTake.Frames.Num() - 1) * SampleInterval;
	Frame.Location = Owner->GetActorLocation();
	Frame.Rotation = Owner->GetActorRotation();
	Frame.Velocity = Owner->GetVelocity();
	Frame.ActionFlags = PendingActions;
	PendingActions = 0;
}

void URetakeRecorderComponent::HandleOwnerAction(ERetakeAction Action)
{
	RecordAction(Action);
}

void URetakeRecorderComponent::HandleOwnerCut()
{
	StopRecording();
	StartRecording();
}

// ---------------------------------------------------------------------------
// Console command: Retake.DumpTake
// ---------------------------------------------------------------------------

static void DumpLastTakes(UWorld* World)
{
	int32 Found = 0;
	for (TObjectIterator<URetakeRecorderComponent> It; It; ++It)
	{
		const URetakeRecorderComponent* Recorder = *It;
		if (!IsValid(Recorder) || Recorder->GetWorld() != World)
		{
			continue;
		}
		++Found;

		const FTakeData& Take = Recorder->GetLastTake();
		if (Take.TakeNumber == 0)
		{
			UE_LOG(LogRetake, Display, TEXT("[%s] No finished take yet (recording: %s)."),
				*GetNameSafe(Recorder->GetOwner()), Recorder->IsRecording() ? TEXT("yes") : TEXT("no"));
			continue;
		}

		int32 Jumps = 0, Interacts = 0, Pushes = 0;
		for (const FTakeFrame& Frame : Take.Frames)
		{
			Jumps += Frame.HasAction(ERetakeAction::Jump);
			Interacts += Frame.HasAction(ERetakeAction::Interact);
			Pushes += Frame.HasAction(ERetakeAction::Push);
		}

		UE_LOG(LogRetake, Display, TEXT("[%s] Take %d: %d frames, %.2f s (jump %d, interact %d, push %d)"),
			*GetNameSafe(Recorder->GetOwner()), Take.TakeNumber, Take.Frames.Num(), Take.Duration,
			Jumps, Interacts, Pushes);
	}

	if (Found == 0)
	{
		UE_LOG(LogRetake, Display, TEXT("Retake.DumpTake: no recorder in this world."));
	}
}

static FAutoConsoleCommandWithWorld GRetakeDumpTakeCmd(
	TEXT("Retake.DumpTake"),
	TEXT("Logs frame count and duration of the last recorded take."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&DumpLastTakes));
