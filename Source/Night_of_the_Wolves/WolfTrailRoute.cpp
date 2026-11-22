#include "WolfTrailRoute.h"
#include "Components/SplineComponent.h"

AWolfTrailRoute::AWolfTrailRoute()
{
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("TrailSpline"));
	SetRootComponent(Spline);
}
