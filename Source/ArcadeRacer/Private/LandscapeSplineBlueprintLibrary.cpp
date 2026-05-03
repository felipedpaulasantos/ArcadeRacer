#include "LandscapeSplineBlueprintLibrary.h"

#include "Landscape.h"
#include "LandscapeSplinesComponent.h"
#include "LandscapeSplineSegment.h"

#include "LandscapeProxy.h"

#include "UObject/UObjectIterator.h"

namespace
{
	static int32 CountLandscapesInWorld(UWorld* World)
	{
		int32 Count = 0;
		for (TObjectIterator<ALandscape> It; It; ++It)
		{
			ALandscape* L = *It;
			if (!IsValid(L) || L->IsTemplate())
			{
				continue;
			}
			if (World && L->GetWorld() != World)
			{
				continue;
			}
			++Count;
		}
		return Count;
	}

	static void GetAllLandscapeSplinesComponents(const ALandscape* Landscape, TArray<ULandscapeSplinesComponent*>& OutComponents)
	{
		OutComponents.Reset();
		if (!IsValid(Landscape))
		{
			return;
		}

		UWorld* World = Landscape->GetWorld();
		const int32 LandscapeCount = CountLandscapesInWorld(World);

		// Global iterator. Handles cases where splines are stored in separate actors (LandscapeSplineActor/WP).
		for (TObjectIterator<ULandscapeSplinesComponent> It; It; ++It)
		{
			ULandscapeSplinesComponent* Comp = *It;
			if (!IsValid(Comp) || Comp->IsTemplate())
			{
				continue;
			}

			if (World && Comp->GetWorld() != World)
			{
				continue;
			}

			// Fast path: exact match.
			if (Comp->GetSplineOwner() == Landscape)
			{
				OutComponents.AddUnique(Comp);
				continue;
			}

			// Common with LandscapeSplineActor: spline owner is a landscape proxy (streaming proxy included).
			// If there's only one landscape in the world, we accept proxy-owned spline components.
			if (LandscapeCount == 1)
			{
				UObject* OwnerObj = Cast<UObject>(Comp->GetSplineOwner());
				if (Cast<ALandscapeProxy>(OwnerObj) && Comp->GetSegments().Num() > 0)
				{
					OutComponents.AddUnique(Comp);
				}
			}
		}

		// Fallback: scan components on the landscape actor.
		if (OutComponents.Num() == 0)
		{
			TInlineComponentArray<ULandscapeSplinesComponent*> Found(Landscape);
			for (ULandscapeSplinesComponent* C : Found)
			{
				if (IsValid(C))
				{
					OutComponents.AddUnique(C);
				}
			}
		}
	}

	static void TransformPointsToWorld(const ULandscapeSplineSegment* Segment, const TArray<FLandscapeSplineInterpPoint>& LocalPoints, TArray<FVector>& OutWorldCenters)
	{
		OutWorldCenters.Reset();
		if (!IsValid(Segment) || LocalPoints.Num() == 0)
		{
			return;
		}

		// Segment's outer is a ULandscapeSplinesComponent; its component transform converts spline local space to world.
		const ULandscapeSplinesComponent* OwnerComp = Segment->GetOuterSafe();
		const FTransform LocalToWorld = IsValid(OwnerComp) ? OwnerComp->GetComponentTransform() : FTransform::Identity;

		OutWorldCenters.Reserve(LocalPoints.Num());
		for (const FLandscapeSplineInterpPoint& P : LocalPoints)
		{
			OutWorldCenters.Add(LocalToWorld.TransformPosition(P.Center));
		}
	}

	static bool SafeNormalize(const FVector& V, FVector& Out)
	{
		const float SizeSq = V.SizeSquared();
		if (SizeSq <= KINDA_SMALL_NUMBER)
		{
			Out = FVector::ForwardVector;
			return false;
		}

		Out = V / FMath::Sqrt(SizeSq);
		return true;
	}
}

bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineSegments(const ALandscape* Landscape, TArray<ULandscapeSplineSegment*>& OutSegments)
{
	OutSegments.Reset();

	TArray<ULandscapeSplinesComponent*> Components;
	GetAllLandscapeSplinesComponents(Landscape, Components);
	if (Components.Num() == 0)
	{
		return false;
	}

	for (ULandscapeSplinesComponent* Splines : Components)
	{
		if (!IsValid(Splines))
		{
			continue;
		}

		for (ULandscapeSplineSegment* Segment : Splines->GetSegments())
		{
			if (IsValid(Segment))
			{
				OutSegments.AddUnique(Segment);
			}
		}
	}

	return OutSegments.Num() > 0;
}

bool ULandscapeSplineBlueprintLibrary::GetSegmentInterpCenters(const ULandscapeSplineSegment* Segment, TArray<FVector>& OutCenters)
{
	OutCenters.Reset();
	if (!IsValid(Segment))
	{
		return false;
	}

	const TArray<FLandscapeSplineInterpPoint>& LocalPoints = Segment->GetPoints();
	if (LocalPoints.Num() < 2)
	{
		return false;
	}

	TransformPointsToWorld(Segment, LocalPoints, OutCenters);
	return OutCenters.Num() >= 2;
}

bool ULandscapeSplineBlueprintLibrary::BuildDistanceTableFromPoints(const TArray<FVector>& Points, TArray<float>& OutCumulativeDistances)
{
	OutCumulativeDistances.Reset();
	if (Points.Num() < 2) return false;
	OutCumulativeDistances.SetNum(Points.Num());
	OutCumulativeDistances[0] = 0.0f;
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		OutCumulativeDistances[i] = OutCumulativeDistances[i - 1] + FVector::Dist(Points[i - 1], Points[i]);
	}
	return true;
}
FVector ULandscapeSplineBlueprintLibrary::EvalFromDistanceTable(const TArray<FVector>& Points, const TArray<float>& CumulativeDistances, float Distance)
{
	if (Points.Num() == 0) return FVector::ZeroVector;
	if (Points.Num() == 1 || CumulativeDistances.Num() != Points.Num()) return Points[0];
	const float Total = CumulativeDistances.Last();
	if (Total <= KINDA_SMALL_NUMBER) return Points.Last();
	Distance = FMath::Clamp(Distance, 0.0f, Total);
	int32 Index = 1;
	while (Index < CumulativeDistances.Num() && CumulativeDistances[Index] < Distance) ++Index;
	if (Index >= CumulativeDistances.Num()) return Points.Last();
	const float D0 = CumulativeDistances[Index - 1];
	const float D1 = CumulativeDistances[Index];
	const float Alpha = (D1 - D0) > KINDA_SMALL_NUMBER ? (Distance - D0) / (D1 - D0) : 0.0f;
	return FMath::Lerp(Points[Index - 1], Points[Index], Alpha);
}
bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineSegmentLength(const ULandscapeSplineSegment* Segment, float& OutLength)
{
	OutLength = 0.0f;
	TArray<FVector> Centers;
	if (!GetSegmentInterpCenters(Segment, Centers)) return false;
	TArray<float> Cum;
	if (!BuildDistanceTableFromPoints(Centers, Cum)) return false;
	OutLength = Cum.Last();
	return true;
}
bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineTotalLength(const ALandscape* Landscape, float& OutTotalLength)
{
	OutTotalLength = 0.0f;

	TArray<ULandscapeSplineSegment*> Segments;
	if (!GetLandscapeSplineSegments(Landscape, Segments))
	{
		return false;
	}

	bool bAny = false;
	for (const ULandscapeSplineSegment* Segment : Segments)
	{
		float Len = 0.0f;
		if (GetLandscapeSplineSegmentLength(Segment, Len))
		{
			OutTotalLength += Len;
			bAny = true;
		}
	}

	return bAny;
}
bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineLocationAtDistance(const ULandscapeSplineSegment* Segment, float Distance, FVector& OutLocation, int32 SamplesPerSegment)
{
	OutLocation = FVector::ZeroVector;
	if (!IsValid(Segment)) return false;
(void)SamplesPerSegment;
	TArray<FVector> Centers;
	if (!GetSegmentInterpCenters(Segment, Centers)) return false;
	TArray<float> Cum;
	if (!BuildDistanceTableFromPoints(Centers, Cum)) return false;
	OutLocation = EvalFromDistanceTable(Centers, Cum, Distance);
	return true;
}
bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineLocationAtDistanceOnLandscape(const ALandscape* Landscape, float Distance, FVector& OutLocation, int32 SamplesPerSegment)
{
	OutLocation = FVector::ZeroVector;

	TArray<ULandscapeSplineSegment*> Segments;
	if (!GetLandscapeSplineSegments(Landscape, Segments))
	{
		return false;
	}

	float Remaining = Distance;
	for (const ULandscapeSplineSegment* Segment : Segments)
	{
		float SegmentLen = 0.0f;
		if (!GetLandscapeSplineSegmentLength(Segment, SegmentLen) || SegmentLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (Remaining <= SegmentLen)
		{
			return GetLandscapeSplineLocationAtDistance(Segment, Remaining, OutLocation, SamplesPerSegment);
		}

		Remaining -= SegmentLen;
	}

	for (int32 i = Segments.Num() - 1; i >= 0; --i)
	{
		const ULandscapeSplineSegment* Segment = Segments[i];
		float SegmentLen = 0.0f;
		if (GetLandscapeSplineSegmentLength(Segment, SegmentLen) && SegmentLen > KINDA_SMALL_NUMBER)
		{
			return GetLandscapeSplineLocationAtDistance(Segment, SegmentLen, OutLocation, SamplesPerSegment);
		}
	}

	return false;
}
bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineLocationAtInputKey(const ALandscape* Landscape, float InputKey, FVector& OutLocation, int32 SamplesPerSegment)
{
	OutLocation = FVector::ZeroVector;

	TArray<ULandscapeSplineSegment*> Segments;
	if (!GetLandscapeSplineSegments(Landscape, Segments) || Segments.Num() == 0)
	{
		return false;
	}

	const int32 SegmentIndex = FMath::Clamp((int32)FMath::FloorToFloat(InputKey), 0, Segments.Num() - 1);
	const float AlphaWithinSegment = FMath::Clamp(InputKey - (float)SegmentIndex, 0.0f, 1.0f);

	const ULandscapeSplineSegment* Segment = Segments[SegmentIndex];
	float SegmentLen = 0.0f;
	if (!GetLandscapeSplineSegmentLength(Segment, SegmentLen))
	{
		return false;
	}

	return GetLandscapeSplineLocationAtDistance(Segment, AlphaWithinSegment * SegmentLen, OutLocation, SamplesPerSegment);
}

bool ULandscapeSplineBlueprintLibrary::BuildDistanceTableCached(
	const ULandscapeSplineSegment* Segment,
	TArray<FVector>& OutCenters,
	TArray<float>& OutCumulativeDistances,
	float& OutSegmentLength
)
{
	OutSegmentLength = 0.0f;
	OutCenters.Reset();
	OutCumulativeDistances.Reset();

	if (!GetSegmentInterpCenters(Segment, OutCenters))
	{
		return false;
	}

	if (!BuildDistanceTableFromPoints(OutCenters, OutCumulativeDistances))
	{
		return false;
	}

	OutSegmentLength = OutCumulativeDistances.Last();
	return true;
}

bool ULandscapeSplineBlueprintLibrary::FindLandscapeSplineInputKeyClosestToWorldLocation(
	const ALandscape* Landscape,
	FVector WorldLocation,
	float& OutInputKey,
	float& OutDistanceToSpline,
	int32 SamplesPerSegment
)
{
	OutInputKey = 0.0f;
	OutDistanceToSpline = 0.0f;

	TArray<ULandscapeSplineSegment*> Segments;
	if (!GetLandscapeSplineSegments(Landscape, Segments) || Segments.Num() == 0)
	{
		return false;
	}

	SamplesPerSegment = FMath::Max(2, SamplesPerSegment);

	bool bFoundAny = false;
	float BestDistSq = TNumericLimits<float>::Max();
	int32 BestSegmentIndex = 0;
	float BestAlpha = 0.0f;

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		const ULandscapeSplineSegment* Segment = Segments[SegmentIndex];
		TArray<FVector> Centers;
		TArray<float> Cum;
		float SegmentLen = 0.0f;
		if (!BuildDistanceTableCached(Segment, Centers, Cum, SegmentLen) || SegmentLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		for (int32 s = 0; s < SamplesPerSegment; ++s)
		{
			const float T = (float)s / (float)(SamplesPerSegment - 1);
			const float D = T * SegmentLen;
			const FVector P = EvalFromDistanceTable(Centers, Cum, D);
			const float DistSq = FVector::DistSquared(P, WorldLocation);

			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestSegmentIndex = SegmentIndex;
				BestAlpha = T;
				bFoundAny = true;
			}
		}
	}

	if (!bFoundAny)
	{
		return false;
	}

	OutInputKey = (float)BestSegmentIndex + BestAlpha;
	OutDistanceToSpline = FMath::Sqrt(BestDistSq);
	return true;
}

bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineDistanceAlongAtInputKey(
	const ALandscape* Landscape,
	float InputKey,
	float& OutDistanceAlongSpline
)
{
	OutDistanceAlongSpline = 0.0f;

	TArray<ULandscapeSplineSegment*> Segments;
	if (!GetLandscapeSplineSegments(Landscape, Segments) || Segments.Num() == 0)
	{
		return false;
	}

	const int32 SegmentIndex = FMath::Clamp((int32)FMath::FloorToFloat(InputKey), 0, Segments.Num() - 1);
	const float AlphaWithinSegment = FMath::Clamp(InputKey - (float)SegmentIndex, 0.0f, 1.0f);

	float Distance = 0.0f;
	for (int32 i = 0; i < SegmentIndex; ++i)
	{
		float Len = 0.0f;
		if (GetLandscapeSplineSegmentLength(Segments[i], Len))
		{
			Distance += Len;
		}
	}

	float SegmentLen = 0.0f;
	if (!GetLandscapeSplineSegmentLength(Segments[SegmentIndex], SegmentLen))
	{
		return false;
	}

	Distance += AlphaWithinSegment * SegmentLen;
	OutDistanceAlongSpline = Distance;
	return true;
}

bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineDirectionAtDistanceOnLandscape(
	const ALandscape* Landscape,
	float Distance,
	FVector& OutDirection
)
{
	OutDirection = FVector::ForwardVector;

	TArray<ULandscapeSplineSegment*> Segments;
	if (!GetLandscapeSplineSegments(Landscape, Segments) || Segments.Num() == 0)
	{
		return false;
	}

	float Remaining = Distance;

	for (const ULandscapeSplineSegment* Segment : Segments)
	{
		float SegmentLen = 0.0f;
		if (!GetLandscapeSplineSegmentLength(Segment, SegmentLen) || SegmentLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (Remaining <= SegmentLen)
		{
			return GetLandscapeSplineDirectionAtDistance(Segment, Remaining, OutDirection);
		}

		Remaining -= SegmentLen;
	}

	for (int32 i = Segments.Num() - 1; i >= 0; --i)
	{
		const ULandscapeSplineSegment* Segment = Segments[i];
		float SegmentLen = 0.0f;
		if (GetLandscapeSplineSegmentLength(Segment, SegmentLen) && SegmentLen > KINDA_SMALL_NUMBER)
		{
			return GetLandscapeSplineDirectionAtDistance(Segment, SegmentLen, OutDirection);
		}
	}

	return false;
}

bool ULandscapeSplineBlueprintLibrary::EvalDirectionFromDistanceTable(
	const TArray<FVector>& Points,
	const TArray<float>& CumulativeDistances,
	float Distance,
	FVector& OutDirection
)
{
	OutDirection = FVector::ForwardVector;

	if (Points.Num() < 2 || CumulativeDistances.Num() != Points.Num())
	{
		return false;
	}

	const float Total = CumulativeDistances.Last();
	if (Total <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	Distance = FMath::Clamp(Distance, 0.0f, Total);

	int32 Index = 1;
	while (Index < CumulativeDistances.Num() && CumulativeDistances[Index] < Distance)
	{
		++Index;
	}

	if (Index >= Points.Num())
	{
		const FVector Dir = Points.Last() - Points[Points.Num() - 2];
		return SafeNormalize(Dir, OutDirection);
	}

	const FVector Dir = Points[Index] - Points[Index - 1];
	return SafeNormalize(Dir, OutDirection);
}

bool ULandscapeSplineBlueprintLibrary::GetLandscapeSplineDirectionAtDistance(
	const ULandscapeSplineSegment* Segment,
	float Distance,
	FVector& OutDirection
)
{
	OutDirection = FVector::ForwardVector;

	TArray<FVector> Centers;
	TArray<float> Cum;
	float SegmentLen = 0.0f;
	if (!BuildDistanceTableCached(Segment, Centers, Cum, SegmentLen))
	{
		return false;
	}

	return EvalDirectionFromDistanceTable(Centers, Cum, Distance, OutDirection);
}

