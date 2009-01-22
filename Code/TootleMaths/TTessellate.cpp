#include "TTessellate.h"
#include <TootleAsset/TMesh.h>



namespace TLMaths
{
	namespace TLTessellator
	{
		const u32	g_BezierStepMin = 1;		//	
		const float	g_BezierStepRate = 1.0f;	//	make beizer step count relative to the lngth of the curve.  a bezier every 0.1m
	}
}
	


//-----------------------------------------------------------------
//	based on a distance, work out how many bezier steps to produce
//-----------------------------------------------------------------
u32 TLMaths::GetBezierStepCount(float LineDistance)
{
	float BezierStepsf = (LineDistance / TLMaths::TLTessellator::g_BezierStepRate) + 0.5f;	//	round up
	
	u32 BezierSteps = (u32)BezierStepsf;
	if ( BezierSteps < TLMaths::TLTessellator::g_BezierStepMin )
		BezierSteps = TLMaths::TLTessellator::g_BezierStepMin;
	
	return BezierSteps;
}


TLMaths::TContour::TContour(const TLMaths::TContour& Contour) :
	m_IsClockwise	( Contour.IsClockwise() )
{
	m_Points.Copy( Contour.GetPoints() );
}



TLMaths::TContour::TContour(const TArray<float3>& Contours,const TArray<TLMaths::TContourCurve>* pContourCurves) :
	m_IsClockwise	( TRUE )
{
	u32 n = Contours.GetSize();
	float3 cur = Contours[(n - 1) % n];
	float3 next = Contours[0];
    float3 prev;
    float3 a;
	float3 b = next - cur;
    float olddir, dir = atan2f((next - cur).y, (next - cur).x);
    float angle = 0.0;
	
	
    // See http://freetype.sourceforge.net/freetype2/docs/glyphs/glyphs-6.html
    // for a full description of FreeType tags.
    for( u32 i = 0; i < n; i++)
    {
        prev = cur;
        cur = next;
        next = Contours[(i + 1) % n];
        olddir = dir;
        dir = atan2f( (next - cur).y, (next - cur).x );
		
        // Compute our path's new direction.
        float t = dir - olddir;
        if(t < -PI) t += 2 * PI;
        if(t > PI) t -= 2 * PI;
        angle += t;
		
        // Only process point tags we know.
		if ( !pContourCurves )
		{
			m_Points.Add(cur);
			continue;
		}
		
		TLMaths::TContourCurve CurveType = pContourCurves->ElementAtConst(i);
		TLMaths::TContourCurve NextCurveType = pContourCurves->ElementAtConst( (i + 1) % n );
		TLMaths::TContourCurve PrevCurveType = pContourCurves->ElementAtConst( (i - 1 + n) % n );
		
		if( n < 2 || CurveType == TLMaths::ContourCurve_On )
        {
            m_Points.Add(cur);
        }
        else if( CurveType == TLMaths::ContourCurve_Conic )
        {
            float3 prev2 = prev;
			float3 next2 = next;
			
            // Previous point is either the real previous point (an "on"
            // point), or the midpoint between the current one and the
            // previous "conic off" point.
            if ( PrevCurveType == TLMaths::ContourCurve_Conic)
            {
                prev2 = (cur + prev) * 0.5f;
                m_Points.Add(prev2);
            }
			
            // Next point is either the real next point or the midpoint.
            if ( NextCurveType == TLMaths::ContourCurve_Conic)
            {
                next2 = (cur + next) * 0.5f;
            }
			
            evaluateQuadraticCurve(prev2, cur, next2);
        }
        else if( CurveType == TLMaths::ContourCurve_Cubic && NextCurveType == TLMaths::ContourCurve_Cubic )
        {
			float3 f = Contours[(i + 2) % n];
            evaluateCubicCurve(prev, cur, next, f );
			
			//	gr: the 2nd cubic here should be ignored shouldnt it??
        }
		else
		{
			//	ignore this combination - eg. second cubic in curve
		}
    }
	
    // If final angle is positive (+2PI), it's an anti-clockwise contour,
    // otherwise (-2PI) it's clockwise.
	m_IsClockwise = (angle < 0.0) ? SyncTrue : SyncFalse;
}


void TLMaths::TContour::evaluateQuadraticCurve(const float3& FromOn,const float3& FromControl,const float3& ToOn)
{
	const float3& A = FromOn;
	const float3& B = FromControl;
	const float3& C = ToOn;
	
	//	work out how many bezier steps to do
	float PointDistanceSq = (A - B).LengthSq() + (B - C).LengthSq();
	u32 BezierSteps = GetBezierStepCount( TLMaths::Sqrtf( PointDistanceSq ) );
	
	//	note: starts at 1 as first point is NOT a point on the curve, it's the ON from before
	for( u32 i=1;	i<BezierSteps;	i++)
    {
        float t = static_cast<float>(i) / (float)BezierSteps;
		float invt = 1.f - t;
		
        float3 U = A * invt + B * t;
        float3 V = B * invt + C * t;
		float3 Point = U * invt + V * t;
        
		m_Points.Add( Point );
    }
}


void TLMaths::TContour::evaluateCubicCurve(const float3& FromOn,const float3& FromControl,const float3& ToControl,const float3& ToOn)
{
	const float3& A = FromOn;
	const float3& B = FromControl;
	const float3& C = ToControl;
	const float3& D = ToOn;
	
	//	work out how many bezier steps to do
	float PointDistanceSq = (A - B).LengthSq() + (B - C).LengthSq() + (C - D).LengthSq();
	u32 BezierSteps = GetBezierStepCount( TLMaths::Sqrtf( PointDistanceSq ) );

	//	gr: should this start at 1? Isn't FromOn already in the contour?...
    for( u32 i=0;	i<BezierSteps;	i++ )
    {
        float t = static_cast<float>(i) / (float)BezierSteps;
		float invt = 1.f - t;
		
        float3 U = A * invt + B * t;
        float3 V = B * invt + C * t;
        float3 W = C * invt + D * t;
		
        float3 M = U * invt + V * t;
        float3 N = V * invt + W * t;
		
		float3 Point = M * invt + N * t;
		
        m_Points.Add( Point );
    }
}


//----------------------------------------------------------
//	scale down the shape using outset calculations
//----------------------------------------------------------
void TLMaths::TContour::Shrink(float OutsetDistance)
{
	TArray<float3> OldPoints;
	OldPoints.Copy( m_Points );

	//	get a bounds sphere for the old points and we can use that radius to detect
	//	shrinking intersections (rather than some arbirtry massive length for the line)
	TLMaths::TSphere2D BoundsSphere;
	BoundsSphere.Accumulate( m_Points );
	
	TArray<TLMaths::TLine2D> EdgeLines;
	GetEdgeLines( EdgeLines );

	//	calculate the new outset point for each point
	for ( s32 i=0;	i<(s32)m_Points.GetSize();	i++ )
	{
		OutsetPoint( i, -OutsetDistance, m_Points, OldPoints, IsClockwise() );

		//	check intersection on shrink,
		//	dont allow this line past half way in the shape...
		//	do this by... extending the line until it hits the other side? 
		//	WHEN it does, half the intersection distance, if we go past this point then
		//	snap
		float TestLineLength = (BoundsSphere.GetRadius() * 2.f);
		float2 LineStart( OldPoints[i].xy() );
		float2 LineDir( m_Points[i].x - LineStart.x, m_Points[i].y - LineStart.y );
		LineDir.Normalise();
		TLMaths::TLine2D OutsetLine( LineStart, LineStart );
		OutsetLine.m_End += LineDir * TestLineLength;

		Bool ShortestIntersectionAlongOutsetIsValid = FALSE;
		float ShortestIntersectionAlongOutset = 0.f;

		//	find shortest intersection with the old shape
		for ( u32 e=0;	e<EdgeLines.GetSize();	e++ )
		{
			//	skip check if 
			if ( LineStart == EdgeLines[e].GetStart() )
				continue;
			if ( LineStart == EdgeLines[e].GetEnd() )
				continue;
			/*
			//	edge starts on i
			if ( e == i )	continue;

			//	edge ends on i
			s32 iplus = i+1;
			TLMaths::Wrap( iplus, 0, (s32)m_Points.GetSize() );
			if ( e == iplus )	continue;
			*/

			//	check to see if our new outset intersects with this edge
			float IntersectionAlongOutset,IntersectionAlongEdge;
			if ( !OutsetLine.GetIntersectionPos( EdgeLines[e], IntersectionAlongOutset, IntersectionAlongEdge ) )
				continue;

			//	intersected, check to see if its the new shortest
			if ( !ShortestIntersectionAlongOutsetIsValid || IntersectionAlongOutset<ShortestIntersectionAlongOutset )
			{
				ShortestIntersectionAlongOutsetIsValid = TRUE;
				ShortestIntersectionAlongOutset = IntersectionAlongOutset;
			}
		}

		//	didn't intersect (gr; wierd, I think we always intersect because of how far we test...)
		if ( !ShortestIntersectionAlongOutsetIsValid )
			continue;

		//	intersected, move the outset
		
		//	get how far it is to the other line...
		float IntersectionDist = ( ShortestIntersectionAlongOutset * TestLineLength );

		//	half it, because that's the furthest we'd want to move...
		//	if we hit the edge, we wanna stop half way between our old pos and this edge
		IntersectionDist *= 0.5f;

		//	we weren't going to move that far anyway!
		if ( IntersectionDist > OutsetDistance )
			continue;

		//	get the pos of the edge we hit
		float2 DirToIntersection = LineDir * IntersectionDist;	//	* fraction * len

		m_Points[i].xy() += OldPoints[i].xy() + DirToIntersection;
	}
}


//----------------------------------------------------------
//	scale down the shape using outset calculations
//----------------------------------------------------------
void TLMaths::TContour::Grow(float OutsetDistance)
{
	TArray<float3> OldPoints;
	OldPoints.Copy( m_Points );

	//	calculate the new outset point for each point
	for ( s32 i=0;	i<(s32)m_Points.GetSize();	i++ )
	{
		OutsetPoint( i, OutsetDistance, m_Points, OldPoints, IsClockwise() );
	}

}

//----------------------------------------------------------
//	move point in/out with outset
//	static
//----------------------------------------------------------
void TLMaths::TContour::OutsetPoint(u32 Index,float Distance,TArray<float3>& NewPoints,const TArray<float3>& OriginalPoints,Bool ContourIsClockwise)
{
	s32 IndexPrev = (s32)Index-1;
	s32 IndexNext = Index+1;
	TLMaths::Wrap( IndexPrev, 0, (s32)OriginalPoints.GetSize() );
	TLMaths::Wrap( IndexNext, 0, (s32)OriginalPoints.GetSize() );

	NewPoints[Index] += TLMaths::TContour::ComputeOutsetPoint( OriginalPoints[IndexPrev], OriginalPoints[Index], OriginalPoints[IndexNext], Distance, ContourIsClockwise );
}


// This function is a bit tricky. Given a path ABC, it returns the
// coordinates of the outset point facing B on the left at a distance
// of 64.0.
//                                         M
//                            - - - - - - X
//                             ^         / '
//                             | 64.0   /   '
//  X---->-----X     ==>    X--v-------X     '
// A          B \          A          B \   .>'
//               \                       \<'  64.0
//                \                       \                  .
//                 \                       \                 .
//                C X                     C X
//
//	static
float3 TLMaths::TContour::ComputeOutsetPoint(const float3& A,const float3& B,const float3& C,float Distance,Bool ContourIsClockwise)
{
	TLDebug_CheckFloat( A );
	TLDebug_CheckFloat( B );
	TLDebug_CheckFloat( C );
	
    /* Build the rotation matrix from 'ba' vector */
    float2 ba( A.x-B.x, A.y-B.y );
	ba.Normalise();
    float2 bc( C.x-B.x, C.y-B.y );
	
    /* Rotate bc to the left */
    float2 tmp(bc.x * -ba.x + bc.y * -ba.y,
			   bc.x * ba.y + bc.y * -ba.x );
	
    /* Compute the vector bisecting 'abc' */
	float norm = TLMaths::Sqrtf(tmp.x * tmp.x + tmp.y * tmp.y);
    
	float dist = 0;
	float normplusx = norm + tmp.x;
	float normminusx = norm - tmp.x;
	if ( normplusx != 0.0 )
	{
		float xdiv = normminusx / normplusx;
		if ( xdiv != 0 )
		{
			float sqrtxdiv = TLMaths::Sqrtf(xdiv);
			dist = Distance * sqrtxdiv;
		}
	}

	//	gr: if tmp.y is positive when shrinking a poly, the new point is on the outside instead of inside

    tmp.x = (tmp.y<0.f) ? dist : -dist;
    tmp.y = Distance;
	
    //	Rotate the new bc to the right
	float3 Result( tmp.x * -ba.x + tmp.y * ba.y, 
				  tmp.x * -ba.y + tmp.y * -ba.x,
				  B.z );

	if ( !ContourIsClockwise )
	{
		Result.x *= -1.f;
		Result.y *= -1.f;
	}

	return Result;
}

	
//-------------------------------------------------------------
//	get all the lines around the edge of the contour
//-------------------------------------------------------------
void TLMaths::TContour::GetEdgeLines(TArray<TLMaths::TLine>& EdgeLines) const
{
	//	generate the edge lines
	for ( u32 a=0;	a<m_Points.GetSize();	a++ )
	{
		s32 aa = a+1;
		
		//	loop around
		if ( aa == m_Points.GetSize() )
			aa = 0;

		const float3& PointA = m_Points[a];
		const float3& PointAA = m_Points[aa];

		EdgeLines.Add( TLMaths::TLine( PointA, PointAA ) );
	}
}


	
//-------------------------------------------------------------
//	get all the lines around the edge of the contour
//-------------------------------------------------------------
void TLMaths::TContour::GetEdgeLines(TArray<TLMaths::TLine2D>& EdgeLines) const
{
	//	generate the edge lines
	for ( u32 a=0;	a<m_Points.GetSize();	a++ )
	{
		s32 aa = a+1;
		
		//	loop around
		if ( aa == m_Points.GetSize() )
			aa = 0;

		const float3& PointA = m_Points[a];
		const float3& PointAA = m_Points[aa];

		EdgeLines.Add( TLMaths::TLine2D( PointA, PointAA ) );
	}
}


//-------------------------------------------------------------
//	check to make sure any lines along the contour dont intersect each other (a self intersecting polygon). returns TRUE if they do
//	gr: note: this is 2D only at the moment
//-------------------------------------------------------------
Bool TLMaths::TContour::HasIntersections() const
{
	//	generate the edge lines
	TArray<TLMaths::TLine2D> ContourEdges;
	GetEdgeLines( ContourEdges );

	for ( u32 e=0;	e<ContourEdges.GetSize();	e++ )
	{
		TLMaths::TLine2D& Edgee = ContourEdges[e];

		//	gr: start at +2... we don't check against the line connected directly to us (the end of the line WILL intersect)
		for ( u32 f=0;	f<m_Points.GetSize();	f++ )
		{
			if ( e==f )
				continue;

			TLMaths::TLine2D& Edgef = ContourEdges[f];

			if ( Edgee.GetEnd() == Edgef.GetStart() )	
				continue;
			if ( Edgef.GetEnd() == Edgee.GetStart() )	
				continue;

			if ( Edgee.GetIntersection( Edgef ) )
				return TRUE;
		}
	}

	return FALSE;

}



void TLMaths::TContour::SetParity(u32 parity)
{
    u32 size = m_Points.GetSize();
	
    if(((parity & 1) && IsClockwise()) || (!(parity & 1) && !IsClockwise()))
    {
        // Contour orientation is wrong! We must reverse all points.
        // FIXME: could it be worth writing FTVector::reverse() for this?
        for ( u32 i=0;	i<size/2;	i++ )
        {
			//	gr: use swap func
			m_Points.SwapElements( i, size - 1 - i );
            //float3 tmp = m_Points[i];
            //m_Points[i] = m_Points[size - 1 - i];
            //m_Points[size - 1 -i] = tmp;
        }
		
		m_IsClockwise = !m_IsClockwise;
    }

/*
    for ( u32 i=0;	i<size;	i++ )
    {
        u32 prev, cur, next;
		
        prev = (i + size - 1) % size;
        cur = i;
        next = (i + size + 1) % size;
		
		const float3& a = Point(prev);
		const float3& b = Point(cur);
		const float3& c = Point(next);
        float3 vOutset = ComputeOutsetPoint( a, b, c, 64.f );
        AddOutsetPoint(vOutset);
    }
	*/
}


//-----------------------------------------------
//	get area of the shape
//-----------------------------------------------
float TLMaths::TContour::GetArea() const
{
	float Area = 0.f;

	for ( s32 i=0;	i<m_Points.GetLastIndex();	i++ )
	{
		Area += ( m_Points[i+1].x * m_Points[i].y - m_Points[i].x * m_Points[i+1].y) * 0.5f;
	}

	//	if the shape is clockwise, Area will be negative, so abs it
	return TLMaths::Absf( Area );
}



TLMaths::TTessellator::TTessellator(TPtr<TLAsset::TMesh>& pMesh) : 
	m_pMesh				( pMesh ),
	m_VertexColourValid	( FALSE )
{
}













TLMaths::TSimpleTessellator::TSimpleTessellator(TPtr<TLAsset::TMesh>& pMesh) :
	TLMaths::TTessellator	( pMesh )
{
}



Bool TLMaths::TSimpleTessellator::GenerateTessellations(TLMaths::TLTessellator::TWindingMode WindingMode,float zNormal)
{
	return FALSE;
}

