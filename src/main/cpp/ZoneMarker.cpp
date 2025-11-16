// Made by Mathieu de Ruiter (github.com/casmo), Spark of Chaos (sparkofchaos.com)
// Parts of this code utilize the concaveman algorithm, available under its respective license.

#include "ZoneMarker.h"
#include "Components/SplineComponent.h"
#include "concaveman/concaveman.h"

AZoneMarker::AZoneMarker()
{
	PrimaryActorTick.bCanEverTick = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;
}

void AZoneMarker::BeginPlay()
{
	Super::BeginPlay();
	
}

void AZoneMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/**
 * Adds a radius of points around the Location.
 * @param Location The location to add the points around.
 * @param Radius The radius of the points to add.
 * @param bUpdateSpline If true, the spline will be updated after the points are added.
*/
void AZoneMarker::AddLocation(FVector Location, float Radius, bool bUpdateSpline)
{
	for (int32 i = 0; i < 32; i++)
	{
		float angle = i * 2 * PI / 32;
		FVector newLocation = Location + FVector(Radius * FMath::Cos(angle), Radius * FMath::Sin(angle), 0.0f);
		if (!IsPointInsidePolygon(newLocation, LocationArray))
		{
			LocationArray.Add(newLocation);
		}
	}
	
	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}

/**
 * Checks if a location is inside the hull.
 * @param Location The location to check.
 * @return True if the location is inside the hull, false otherwise.
 */
bool AZoneMarker::IsLocationInsideHull(FVector Location)
{
	return IsPointInsidePolygon(Location, HullLocations);
}

bool AZoneMarker::IsPointInsidePolygon(const FVector& Point, const TArray<FVector>& Polygon)
{
	if (Polygon.Num() < 3)
	{
		return false;
	}

	// Ray casting algorithm: count intersections of a horizontal ray from point to infinity
	int32 intersections = 0;
	int32 n = Polygon.Num();

	for (int32 i = 0; i < n; ++i)
	{
		int32 j = (i + 1) % n;
		const FVector& p1 = Polygon[i];
		const FVector& p2 = Polygon[j];

		// Check if ray intersects with edge (p1, p2)
		// Skip horizontal edges
		if (FMath::Abs(p2.Y - p1.Y) > 1e-6f)
		{
			if (((p1.Y > Point.Y) != (p2.Y > Point.Y)) &&
				(Point.X < (p2.X - p1.X) * (Point.Y - p1.Y) / (p2.Y - p1.Y) + p1.X))
			{
				intersections++;
			}
		}
	}

	// Odd number of intersections means point is inside
	return (intersections % 2) == 1;
}

bool AZoneMarker::IsCircleFullyInsideHull(const FVector& Center, float Radius, const TArray<FVector>& Hull)
{
	if (Hull.Num() < 3)
	{
		return false;
	}

	// Check if center is inside
	if (!IsPointInsidePolygon(Center, Hull))
	{
		return false;
	}

	// Check 8 points on the circle perimeter
	for (int32 i = 0; i < 8; ++i)
	{
		float angle = i * 2 * PI / 8;
		FVector perimeterPoint = Center + FVector(Radius * FMath::Cos(angle), Radius * FMath::Sin(angle), 0.0f);
		
		if (!IsPointInsidePolygon(perimeterPoint, Hull))
		{
			return false;
		}
	}

	return true;
}

/**
 * Removes a location from the array.
 * @param Location The location to remove.
 * @param Range The range to remove the location from.
 * @param bUpdateSpline If true, the spline will be updated after the points are removed.
 */
void AZoneMarker::RemoveLocation(FVector Location, float Range, bool bUpdateSpline)
{
	if (LocationArray.Num() < 3)
	{
		// Need at least 3 points to form a hull
		return;
	}

	// Check if the location with Range is fully inside the concave hull
	if (IsCircleFullyInsideHull(Location, Range, HullLocations))
	{
		return;
	}

	// Partially outside: store original hull
	TArray<FVector> originalHull = HullLocations;

	// Remove all locations from LocationArray that are within Range of Location
	for (int32 i = LocationArray.Num() - 1; i >= 0; --i)
	{
		if (FVector::DistSquared(LocationArray[i], Location) <= Range * Range)
		{
			LocationArray.RemoveAt(i);
		}
	}
	
	for (int32 i = 0; i < 32; i++)
	{
		float angle = i * 2 * PI / 32;
		FVector newLocation = Location + FVector(Range * FMath::Cos(angle), Range * FMath::Sin(angle), 0.0f);
		if (!IsPointInsidePolygon(newLocation, originalHull))
		{
			continue;
		}
		LocationArray.Add(newLocation);
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}

/**
 * Generated the concave and updates the spline.
 */
void AZoneMarker::UpdateSpline()
{
	if (LocationArray.Num() < 4)
	{
		return;
	}

	// Convert LocationArray to 2D points (ignoring Z axis)
	std::vector<std::array<float, 2>> points2D;
	points2D.reserve(LocationArray.Num());
	for (const FVector& location : LocationArray)
	{
		points2D.push_back({ static_cast<float>(location.X), static_cast<float>(location.Y) });
	}

	// Compute convex hull indices using Graham scan
	std::vector<int> hullIndices;
	if (LocationArray.Num() < 3)
	{
		// If less than 3 points, use all indices
		for (int32 i = 0; i < LocationArray.Num(); ++i)
		{
			hullIndices.push_back(i);
		}
	}
	else
	{
		// Find the bottom-most point (or left-most in case of tie)
		int32 bottom = 0;
		for (int32 i = 1; i < LocationArray.Num(); ++i)
		{
			if (LocationArray[i].Y < LocationArray[bottom].Y || 
				(LocationArray[i].Y == LocationArray[bottom].Y && LocationArray[i].X < LocationArray[bottom].X))
			{
				bottom = i;
			}
		}

		// Build convex hull
		int32 p = bottom;
		do
		{
			hullIndices.push_back(p);
			int32 q = (p + 1) % LocationArray.Num();
			
			for (int32 i = 0; i < LocationArray.Num(); ++i)
			{
				if (orientation(LocationArray[p], LocationArray[i], LocationArray[q]) == 2)
				{
					q = i;
				}
			}
			p = q;
		} while (p != bottom);
	}

	// Call concaveman
	auto concavePoints = concaveman<float, 16>(points2D, hullIndices, 2.0f, 0.0f);

	// Convert back to FVector, preserving original Z values
	HullLocations = Convert2DPointsToFVector(concavePoints, LocationArray);
	
	SplineComponent->ClearSplinePoints();
	for (const FVector& Point : HullLocations)
	{
		SplineComponent->AddSplinePoint(Point, ESplineCoordinateSpace::World);
	}
	SplineComponent->UpdateSpline();

	LocationArray.Empty();
	LocationArray = HullLocations;
}

int32 AZoneMarker::orientation(FVector p, FVector q, FVector r)
{
	float val = (q.Y - p.Y) * (r.X - q.X) -
			(q.X - p.X) * (r.Y - q.Y);

	if (val == 0) return 0;  // collinear
	return (val > 0)? 1: 2; // clock or counterclock wise
}

/**
 * Converts 2D points to FVector array, preserving Z values from original points.
 * @param Points2D The 2D points to convert.
 * @param OriginalPoints The original points to preserve the Z values from.
 * @param Tolerance The tolerance to use for the conversion.
 * @return The FVector array.
 */
TArray<FVector> AZoneMarker::Convert2DPointsToFVector(const std::vector<std::array<float, 2>>& Points2D, const TArray<FVector>& OriginalPoints, float Tolerance)
{
	TArray<FVector> Result;
	Result.Reserve(Points2D.size());
	
	for (const auto& Point2D : Points2D)
	{
		float X = Point2D[0];
		float Y = Point2D[1];
		float Z = 0.0f;
		
		// Try to find exact match first
		bool bFound = false;
		for (const FVector& Original : OriginalPoints)
		{
			if (FMath::Abs(Original.X - X) < Tolerance && FMath::Abs(Original.Y - Y) < Tolerance)
			{
				Z = Original.Z;
				bFound = true;
				break;
			}
		}
		
		// If no exact match, find closest point by 2D distance
		if (!bFound && OriginalPoints.Num() > 0)
		{
			float MinDistSq = MAX_flt;
			int32 ClosestIdx = 0;
			for (int32 i = 0; i < OriginalPoints.Num(); ++i)
			{
				float DistSq = FMath::Square(OriginalPoints[i].X - X) + FMath::Square(OriginalPoints[i].Y - Y);
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					ClosestIdx = i;
				}
			}
			Z = OriginalPoints[ClosestIdx].Z;
		}
		
		Result.Add(FVector(X, Y, Z));
	}
	
	return Result;
}