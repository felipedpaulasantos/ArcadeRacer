#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LandscapeSplineBlueprintLibrary.generated.h"

class ALandscape;
class ULandscapeSplineSegment;

/**
 * Blueprint helpers to query Landscape Splines with a SplineComponent-like API.
 *
 * Notes:
 * - Landscape splines are stored as multiple segments, not a single continuous spline.
 * - This library provides approximation-based queries by sampling segment interpolation points.
 * - All locations are returned in world space.
 */
UCLASS()
class ARCADERACER_API ULandscapeSplineBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns all spline segments from a Landscape (in the current stored order). */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineSegments(const ALandscape* Landscape, TArray<ULandscapeSplineSegment*>& OutSegments);

	/** Returns an approximate length for a single landscape spline segment (in Unreal units). */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineSegmentLength(const ULandscapeSplineSegment* Segment, float& OutLength);

	/**
	 * Returns an approximate total length across all segments (sum of segment lengths).
	 * This is not guaranteed to match any intended "path" if segments aren't connected or ordered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineTotalLength(const ALandscape* Landscape, float& OutTotalLength);

	/**
	 * Equivalent in spirit to USplineComponent::GetLocationAtDistanceAlongSpline, but for a single segment.
	 * Distance is clamped to [0, SegmentLength].
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineLocationAtDistance(
		const ULandscapeSplineSegment* Segment,
		float Distance,
		FVector& OutLocation,
		int32 SamplesPerSegment = 64
	);

	/**
	 * Equivalent in spirit to USplineComponent::GetLocationAtDistanceAlongSpline, but over all segments.
	 * Distance is clamped to [0, TotalLength].
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineLocationAtDistanceOnLandscape(
		const ALandscape* Landscape,
		float Distance,
		FVector& OutLocation,
		int32 SamplesPerSegment = 64
	);

	/**
	 * Equivalent in spirit to USplineComponent::GetLocationAtSplineInputKey.
	 * InputKey format: SegmentIndex + AlphaWithinSegment, where AlphaWithinSegment is [0..1].
	 * Example: 3.25 => segment 3 at 25%.
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineLocationAtInputKey(
		const ALandscape* Landscape,
		float InputKey,
		FVector& OutLocation,
		int32 SamplesPerSegment = 64
	);

	/**
	 * Equivalent in spirit to USplineComponent::FindInputKeyClosestToWorldLocation, but for landscape splines.
	 * Returns an InputKey in the format SegmentIndex + AlphaWithinSegment.
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool FindLandscapeSplineInputKeyClosestToWorldLocation(
		const ALandscape* Landscape,
		FVector WorldLocation,
		float& OutInputKey,
		float& OutDistanceToSpline,
		int32 SamplesPerSegment = 64
	);

	/**
	 * Equivalent in spirit to USplineComponent::GetDistanceAlongSplineAtSplineInputKey.
	 * Treats the spline as all segments in their stored order; distance is accumulated across previous segments.
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineDistanceAlongAtInputKey(
		const ALandscape* Landscape,
		float InputKey,
		float& OutDistanceAlongSpline
	);

	/**
	 * Equivalent in spirit to USplineComponent::GetDirectionAtDistanceAlongSpline, but for a single landscape spline segment.
	 * Direction is a unit vector in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineDirectionAtDistance(
		const ULandscapeSplineSegment* Segment,
		float Distance,
		FVector& OutDirection
	);

	/**
	 * Same as GetLandscapeSplineDirectionAtDistance, but for all segments in the landscape (stored order).
	 */
	UFUNCTION(BlueprintCallable, Category = "Landscape|Splines")
	static bool GetLandscapeSplineDirectionAtDistanceOnLandscape(
		const ALandscape* Landscape,
		float Distance,
		FVector& OutDirection
	);

private:
	static bool GetSegmentInterpCenters(const ULandscapeSplineSegment* Segment, TArray<FVector>& OutCenters);
	static bool BuildDistanceTableFromPoints(const TArray<FVector>& Points, TArray<float>& OutCumulativeDistances);
	static FVector EvalFromDistanceTable(const TArray<FVector>& Points, const TArray<float>& CumulativeDistances, float Distance);
	static bool BuildDistanceTableCached(
		const ULandscapeSplineSegment* Segment,
		TArray<FVector>& OutCenters,
		TArray<float>& OutCumulativeDistances,
		float& OutSegmentLength
	);
	static bool EvalDirectionFromDistanceTable(
		const TArray<FVector>& Points,
		const TArray<float>& CumulativeDistances,
		float Distance,
		FVector& OutDirection
	);
};
