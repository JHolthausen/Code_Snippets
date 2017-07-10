// Fill out your copyright notice in the Description page of Project Settings.

#include "SpAI_ObjectBaseStatic.h"

ASpAI_ObjectBaseStatic::ASpAI_ObjectBaseStatic() : Super()
{
	SM_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Refference"));
	SM_Mesh->AttachToComponent(root,FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	tolerance = 1.0f;
}

void ASpAI_ObjectBaseStatic::FindNeighborObjects()
{
	UE_LOG(LogTemp, Warning, TEXT("FindNeighborObjects not implemented"));
}



TArray<USpAI_Node*> ASpAI_ObjectBaseStatic::CreateNodes()
{
	return{ nullptr };
}


bool ASpAI_ObjectBaseStatic::CanAddNode(FVector location){return 0; };

void ASpAI_ObjectBaseStatic::Triangulate()
{
	int cntr = 0;
	TArray<AActor*> ignoreActors;

	for (TActorIterator<ASpAI_ObjectBaseStatic> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		AActor* obj = Cast<AActor>(*ActorItr);
		ASpAI_ObjectBaseStatic* a = Cast<ASpAI_ObjectBaseStatic>(*ActorItr);

		if (!obj || obj == this)
			continue;

		ignoreActors.Add(obj);
	}

	for (int32 Index_A = 0; Index_A < gNodes.Num(); Index_A++)
	{
		FVector elementA = gNodes[Index_A]->location;

		for (int32 Index_B = Index_A + 1; Index_B < gNodes.Num(); Index_B++)
		{
			FVector elementB = gNodes[Index_B]->location;

			//point not reachable seek new point
			if (PointReachable(elementA, elementB, {}) == false)
				continue;

			//point is reachable
			//a max of 2 triangles can be created from line a-b
			int triangleCount = 0;

			for (int32 Index_C = Index_B + 1; Index_C < gNodes.Num(); Index_C++)
			{
				FVector elementC = gNodes[Index_C]->location;

				//triangle exists?
				if (PointReachable(elementA, elementC, {}) == false ||
					PointReachable(((elementB - elementA) * 0.5f) + elementA, ((elementC - elementA) * 0.5f) + elementA, {}) == false)
				{
					continue;
				}

				//counter to store amount of time proven this triangle cannot exist, can be max 2 for early out
				int falseTrianglecCntr = 0;
				int quadruplePointIndx = -1;

				//check against other triangles
				for (int32 Index_D = 0; Index_D < gNodes.Num(); Index_D++)
				{
					//exclude triangle nodes
					if (Index_D == Index_A || Index_D == Index_B || Index_D == Index_C)
					{
						continue;
					}
										
					FVector elementD = gNodes[Index_D]->location;

					//is point on the same plane?
					if (PointReachable((elementA - elementB) * 0.5f + elementB, elementD, ignoreActors) == false ||
						PointReachable((elementB - elementC) * 0.5f + elementC, elementD, ignoreActors) == false ||
						PointReachable((elementC - elementA) * 0.5f + elementA, elementD, ignoreActors) == false)
					{
						continue;
					}

					FCircumCircle circle = CreateCircumCircle(elementA, elementB, elementC);

					FVector tmp = elementD - circle.location;
					//is point inside circumcircle
					if (tmp.Size() <= (circle.radius + 2.f))
					{
						/*
						cntr++;
						falseTrianglecCntr = 50;
						
						UE_LOG(LogTemp, Warning, TEXT("MyCharacter's Location is %f"), tmp.Size());

						UE_LOG(LogTemp, Warning, TEXT("MyCharacter's Location is %f"), (circle.radius + 2.f));


						DrawDebugSphere(GetWorld(), circle.location, circle.radius, 50, FColor::Blue, 0, 20, 0, 1);
						DrawDebugPoint(GetWorld(), elementA, 20, FColor::Green, 0, 10, 0);
						DrawDebugPoint(GetWorld(), elementB, 20, FColor::Green, 0, 10, 0);
						DrawDebugPoint(GetWorld(), elementC, 20, FColor::Green, 0, 10, 0);
						DrawDebugPoint(GetWorld(), elementD, 20, FColor::Red,  0, 30, 0);*/
					
						float tb = FVector(elementB - elementA).Size();
						float tc = FVector(elementC - elementA).Size();
						float td = FVector(elementD - elementA).Size();

						bool bIsQuad = false;
						if (tb > tc && tb > td) {
							bIsQuad = IsQaudruple(elementA, elementC, elementB, elementD);
						}
						else if (tc > tb && tc > td) {
							bIsQuad = IsQaudruple(elementA, elementB, elementC, elementD);
						}
						else if (td > tb && td > tc) {
							bIsQuad = IsQaudruple(elementA, elementB, elementD, elementC);
						}

						if (bIsQuad)
						{
							if (falseTrianglecCntr == 0)
							{
								falseTrianglecCntr++; //false triangle but is qaudruple
								quadruplePointIndx = Index_D;
							}
							else
							{
								falseTrianglecCntr = 50; //random number to overflow
							}
						}
						else
						{
							falseTrianglecCntr = 50; //random number to overflow
						}
					}

					//early out
					if (falseTrianglecCntr > 2)
					{
						break;
					}
				}


				//loop c
				if (falseTrianglecCntr < 2)
				{
					if(falseTrianglecCntr == 0)
					{
						UE_LOG(LogTemp, Warning, TEXT("triangle"));
						AddTriangle(Index_A, Index_B, Index_C);
						triangleCount++;
					}
					else if(falseTrianglecCntr == 1) //triangle was false but is qaudruple
					{
						//SplitQuadruple(Index_A, Index_B, Index_C, quadruplePointIndx);
						triangleCount++;

						float tb = FVector(elementB - elementA).Size();
						float tc = FVector(elementC - elementA).Size();
						float td = FVector(gNodes[quadruplePointIndx]->location - elementA).Size();

						if (tb > tc && tb > td) {
							triangleCount++;
						}						
					}
				}

				if (triangleCount >= 2)
					break;
				
			}
			//loop b
		}
		//loop a
	}
}

void ASpAI_ObjectBaseStatic::AddTriangle(int32 a, int32 b, int32 c)
{
	FVector fa = gNodes[a]->location;
	FVector fb = gNodes[b]->location;
	FVector fc = gNodes[c]->location;

	DrawDebugLine(GetWorld(), fa, fb, FColor::Red, 0, 5, 0, 5);
	DrawDebugLine(GetWorld(), fb, fc, FColor::Red, 0, 5, 0, 5);
	DrawDebugLine(GetWorld(), fc, fa, FColor::Red, 0, 5, 0, 5);

	DrawDebugPoint(GetWorld(), (fa + fb + fc) / 3, 20, FColor::Orange, 0, 20, 0);

}

void ASpAI_ObjectBaseStatic::SplitQuadruple(int32 ia, int32 ib, int32 ic, int32 id)
{

	float tb = FVector(gNodes[ib]->location - gNodes[ia]->location).Size();
	float tc = FVector(gNodes[ic]->location - gNodes[ia]->location).Size();
	float td = FVector(gNodes[id]->location - gNodes[ia]->location).Size();

	bool bIsQuad = false;
	if (tb > tc && tb > td) {
		AddTriangle(ia, ic, id);
		AddTriangle(ic, id, ib);
	}
	else if (tc > tb && tc > td) {
		AddTriangle(ia, ib, id);
		AddTriangle(ib, id, ic);
	}
	else if (td > tb && td > tc) {
		AddTriangle(ia, ib, ic);
		AddTriangle(ib, ic, id);
	}

}



bool ASpAI_ObjectBaseStatic::IsQaudruple(FVector a, FVector b, FVector c, FVector d)
{
	float min = 89.f, max = 91.f;
	float angle1 = GetAngle(a, b, d);
	float angle2 = GetAngle(b, c, a);
	float angle3 = GetAngle(c, d, b);
	float angle4 = GetAngle(d, a, c);

	return((angle1 >= min && angle1 <= max) &&
		   (angle2 >= min && angle2 <= max) &&
		   (angle3 >= min && angle3 <= max) &&
		   (angle4 >= min && angle4 <= max));

	return false;
}

float ASpAI_ObjectBaseStatic::GetAngle(FVector start, FVector forward, FVector a)
{
	float angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct((FVector(forward - start)), (FVector(a - start)))));
	return angle;
}

FVector ASpAI_ObjectBaseStatic::GetLongestDistanceVector(FVector start, FVector a, FVector b, FVector c)
{
	float ta = FVector(a - start).Size();
	float tb = FVector(b - start).Size();
	float tc = FVector(c - start).Size();
	float longest = ta >= tb ? ta : tb;
	longest = longest >= tc ? longest : tc;

	if (longest == ta) return a;
	if (longest == tb) return b;
	if (longest == tc) return c;

	return start;
}





bool ASpAI_ObjectBaseStatic::PointReachable(FVector start, FVector end, TArray<AActor*> IgnoreActors)
{
	FHitResult hitOut;

	FCollisionObjectQueryParams CQP;
	FCollisionQueryParams CollisionParams;
	//trace against the mesh not the volume
	CQP.AddObjectTypesToQuery(ECC_WorldDynamic);

	CollisionParams.AddIgnoredActors(IgnoreActors);

	if (GetWorld()->LineTraceSingleByObjectType(
		hitOut,
		start,
		end,
		CQP,
		CollisionParams))
	{
		return false;
	}
	else
	{
		return true;
	}
}

FCircumCircle ASpAI_ObjectBaseStatic::CreateCircumCircle(FVector a, FVector b, FVector c)
{
	FCircumCircle circle;
	//https://gamedev.stackexchange.com/questions/60630/how-do-i-find-the-circumcenter-of-a-triangle-in-3d
	FVector AB = b- a;
	FVector AC = c - a;
	FVector abXac = FVector::CrossProduct(AB,AC);

	// this is the vector from a TO the circumsphere center
	FVector toCircumsphereCenter = (FVector::CrossProduct(abXac, AB)*AC.SizeSquared() + FVector::CrossProduct(AC,abXac)*AB.SizeSquared()) / (2.f*abXac.SizeSquared());
	circle.radius = toCircumsphereCenter.Size();

	// The 3 space coords of the circumsphere center then:
	circle.location = a + toCircumsphereCenter; // now this is the actual 3space location
	return circle;
}
