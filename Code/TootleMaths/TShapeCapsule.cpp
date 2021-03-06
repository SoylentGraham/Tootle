#include "TShapeCapsule.h"
#include <TootleCore/TBinaryTree.h>




//---------------------------------------
//	create sphere from box
//---------------------------------------
TLMaths::TShapeCapsule2D::TShapeCapsule2D(const TLMaths::TBox2D& Box)
{
	m_Shape.Set( Box );
}


//---------------------------------------
//	
//---------------------------------------
Bool TLMaths::TShapeCapsule2D::ImportData(TBinaryTree& Data)
{
	if ( !Data.ImportData("Start", m_Shape.GetLine().GetStart() ) )	return FALSE;
	if ( !Data.ImportData("End", m_Shape.GetLine().GetEnd() ) )		return FALSE;
	if ( !Data.ImportData("Radius", m_Shape.GetRadius() ) )	return FALSE;

	return TRUE;
}

//---------------------------------------
//	
//---------------------------------------
Bool TLMaths::TShapeCapsule2D::ExportData(TBinaryTree& Data) const
{
	Data.ExportData("Start", m_Shape.GetLine().GetStart() );
	Data.ExportData("End", m_Shape.GetLine().GetEnd() );
	Data.ExportData("Radius", m_Shape.GetRadius() );

	return TRUE;
}



//----------------------------------------------------------
//	
//----------------------------------------------------------
TPtr<TLMaths::TShape> TLMaths::TShapeCapsule2D::Transform(const TLMaths::TTransform& Transform,TPtr<TLMaths::TShape>& pOldShape,Bool KeepShape) const
{
	if ( !m_Shape.IsValid() )
		return NULL;

	//	copy and transform capsule
	TLMaths::TCapsule2D NewCapsule( m_Shape );
	NewCapsule.Transform( Transform );

	//	re-use old shape object
	if ( pOldShape && pOldShape.GetObjectPointer() != this && pOldShape->GetShapeType() == TLMaths::TCapsule2D::GetTypeRef() )
	{
		pOldShape.GetObjectPointer<TShapeCapsule2D>()->SetCapsule( NewCapsule );
		return pOldShape;
	}

	return new TShapeCapsule2D( NewCapsule );
}


//----------------------------------------------------------
//	
//----------------------------------------------------------
Bool TLMaths::TShapeCapsule2D::GetIntersection(TShapeCapsule2D& CollisionShape,TLMaths::TIntersection& NodeAIntersection,TLMaths::TIntersection& NodeBIntersection)
{
	const TLMaths::TCapsule2D& ThisCapsule = this->GetCapsule();
	const TLMaths::TCapsule2D& ShapeCapsule = CollisionShape.GetCapsule();

	float IntersectionAlongThis,IntersectionAlongShape;
	SyncBool IntersectionResult = ThisCapsule.GetLine().GetIntersectionDistance( ShapeCapsule.GetLine(), IntersectionAlongThis, IntersectionAlongShape );

	//	lines of the capsules intersected
	if ( IntersectionResult == SyncTrue )
	{
		/*	//	gr: this gives intersection points at the interesction, which is wrong (we need the intersction on the surface)
		//	get the point along the capsule line where we intersected
		const TLMaths::TLine2D& ThisCapsuleLine = ThisCapsule.GetLine();
		ThisCapsuleLine.GetPointAlongLine( NodeAIntersection.m_Intersection.xy(), IntersectionAlongThis );
		NodeAIntersection.m_Intersection.z = 0.f;

		//	set B's intersection
		const TLMaths::TLine2D& ShapeCapsuleLine = ShapeCapsule.GetLine();
		ShapeCapsuleLine.GetPointAlongLine( NodeBIntersection.m_Intersection.xy(), IntersectionAlongLine );
		NodeBIntersection.m_Intersection.z = 0.f;
		*/
		
		//	get the intersection point
		const TLMaths::TLine2D& ThisCapsuleLine = ThisCapsule.GetLine();
		const TLMaths::TLine2D& ShapeCapsuleLine = ShapeCapsule.GetLine();
		float2 IntersectionPoint;
		ThisCapsuleLine.GetPointAlongLine( IntersectionPoint, IntersectionAlongThis );

		//	work out which capsule we intersected closest to the end of
		//	we use this capsule's direction as our "intersection direction"
		float ThisIntersectionAlongFromMiddle = (IntersectionAlongThis < 0.5f) ? (0.5f - IntersectionAlongThis) : (IntersectionAlongThis - 0.5f);
		float ShapeIntersectionAlongFromMiddle = (IntersectionAlongShape < 0.5f) ? (0.5f - IntersectionAlongShape) : (IntersectionAlongShape - 0.5f);
	
		if ( ThisIntersectionAlongFromMiddle < ShapeIntersectionAlongFromMiddle )
		{
			//	"this interected shape"
			//	use this's direction as the intersection direction
			//	if the intersection is closer to start the direction is from END to START. these lines END at (close to) the intersection
			float2 IntersectionDirectionNormal = (IntersectionAlongThis < 0.5f) ? ( ThisCapsuleLine.GetEnd() - ThisCapsuleLine.GetStart() ).Normal() : ThisCapsuleLine.GetDirectionNormal();

			//	the intersection here will be in-line with the capsule's line (either at the end or the start)
			NodeAIntersection.m_Intersection.xy() = IntersectionPoint;
			NodeAIntersection.m_Intersection.xy() += IntersectionDirectionNormal * ThisCapsule.GetRadius();

			//	this intersection point can be anywhere along the edge of the capsule that was intersected into
			NodeBIntersection.m_Intersection.xy() = IntersectionPoint;
			NodeBIntersection.m_Intersection.xy() -= IntersectionDirectionNormal * ShapeCapsule.GetRadius();
		}
		else
		{
			//	"shape intersected this"
			//	use this's direction as the intersection direction
			//	if the intersection is closer to start the direction is from END to START. these lines END at (close to) the intersection
			float2 IntersectionDirectionNormal = (IntersectionAlongShape < 0.5f) ? ( ShapeCapsuleLine.GetEnd() - ShapeCapsuleLine.GetStart() ).Normal() : ShapeCapsuleLine.GetDirectionNormal();

			//	the intersection here will be in-line with the capsule's line (either at the end or the start)
			NodeBIntersection.m_Intersection.xy() = IntersectionPoint;
			NodeBIntersection.m_Intersection.xy() += IntersectionDirectionNormal * ShapeCapsule.GetRadius();

			//	this intersection point can be anywhere along the edge of the capsule that was intersected into
			NodeAIntersection.m_Intersection.xy() = IntersectionPoint;
			NodeAIntersection.m_Intersection.xy() -= IntersectionDirectionNormal * ThisCapsule.GetRadius();
		}

		//	make sure z's are set
		NodeAIntersection.m_Intersection.z =
		NodeBIntersection.m_Intersection.z = 0.f;
		TLDebug_CheckFloat( NodeAIntersection.m_Intersection );
		TLDebug_CheckFloat( NodeBIntersection.m_Intersection );

		return TRUE;
	}


	const float2* pNearestPointOnThis = NULL;
	const float2* pNearestPointOnShape = NULL;
	float2 TempPoint;

	//	use the capsule distance func to get distance and points
	float DistanceSq = ThisCapsule.GetLine().GetDistanceSq( ShapeCapsule.GetLine(), TempPoint, &pNearestPointOnThis, &pNearestPointOnShape, IntersectionResult, IntersectionAlongThis, IntersectionAlongShape );

	//	too embedded (must be exactly over each other
	if ( DistanceSq < TLMaths_NearZero )
		return FALSE;

	//	too far away, not intersecting
	float TotalRadiusSq = ThisCapsule.GetRadius() + ShapeCapsule.GetRadius();
	TotalRadiusSq *= TotalRadiusSq;
	if ( DistanceSq > TotalRadiusSq )
		return FALSE;

	//	get the vector between the spheres
	float2 Diff( (*pNearestPointOnShape) - (*pNearestPointOnThis) );
	float DiffLength = TLMaths::Sqrtf( DistanceSq );

	//	intersected, work out the intersection points on the surface of the capsule
	float NormalMultA = ThisCapsule.GetRadius() / DiffLength;
	NodeAIntersection.m_Intersection = (*pNearestPointOnThis) + (Diff * NormalMultA);

	float NormalMultB = ShapeCapsule.GetRadius() / DiffLength;
	NodeBIntersection.m_Intersection = (*pNearestPointOnShape) - (Diff * NormalMultB);
	
	//	make sure z's are set
	NodeAIntersection.m_Intersection.z =
	NodeBIntersection.m_Intersection.z = 0.f;

	TLDebug_CheckFloat( NodeAIntersection.m_Intersection );
	TLDebug_CheckFloat( NodeBIntersection.m_Intersection );

	return TRUE;
}



