#include "TPhysicsgraph.h"
#include "TPhysicsNode.h"
#include "TPhysicsNodeSphere.h"
#include "TJoint.h"
#include "TLPhysics.h"
#include "TLPhysicsInline.h"

#include <TootleMaths/TShapePolygon.h>
#include <TootleCore/TLTime.h>

#include <box2d/include/box2d.h>


#define MAX_PHYSICS_TIMESTEP	0.3f	//	max step is 20/60 ish...


#define BOX2D_VELOCITY_ITERATIONS		1	//	movement iterations
#define BOX2D_POSITION_ITERATIONS		10	//	constraint/collision/restitution iterations

#ifdef _DEBUG
	//#define DEBUG_PRINT_COLLISIONS
#endif


namespace TLPhysics
{
	TPtr<TPhysicsgraph> g_pPhysicsgraph;

	Bool		Debug_CheckCollisionObjects(TLPhysics::TPhysicsNode* pNodeA,TLPhysics::TPhysicsNode* pNodeB);
}



SyncBool TLPhysics::TPhysicsgraph::Initialise()
{
	if ( TLGraph::TGraph<TLPhysics::TPhysicsNode>::Initialise() == SyncFalse )
		return SyncFalse;

	//	create generic render node factory
	TPtr<TNodeFactory<TLPhysics::TPhysicsNode> > pFactory = new TPhysicsNodeFactory();
	AddFactory(pFactory);

	return SyncTrue;
}

SyncBool TLPhysics::TPhysicsgraph::Shutdown()
{
	TLDebug_Print("Physicsgraph shutdown");

	if ( TLGraph::TGraph<TLPhysics::TPhysicsNode>::Shutdown() == SyncFalse )
		return SyncFalse;

	if(m_pWorld)
	{
		delete m_pWorld;
		m_pWorld = NULL;
	}

	return SyncTrue;
}

	
void TLPhysics::TPhysicsgraph::UpdateGraph(float fTimeStep)
{
	TLTime::TScopeTimer Timer( TRef_Static(P,h,y,s,c) );

	TLMaths::Limit( fTimeStep, 0.f, MAX_PHYSICS_TIMESTEP );

	//	create queued joints
	for ( s32 j=m_NodeJointQueue.GetLastIndex();	j>=0;	j-- )
	{
		SyncBool Result = CreateJoint( m_NodeJointQueue[j] );

		//	keep the joint in the queue if it returns wait
		if ( Result != SyncWait )
			m_NodeJointQueue.RemoveAt( j );
	}

	//	process the refilters
	//	gr: do this before the graph structure change - this way we can assume the shapes are all still valid
	for ( u32 i=0;	i<m_RefilterQueue.GetSize();	i++ )
	{
		m_pWorld->Refilter( m_RefilterQueue[i] );
	}
	m_RefilterQueue.Empty();

	// Process all queued messages first
	ProcessMessageQueue();
	
	//	no root? nothing to do
	TLPhysics::TPhysicsNode& RootNode = GetRootNode();
	//	do update
	{
		TLTime::TScopeTimer Timer( TRef_Static(p,h,u,p,d) );
		RootNode.UpdateAll( fTimeStep );
	}
	
	//	do box2d world step
	{
		TLTime::TScopeTimer Timer( TRef_Static(C,o,l,l,i) );
		
		//	box2d prefers fixed timesteps... lets see how our variable rate goes...
		float timeStep = fTimeStep;			//	1.0f / 60.0f;
		if ( m_pWorld )
		{
			m_pWorld->Step( timeStep, BOX2D_VELOCITY_ITERATIONS, BOX2D_POSITION_ITERATIONS );
		}
	}
	
	
	//	do post update
	{
		TLTime::TScopeTimer Timer( TRef_Static(p,p,o,s,t) );
		RootNode.PostUpdateAll( fTimeStep, *this );
	}

	//	gr: if this is before the world step, post updates etc, for some reason my 
	//	render nodes don't show up (physics node doesn't MOVE i suppose and scene node 
	//	needs an initial transform message...? seems odd to be dependant on the physics node)
	// Update the graph structure with any changes
	UpdateGraphStructure();

	//	debug collision count
#ifdef _DEBUG
#ifdef ENABLE_COLLISIONTEST_TRACE
	float ZoneTestSuccess = m_Debug_InZoneTests == 0 ? 0.f : (float)m_Debug_InZoneTestsFailed / (float)m_Debug_InZoneTests;
	float IntersectionRate = m_Debug_CollisionTestCount == 0 ? 0.f : (float)m_Debug_CollisionIntersections / (float)m_Debug_CollisionTestCount;
	float CollisionRateUp = m_Debug_CollisionTestCount == 0 ? 0.f : (float)m_Debug_CollisionTestsUpwards / (float)m_Debug_CollisionTestCount;
	float CollisionRateDown = m_Debug_CollisionTestCount == 0 ? 0.f : (float)m_Debug_CollisionTestsDownwards / (float)m_Debug_CollisionTestCount;
	//TLDebug_Print( TString("Collision tests: %d (%d static, %.2f up, %.2f down) %d intersections(%.2f%%) %d zone shape tests(%.2f%%) %d zone loops", m_Debug_CollisionTestCount, m_Debug_StaticCollisionTestCount, CollisionRateUp, CollisionRateDown, m_Debug_CollisionIntersections, IntersectionRate*100.f, m_Debug_InZoneTests, ZoneTestSuccess*100.f, m_Debug_ZonesTested ) );
#endif
#endif
}


//----------------------------------------------------------
//
//----------------------------------------------------------
void TLPhysics::TPhysicsgraph::OnNodeRemoving(TLPhysics::TPhysicsNode& Node)
{
	//	inherited OnRemoving()
	TLGraph::TGraph<TLPhysics::TPhysicsNode>::OnNodeRemoving( Node );
}


//----------------------------------------------------------
//
//----------------------------------------------------------
void TLPhysics::TPhysicsgraph::OnNodeAdded(TLPhysics::TPhysicsNode& Node,Bool SendAddedMessage)
{
	//	inherited OnAdded()
	//	gr: do the inherited OnNodeAdded first, this means the Initialise() will be called BEFORE we
	//		try to add it to the zone. ie. initialise collision shape from Init before using shape tests in zone code
	//		if this is an issue for something, then just manually call ProcessMessageQueue and then move this back
	//		to the end of OnNodeAdded
	//	gr: DONT send the OnNodeAdded message here, we send it after we've created our body
	TLGraph::TGraph<TLPhysics::TPhysicsNode>::OnNodeAdded( Node, FALSE );

#ifdef _DEBUG
	TDebugString Debug_String;
	Debug_String << "Added node to physics graph: " << Node.GetNodeRef();
	TLDebug_Print( Debug_String );
#endif

	//	create box2d body
	if ( m_pWorld )
	{
		Node.CreateBody( *m_pWorld );
	}

	// Send the added node notificaiton
	if ( SendAddedMessage )
	{
		TLMessaging::TMessage Message( TRef_Static(N,o,d,e,A), GetGraphRef());
		Message.Write( Node.GetNodeRef() );
		Message.ExportData( TRef_Static4(N,o,d,e), Node.GetNodeRef() );

		PublishMessage(Message);
	}
}


//----------------------------------------------------------
//	set a new root collision zone
//----------------------------------------------------------
void TLPhysics::TPhysicsgraph::SetRootCollisionZone(TPtr<TLMaths::TQuadTreeZone>& pZone,Bool AllowSleep)
{
	//	create box2d world[bounds]
	if ( pZone )
	{
		m_WorldShape = pZone->GetShape();

		b2AABB WorldBox;
		WorldBox.lowerBound.Set( m_WorldShape.GetLeft(), m_WorldShape.GetTop() );
		WorldBox.upperBound.Set( m_WorldShape.GetRight(), m_WorldShape.GetBottom() );

		//	gravity is I think is still meters/sec
		//float2 Gravity = g_WorldUpNormal.xy() * TLPhysics::g_GravityMetresSec;
		
		//	gr: gravity is applied by node, not by system
		float2 Gravity( 0.f, 0.f );

		// World already exists?
		if(m_pWorld)
		{
			delete m_pWorld;
			m_pWorld = NULL;
		}

		//	create box2d world with the shape of our zone
		//	gr: note our world up is opposite to box2d...
		m_pWorld = new b2World( WorldBox, b2Vec2( Gravity.x, -Gravity.y ), AllowSleep );

		//	set contact filter & listener
		m_pWorld->SetContactFilter( this );
		m_pWorld->SetContactListener( this );
	}
	else
	{
		if(m_pWorld)
		{
			//	delete old world
			delete m_pWorld;
			m_pWorld = NULL;
		}
	}
}


//-------------------------------------------------
//	world up has changed, recalc the normal
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::CalcWorldUpNormal()
{
	//	just for now, we use a float2
	float2 NewNormal( g_WorldUp.x, g_WorldUp.y );
	float NormalLenSq = NewNormal.LengthSq();
	
	//	new values are too small, dont change
	if ( NormalLenSq < TLMaths_NearZero )
		return;
	
	//	valid values, normalise
	NewNormal.Normalise( TLMaths::Sqrtf(NormalLenSq), 1.f );
	g_WorldUpNormal.x = NewNormal.x;
	g_WorldUpNormal.y = NewNormal.y;
	g_WorldUpNormal.z = 0.f;
}


//-------------------------------------------------
//	create joint. if Wait then we're waiting for a node to be created still
//-------------------------------------------------
SyncBool TLPhysics::TPhysicsgraph::CreateJoint(const TJoint& Joint)
{
	//	must have a world
	if ( !m_pWorld )
	{
		TLDebug_Break("world expected when adding a joint");
		return SyncFalse;
	}

	//	make sure joint doesnt already exist
	for ( s32 j=m_NodeJoints.GetLastIndex();	j>=0;	j-- )
	{
		TJoint& ExistingJoint = m_NodeJoints[j];

		//	check for matching joint
		if ( ( ExistingJoint.m_NodeA == Joint.m_NodeA && ExistingJoint.m_NodeB == Joint.m_NodeB ) ||
			 ( ExistingJoint.m_NodeA == Joint.m_NodeB && ExistingJoint.m_NodeB == Joint.m_NodeA ) )
		{
			return SyncFalse;
		}
	}


	//	make new joint 
	TJoint NewJoint = Joint;

	//	create joint in world
	SyncBool CreateResult = NewJoint.CreateJoint( *m_pWorld, *this );
	if ( CreateResult != SyncTrue )
		return CreateResult;

	//	add joint to list
	m_NodeJoints.Add( NewJoint );

	return SyncTrue;
}


//-------------------------------------------------
//	remove joint between these two nodes
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::RemoveJoint(TRefRef NodeA,TRefRef NodeB)
{
	if ( !m_pWorld )
	{
		TLDebug_Break("World expected when removing joints. Joints must be invalid if world was deleted");
		return;
	}

	for ( s32 j=m_NodeJoints.GetLastIndex();	j>=0;	j-- )
	{
		TJoint& Joint = m_NodeJoints[j];

		//	joint includes these nodes together
		if ( ( Joint.m_NodeA == NodeA && Joint.m_NodeB == NodeB ) ||
			 ( Joint.m_NodeA == NodeB && Joint.m_NodeB == NodeA ) )
		{
			//	destroy node
			Joint.DestroyJoint( *m_pWorld );

			//	remove joint from list
			m_NodeJoints.RemoveAt( j );

			//	gr: can only have 1 link between 2 nodes so won't be any more matches
			return;
		}
	}
}


//-------------------------------------------------
//	remove all joints involving this node
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::RemoveJoint(TRefRef NodeA)
{
	if ( !m_pWorld )
	{
		TLDebug_Break("World expected when removing joints. Joints must be invalid if world was deleted");
		return;
	}

	for ( s32 j=m_NodeJoints.GetLastIndex();	j>=0;	j-- )
	{
		TJoint& Joint = m_NodeJoints[j];

		//	joint involves this node
		if ( Joint.m_NodeA == NodeA || Joint.m_NodeB == NodeA )
		{
			//	destroy node
			Joint.DestroyJoint( *m_pWorld );

			//	remove joint from list
			m_NodeJoints.RemoveAt( j );
		}
	}

}


void TLPhysics::TPhysicsgraph::GetNodesInShape(const TLMaths::TShapePolygon2D& Shape,TPointerArray<TLPhysics::TPhysicsNode>& NearPhysicsNodes)
{
	//	convert to box and do test
	TLMaths::TBox2D BoxShape;
	BoxShape.Accumulate( Shape.GetOutline() );

	//	get nodes in box
	GetNodesInShape( BoxShape, NearPhysicsNodes );
}


//-------------------------------------------------
//	get all the nodes in this shape - for spheres optionally do strict sphere checks - box2D uses Boxes for its query so it can return objects outside the sphere. this does an extra loop to make sure distance is within the radius
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::GetNodesInShape(const TLMaths::TSphere2D& Shape,TPointerArray<TLPhysics::TPhysicsNode>& NearPhysicsNodes,Bool StrictSphere)
{
	//	convert to box and do test
	TLMaths::TBox2D BoxShape;
	BoxShape.Accumulate( Shape );

	//	get nodes in box
	GetNodesInShape( BoxShape, NearPhysicsNodes );

	//	get rid of nodes that are outside of the SPHERE (even if they're in the box)
	if ( StrictSphere )
	{
		float RadSq = Shape.GetRadiusSq();
		for ( s32 n=NearPhysicsNodes.GetLastIndex();	n>=0;	n-- )
		{
			//	get distance from center of sphere to node
			TLPhysics::TPhysicsNode* pPhysicsNode = NearPhysicsNodes[n];
			float2 NodeToShape( Shape.GetPos() - pPhysicsNode->GetPosition().xy() );
			float DistSq = NodeToShape.LengthSq();

			//	in box, but outside of sphere's radius
			if ( DistSq > RadSq )
				NearPhysicsNodes.RemoveAt( n );
		}
	}
}


//-------------------------------------------------
//	get all the nodes in this shape
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::GetNodesInShape(const TLMaths::TBox2D& Shape,TPointerArray<TLPhysics::TPhysicsNode>& NearPhysicsNodes)
{
	//	no world to check for bodies
	if ( !m_pWorld )
		return;

	//	make box2D bounds shape
	b2AABB BoxShape;
	BoxShape.lowerBound.x = Shape.GetLeft();
	BoxShape.lowerBound.y = Shape.GetTop();
	BoxShape.upperBound.x = Shape.GetRight();
	BoxShape.upperBound.y = Shape.GetBottom();

	//	get a buffer for shapes's found
	TFixedArray<b2Fixture*,100> ShapeBuffer;
	ShapeBuffer.SetSize( 100 );
	
	//	find bodies
	s32 ShapeCount = m_pWorld->Query( BoxShape, ShapeBuffer.GetData(), ShapeBuffer.GetSize() );
	
	//	correct the valid body count on the array (cull unused entries)
	ShapeBuffer.SetSize( ShapeCount );

	//	get the nodes of each body
	for ( u32 b=0;	b<ShapeBuffer.GetSize();	b++ )
	{
		TPhysicsNode* pNode = TLPhysics::GetPhysicsNodeFromShape( ShapeBuffer[b] );
		if ( !pNode )
		{
			TLDebug_Break("Failed to get node from shape entry from box2d world::query");
			continue;
		}

		//	add node entry to list
		NearPhysicsNodes.Add( pNode );
	}
}



//-----------------------------------------------------
//	custom box2D contact filterer
//-----------------------------------------------------
bool TLPhysics::TPhysicsgraph::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
{
//	const b2FilterData& filter1 = fixtureA->GetFilterData();
//	const b2FilterData& filter2 = fixtureB->GetFilterData();

	//	increment collision test counter
	TLCounter::Debug_Increment( TRef_Static(C,o,T,s,t) );

	TPhysicsNode* pNodeA = TLPhysics::GetPhysicsNodeFromShape( fixtureA );
	TPhysicsNode* pNodeB = TLPhysics::GetPhysicsNodeFromShape( fixtureB );

	//	missing a node, ignore
	if ( !pNodeA || !pNodeB )
	{
		TLDebug_Break("All shapes/bodies should have nodes associated with them...");
		return FALSE;
	}

	//	if either has no collision, then no collision occurs!
	if ( !pNodeA->HasCollision() || !pNodeB->HasCollision() )
		return FALSE;

	//	explicit node-no-collision-with-node check
	if ( !pNodeA->IsAllowedCollisionWithNode( pNodeB->GetNodeRef() ) )
		return FALSE;
	if ( !pNodeB->IsAllowedCollisionWithNode( pNodeA->GetNodeRef() ) )
		return FALSE;

	//	has collision!
	return TRUE;
}




//-------------------------------------------------
//	
//-------------------------------------------------
TLPhysics::TPhysicsNode* TLPhysics::TPhysicsNodeFactory::CreateObject(TRefRef InstanceRef,TRefRef TypeRef)
{
	if ( TypeRef == "Sphere" )
		return new TLPhysics::TPhysicsNodeSphere(InstanceRef,TypeRef);

	return NULL;
}



//-------------------------------------------------
//	new collision with shape
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::BeginContact(b2Contact* contact)
{
	//	get physics nodes for shapes
	b2Fixture* pShapeA = contact->GetFixtureA();
	b2Fixture* pShapeB = contact->GetFixtureB();
	TPhysicsNode* pNodeA = TLPhysics::GetPhysicsNodeFromShape( pShapeA );
	TPhysicsNode* pNodeB = TLPhysics::GetPhysicsNodeFromShape( pShapeB );

	if ( !pNodeA || !pNodeB )
	{
		TLDebug_Break("Collision between shapes missing node[s]");
		return;
	}

	TRef ShapeARef = TLPhysics::GetShapeRefFromShape( pShapeA );
	TRef ShapeBRef = TLPhysics::GetShapeRefFromShape( pShapeB );

#ifdef DEBUG_PRINT_COLLISIONS
	TTempString Debug_String("Collision between ");
	
	//	node(shape)
	pNodeA->GetNodeRef().GetString(Debug_String );
	Debug_String.Append("(");
	ShapeARef.GetString(Debug_String);
	Debug_String.Append(")");
	
	Debug_String.Append(" and ");
	
	pNodeB->GetNodeRef().GetString(Debug_String );
	Debug_String.Append("(");
	ShapeBRef.GetString(Debug_String);
	Debug_String.Append(")");

	TLDebug_Print( Debug_String );
#endif

	u32 ContactPointCount = contact->GetManifold()->m_pointCount;
	
	//	gr: no contact?
	if ( ContactPointCount == 0 )
	{
		TLDebug_Break("No contact points in BeginContact? don't know where collision is");
		return;
	}

	if ( ContactPointCount > 2 )
	{
		TLDebug_Break("More than 2 contact points in the manifold, must be a change to box2d we don't handle");
	}

	//	get world space manifold (world space collision points)
	b2WorldManifold WorldManifold;
	contact->GetWorldManifold( &WorldManifold );

	const b2Vec2& FirstContactLocalPoint = WorldManifold.m_points[0];
	const b2Vec2& SecondContactLocalPoint = (ContactPointCount > 1) ? WorldManifold.m_points[1] : FirstContactLocalPoint;
	
	//	if the Other node is a sensor, (we haven't bounced off of it) then don't count it as a collision, it's a SENSOR for the other node
	if ( !pShapeB->IsSensor() )
	{
		TLPhysics::TCollisionInfo* pCollisionInfo = pNodeA->OnCollision();
		if ( pCollisionInfo )
		{
			pCollisionInfo->m_Intersection = float3( FirstContactLocalPoint.x, FirstContactLocalPoint.y, 0.f );
			pCollisionInfo->m_OtherIntersection = float3( SecondContactLocalPoint.x, SecondContactLocalPoint.y, 0.f );
			pCollisionInfo->m_OtherNode = pNodeB->GetNodeRef();
			pCollisionInfo->m_OtherNodeOwner = pNodeB->GetOwnerSceneNodeRef();
			pCollisionInfo->m_OtherNodeStatic = pNodeB->IsStatic();
			pCollisionInfo->m_IntersectionNormal = float2( WorldManifold.m_normal.x, WorldManifold.m_normal.y );
			pCollisionInfo->m_Shape = ShapeARef;
			pCollisionInfo->m_OtherShape = ShapeBRef;
		}
	}

	//	if the Other node is a sensor, (we haven't bounced off of it) then don't count it as a collision, it's a SENSOR for the other node
	if ( !pShapeA->IsSensor() )
	{
		TLPhysics::TCollisionInfo* pCollisionInfo = pNodeB->OnCollision();
		if ( pCollisionInfo )
		{
			pCollisionInfo->m_Intersection = float3( SecondContactLocalPoint.x, SecondContactLocalPoint.y, 0.f );
			pCollisionInfo->m_OtherIntersection = float3( FirstContactLocalPoint.x, FirstContactLocalPoint.y, 0.f );
			pCollisionInfo->m_OtherNode = pNodeA->GetNodeRef();
			pCollisionInfo->m_OtherNodeOwner = pNodeA->GetOwnerSceneNodeRef();
			pCollisionInfo->m_OtherNodeStatic = pNodeA->IsStatic();
			//	invert normal
			pCollisionInfo->m_IntersectionNormal = float2( -WorldManifold.m_normal.x, -WorldManifold.m_normal.y );

			pCollisionInfo->m_Shape = ShapeBRef;
			pCollisionInfo->m_OtherShape = ShapeARef;
		}
	}



}


//-------------------------------------------------
//	no longer colliding with shape
//-------------------------------------------------
void TLPhysics::TPhysicsgraph::EndContact(b2Contact* contact)
{
	//	get physics node for shape 1
	b2Fixture* pShapeA = contact->GetFixtureA();
	b2Fixture* pShapeB = contact->GetFixtureB();
	TPhysicsNode* pNodeA = TLPhysics::GetPhysicsNodeFromShape( pShapeA );
	TPhysicsNode* pNodeB = TLPhysics::GetPhysicsNodeFromShape( pShapeB );

	if ( !pNodeA || !pNodeB )
	{
		TLDebug_Break("End of collision between shapes missing node[s]");
		return;
	}

	TRef ShapeARef = TLPhysics::GetShapeRefFromShape( pShapeA );
	TRef ShapeBRef = TLPhysics::GetShapeRefFromShape( pShapeB );

#ifdef DEBUG_PRINT_COLLISIONS
	TTempString Debug_String("No more collision between ");
	
	//	node(shape)
	pNodeA->GetNodeRef().GetString(Debug_String );
	Debug_String.Append("(");
	ShapeARef.GetString(Debug_String);
	Debug_String.Append(")");
	
	Debug_String.Append(" and ");
	
	pNodeB->GetNodeRef().GetString(Debug_String );
	Debug_String.Append("(");
	ShapeBRef.GetString(Debug_String);
	Debug_String.Append(")");

	TLDebug_Print( Debug_String );
#endif

	if ( !pShapeB->IsSensor() )
		pNodeA->OnEndCollision( ShapeARef, *pNodeB, ShapeBRef );

	if ( !pShapeA->IsSensor() )
		pNodeB->OnEndCollision( ShapeBRef, *pNodeA, ShapeARef );
}

