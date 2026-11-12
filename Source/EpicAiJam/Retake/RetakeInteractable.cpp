#include "Retake/RetakeInteractable.h"

#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

int32 RetakeInteraction::PerformInteract(AActor* Interactor, float Radius)
{
	if (!Interactor || !Interactor->GetWorld())
	{
		return 0;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RetakeInteract), false, Interactor);

	TArray<FOverlapResult> Overlaps;
	Interactor->GetWorld()->OverlapMultiByObjectType(
		Overlaps, Interactor->GetActorLocation(), FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Radius), QueryParams);

	TSet<AActor*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Done.Contains(Target))
		{
			continue;
		}
		if (IRetakeInteractable* Interactable = Cast<IRetakeInteractable>(Target))
		{
			Done.Add(Target);
			Interactable->RetakeInteract(Interactor);
		}
	}
	return Done.Num();
}
