// Made by Mathieu de Ruiter (github.com/casmo), Spark of Chaos (sparkofchaos.com)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Math/Vector.h"
#include "ZoneMarker.generated.h"

UCLASS()
class CROPOUTSAMPLEPROJECT_API AZoneMarker : public AActor
{
	GENERATED_BODY()
	
public:
	AZoneMarker();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	float MaxDistance = 100.0f;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USplineComponent* SplineComponent;

	/**
	 * Adds a location to the array.
	 * @param Location The location to add.
	 * @param Radius The radius to add the location around.
	 * @param bUpdateSpline If true, the spline will be updated after the points are added.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone")
	void AddLocation(FVector Location, float Radius = 100.0f, bool bUpdateSpline = true);

	/**
	 * Removes all hull points visible from the given viewing position.
	 * @param Location The location to remove the points from.
	 * @param Range The range to remove the points from.
	 * @param bUpdateSpline If true, the spline will be updated after the points are removed.
	 */
	// ViewPosition: The position to view the hull from. Points visible from this position will be removed.
	UFUNCTION(BlueprintCallable, Category = "Zone")
	void RemoveLocation(FVector Location, float Range = 100.0f, bool bUpdateSpline = true);

	/**
     * Generated the concave and updates the spline.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone")
	void UpdateSpline();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	TArray<FVector> LocationArray;

	/**
	 * Array of Vector 3 locations for concave hull, they are the same points as the spline points.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FVector> HullLocations;

	/**
	 * Checks if a location is inside the hull.
	 * @param Location The location to check.
	 * @return True if the location is inside the hull, false otherwise.
	 */
	UFUNCTION(BlueprintPure, Category = "Zone")
	bool IsLocationInsideHull(FVector Location);

private:

	// ------------------------------------------------------------------
	// Helper: orientation (positive = left turn, negative = right turn)
	// ------------------------------------------------------------------
	// double turn(const FVector& a, const FVector& b, const FVector& c);
	int32 orientation(FVector p, FVector q, FVector r);

	// Check if a point is inside a polygon (2D, using X and Y only)
	bool IsPointInsidePolygon(const FVector& Point, const TArray<FVector>& Polygon);

	// Check if a circle is fully inside the hull
	bool IsCircleFullyInsideHull(const FVector& Center, float Radius, const TArray<FVector>& Hull);

	// Convert 2D points from concaveman to FVector array, preserving Z values from original points
	TArray<FVector> Convert2DPointsToFVector(const std::vector<std::array<float, 2>>& Points2D, const TArray<FVector>& OriginalPoints, float Tolerance = 0.01f);

};
