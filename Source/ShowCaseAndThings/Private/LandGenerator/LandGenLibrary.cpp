// Fill out your copyright notice in the Description page of Project Settings.


#include "LandGenerator/LandGenLibrary.h"

FVector4 ULandGenLibrary::Test(FVector X)
{
	FVector p = FVector(floor(X.X),floor(X.Y),floor(X.Z));
	FVector w = FVector(FMath::Frac(X.X),FMath::Frac(X.Y),FMath::Frac(X.Z));

	FVector u = w*w*w*(w*(w*6.0-15.0)+10.0);
	FVector du = 30.0*w*w*(w*(w-2.0)+1.0);

	float a = FMath::PerlinNoise3D(p + FVector(0,0,0) );
	float b = FMath::PerlinNoise3D(p + FVector(1,0,0) );
	float c = FMath::PerlinNoise3D(p + FVector(0,1,0) );
	float d = FMath::PerlinNoise3D(p + FVector(1,1,0) );
	float e = FMath::PerlinNoise3D(p + FVector(0,0,1) );
	float f = FMath::PerlinNoise3D(p + FVector(1,0,1) );
	float g = FMath::PerlinNoise3D(p + FVector(0,1,1) );
	float h = FMath::PerlinNoise3D(p + FVector(1,1,1) );

	float k0 =   a;
	float k1 =   b - a;
	float k2 =   c - a;
	float k3 =   e - a;
	float k4 =   a - b - c + d;
	float k5 =   a - c - e + g;
	float k6 =   a - b - e + f;
	float k7 = - a + b + c - d + e - f - g + h;

	FVector test = 2.0* du * FVector( k1 + k4*u.Y + k6*u.Z + k7*u.Y*u.Z,
								 k2 + k5*u.Z + k4*u.X + k7*u.Z*u.X,
								 k3 + k6*u.X + k5*u.Y + k7*u.X*u.Y );
	FVector4 ToReturn;
	ToReturn.X = -1.0 + 2.0*(k0 + k1*u.X + k2*u.Y + k3*u.Z + k4*u.X*u.Y + k5*u.Y*u.Z + k6*u.Z*u.X + k7*u.X*u.Y*u.Z);
	ToReturn.Y = test.X;
	ToReturn.Z = test.Y;
	ToReturn.W = test.Z ;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ToReturn : %s"), *ToReturn.ToString()));
	return ToReturn;
}
