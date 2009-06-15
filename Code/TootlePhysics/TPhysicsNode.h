/*------------------------------------------------------

	Self contained physics node. Handles physics movement,
	collision etc

-------------------------------------------------------*/
#pragma once

#include <TootleCore/TLGraph.h>
#include <TootleCore/TFlags.h>
#include <TootleCore/TLMaths.h>
#include "TCollisionShape.h"
#include <TootleMaths/TQuadTree.h>
#include "TLPhysics.h"


//namespace Box2D
	#include <box2d/include/box2d.h>

//namespace Box2D
	class b2World;
	class b2Body;


namespace TLMaths
{
	class TSphere;
	class TBox;
	class TCapsule;
};

namespace TLPhysics
{
	class TPhysicsNode;
	class TPhysicsgraph;
	class TJoint;

	extern float3		g_WorldUp;			//	gr: currently a global, change to be on the graph, or per node at some point so individual nodes can have their own gravity direction. Depends on what we need it for
	extern float3		g_WorldUpNormal;	//	gr: currently a global, change to be on the graph, or per node at some point so individual nodes can have their own gravity direction. Depends on what we need it for
	extern float		g_GravityMetresSec;	//	gravity in metres per second (1unit being 1metre)
};

//------------------------------------------------------
//	
//------------------------------------------------------
class TLPhysics::TPhysicsNode : public TLGraph::TGraphNode<TLPhysics::TPhysicsNode>, public TLMaths::TQuadTreeNode
{
	friend class TLPhysics::TPhysicsgraph;
	friend class TLPhysics::TJoint;
public:
	enum Flags
	{
		Flag_HasGravity = 0,	//	has generic gravity force applied
		Flag_RestOnStatic,		//	stick to floor when we hit it (stops tiny vibration bounce)
		Flag_Static,			//	does not move when collided wtih
		//Flag_CollideBothSides,	//	collide with inside of shape, or dont check normal in polygon 
		Flag_HasCollision,		//	expecting a valid collision shape - clear this to DISABLE collision, but still keep shape etc
		Flag_ZoneExpected,		//	expecting to be in a collision zone
		Flag_Enabled,			//	if not enabled, graph does not update this node
		Flag_Rotate,			//	if disabled (on by default) then box2d's collision doesn't rotate objects
	};

public:
	TPhysicsNode(TRefRef NodeRef,TRefRef TypeRef=TRef());

	virtual void				Update(float Timestep);						//	physics update
	virtual void				Shutdown();									//	cleanup
	virtual void				PostUpdate(float Timestep,TLPhysics::TPhysicsgraph& Graph,TPtr<TLPhysics::TPhysicsNode>& pThis);			//	after collisions are handled
	virtual Bool				PostIteration(u32 Iteration);				//	called after iteration, return TRUE to do another iteration

	FORCEINLINE TRefRef			GetOwnerSceneNodeRef() const				{	return m_OwnerSceneNode;	}

	float3						GetPosition() const;
	void						SetPosition(const float3& Position);
	void						MovePosition(const float3& Movement,float Timestep);
	virtual void				SetTransform(const TLMaths::TTransform& NewTransform,Bool PublishChanges=TRUE);	//	explicit change of transform
	const TLMaths::TTransform&	GetTransform() const					{	return m_Transform;	}
	virtual const TLMaths::TTransform&	GetRenderTransform() const		{	return GetTransform();	}

	TFlags<Flags>&				GetPhysicsFlags()					{	return m_PhysicsFlags;	}
	const TFlags<Flags>&		GetPhysicsFlags() const				{	return m_PhysicsFlags;	}
	virtual Bool				IsStatic() const					{	return m_PhysicsFlags.IsSet( TPhysicsNode::Flag_Static );	}
	FORCEINLINE Bool			IsEnabled() const					{	return m_PhysicsFlags.IsSet( TPhysicsNode::Flag_Enabled );	}
	FORCEINLINE void			SetEnabled(Bool Enabled)			{	return m_PhysicsFlags.Set( TPhysicsNode::Flag_Enabled, Enabled );	}

	FORCEINLINE void			AddForce(const float3& Force,Bool MassRelative=FALSE);
	FORCEINLINE void			AddForceFrom(const float3& Force,const float3& ForceLocalPosition,Bool MassRelative=FALSE);
	FORCEINLINE void			AddTorque(float AngleRadians);
	FORCEINLINE void			SetVelocity(const float3& Velocity);
	FORCEINLINE float3			GetVelocity() const;
	FORCEINLINE void			ResetForces();					//	reset all forces to zero
	
	FORCEINLINE void			OnVelocityChanged()					{	SetAccumulatedMovementInvalid();	}
	FORCEINLINE void			OnForceChanged()					{	SetAccumulatedMovementInvalid();	}

	FORCEINLINE void			OnTransformChanged(Bool TransChanged,Bool ScaleChanged,Bool RotationChanged)	{	OnTransformChanged( (TransChanged*TLMaths_TransformBitTranslate) | (ScaleChanged*TLMaths_TransformBitScale) | (RotationChanged*TLMaths_TransformBitRotation) );	}
	FORCEINLINE void			OnTransformChanged(u8 TransformChangedBits)										{	m_TransformChangedBits |= TransformChangedBits;	if ( TransformChangedBits != 0x0 )	SetWorldCollisionShapeInvalid();	}
	FORCEINLINE void			OnTransformChangedNoPublish()		{	SetWorldCollisionShapeInvalid();	}
	FORCEINLINE void			OnTranslationChanged()				{	OnTransformChanged( TLMaths_TransformBitTranslate );	}
	FORCEINLINE void			OnRotationChanged()					{	OnTransformChanged( TLMaths_TransformBitRotation );	}
	FORCEINLINE void			OnScaleChanged()					{	OnTransformChanged( TLMaths_TransformBitScale );	}
	void						PublishTransformChanges();			//	send transform changes as per m_TransformChanges

	Bool								HasCollision() const				{	return (IsEnabled() && HasCollisionFlag() && m_pCollisionShape.IsValid()) ? m_pCollisionShape->IsValid() : FALSE;	}
	Bool								HasCollisionFlag() const			{	return m_PhysicsFlags( Flag_HasCollision );	}
	FORCEINLINE void					EnableCollision(Bool Enable=TRUE)	{	if ( m_PhysicsFlags( Flag_HasCollision ) != Enable )	{	m_PhysicsFlags.Set( Flag_HasCollision, Enable );	OnCollisionEnabledChanged( Enable );	}	}

	FORCEINLINE void					SetCollisionNone()											{	m_pCollisionShape = NULL;	SetWorldCollisionShapeInvalid();	SetCollisionZoneNeedsUpdate();	}
	void								SetCollisionShape(const TPtr<TLMaths::TShape>& pShape);		//	setup collision shape from a shape
	FORCEINLINE TLMaths::TTransform&	GetCollisionShapeTransform()								{	return m_Transform;	}
	FORCEINLINE TPtr<TLMaths::TShape>&	GetCollisionShape()											{	return m_pCollisionShape;	}
	
	//	gr: todo: change all this to handle multiple shapes/bodies more natively, return/get arrays of shapes
	TPtr<TLMaths::TShape>&				GetWorldCollisionShape()				{	return m_pWorldCollisionShape;	}
	const TPtr<TLMaths::TShape>&		GetWorldCollisionShape() const			{	return m_pWorldCollisionShape;	}
	TLMaths::TShape*					CalcWorldCollisionShape();				//	calculate transformed collision shape 
	FORCEINLINE void					SetWorldCollisionShapeInvalid()			{	if ( m_pWorldCollisionShape )	m_pLastWorldCollisionShape = m_pWorldCollisionShape;	m_pWorldCollisionShape = NULL;	}
	virtual Bool						HasMultipleShapes() const				{	return FALSE;	}
	void								GetBodyWorldShapes(TPtrArray<TLMaths::TShape>& ShapeArray);			//	get a more exact array of all the box 2D body shapes

//	Bool						SetCollisionZone(TPtr<TLMaths::TQuadTreeZone>& pCollisionZone,TPtr<TPhysicsNode> pThis,const TFixedArray<u32,4>* pChildZoneList);

	FORCEINLINE void			SetCollisionZoneNeedsUpdate(Bool NeedsUpdate=TRUE)		{	SetZoneOutOfDate( NeedsUpdate );	}
	FORCEINLINE Bool			GetCollisionZoneNeedsUpdate() const						{	return IsZoneOutOfDate();	}
	void						UpdateNodeCollisionZone(TPtr<TLPhysics::TPhysicsNode>& pThis,TLPhysics::TPhysicsgraph& Graph);	//	update what collision zone we're in

	FORCEINLINE Bool			operator==(TRefRef Ref) const							{	return GetNodeRef() == Ref;	}

#ifdef USE_BOX2D
	FORCEINLINE float3			GetForce() const							{	return float3();	}
	FORCEINLINE float3			GetVelocityAndForce() const					{	return float3();	}
	FORCEINLINE void			SetLinearDamping(float Damping);
	FORCEINLINE void			SetAngularDamping(float Damping);
#else
	FORCEINLINE const float3&	GetForce() const							{	return (m_Force);	}
	FORCEINLINE float3			GetVelocityAndForce() const					{	return (m_Velocity + m_Force);	}
#endif
	
	FORCEINLINE float			GetFriction() const;
	FORCEINLINE void			SetFriction(float Friction);
	
protected:
	virtual void				Initialise(TLMessaging::TMessage& Message);	
	void						PostUpdateAll(float Timestep,TLPhysics::TPhysicsgraph& Graph,TPtr<TLPhysics::TPhysicsNode>& pThis);		//	update tree: update self, and children and siblings
	const float3&				GetWorldUp() const							{	return HasParent() ? GetParent()->GetWorldUp() : TLPhysics::g_WorldUpNormal;	}

	FORCEINLINE const float3&	GetAccumulatedMovement()					{	return m_AccumulatedMovementValid ? m_Temp_Intersection.m_Movement : CalcAccumulatedMovement();	}
	FORCEINLINE const float3&	CalcAccumulatedMovement()				 	{	m_AccumulatedMovementValid = TRUE;	return (m_Temp_Intersection.m_Movement = GetVelocityAndForce()*m_Temp_ExtrudeTimestep );	}

	FORCEINLINE void			SetAccumulatedMovementInvalid()				{	m_AccumulatedMovementValid = FALSE;	}
	FORCEINLINE Bool			IsAccumulatedMovementValid() const			{	return m_AccumulatedMovementValid;	}


	virtual Bool				OnCollision(const TPhysicsNode& OtherNode);	//	handle collision with other object
	void						AddCollisionInfo(const TLPhysics::TPhysicsNode& OtherNode,const TLMaths::TIntersection& Intersection);
	void						PublishCollisions();						//	send out our list of collisions
	void						OnCollisionEnabledChanged(Bool IsNowEnabled);	//	called when collision is enabled/disabled - changes group of box2D body so it won't do collision checks

	virtual SyncBool			IsInShape(const TLMaths::TBox2D& Shape);
	virtual Bool				HasZoneShape();								//	return validity of shape for early bail out tests.

	//	box2d interface
	Bool						CreateBody(b2World& World);					//	create the body in the world
	Bool						CreateBodyShape();							//	when our collision shape changes we recreate the shape on the body
	void						SetBodyTransform();							//	reset the body's transform
	FORCEINLINE b2Body*			GetBody()									{	return m_pBody;	}
	FORCEINLINE const b2Body*	GetBody() const								{	return m_pBody;	}
	FORCEINLINE b2Shape*		GetBodyShape()								{	return m_pBody ? m_pBody->GetShapeList() : NULL;	}	//	quick access to the first shape on the body - assuming we only ever have one shape
	FORCEINLINE const b2Shape*	GetBodyShape() const						{	return m_pBody ? m_pBody->GetShapeList() : NULL;	}	//	quick access to the first shape on the body - assuming we only ever have one shape
	virtual void				GetBodys(TArray<b2Body*>& Bodies) const		{	if ( m_pBody )	Bodies.Add( m_pBody );	}

public:
	float					m_Mass;				//	used for varying impact of two objects, larger object bounces less
	float					m_Bounce;			//	elasticity :)
	float					m_Squidge;			//	the amount (factor) the collision shape can be overlapped by (opposite to rigiditty)

protected:
	TLMaths::TTransform		m_Transform;				//	world transform of shape
	u8						m_TransformChangedBits;		//	dont broadcast trasnform changes until post update - TRANSFORM_BIT_XXX

	TFlags<Flags>			m_PhysicsFlags;

#ifndef USE_BOX2D
	float					m_Friction;			//	
	float3					m_Force;
	float3					m_Velocity;
#endif

	TPtr<TLMaths::TShape>	m_pCollisionShape;			//	collision shape
	TPtr<TLMaths::TShape>	m_pWorldCollisionShape;		//	transformed collision shape, delete/invalidated when pos or collision shape changes
	TPtr<TLMaths::TShape>	m_pLastWorldCollisionShape;	//	to save re-allocations of the same object, when we invalidate the world collision shape we set it to this. then when we recalc it, we try to reuse this pointer
	TArray<TCollisionInfo>	m_Collisions;				//	list of collisions during our last update - published in PostUpdate to subscribers

	Bool					m_InitialisedZone;			//	

	float					m_Temp_ExtrudeTimestep;		//	timestep for this frame... just saves passing around, used when calculating world collision shape for this frame
	TLMaths::TIntersection	m_Temp_Intersection;		//	current intersection. assume is invalid unless we're in an OnCollision func
	Bool					m_AccumulatedMovementValid;	//	accumulated movement float3 is now in m_Temp_Intersection, this bool dictates if it needs to be updated

	TRef					m_OwnerSceneNode;			//	"Owner" scene node - if this is set then we automaticcly process some stuff

	b2Body*					m_pBody;					//	box2d body
};



FORCEINLINE void TLPhysics::TPhysicsNode::AddForce(const float3& Force,Bool MassRelative)
{
	if ( m_pBody && Force.IsNonZero() )	
	{
#ifdef USE_BOX2D
		//	multiply by the mass if it's not mass relative otherwise box will scale down the effect of the force. 
		//	eg. gravity doesn't want to be mass related otherwise things will fall at the wrong rates
		float Mass = MassRelative ? 1.f : m_pBody->GetMass();

		//	gr: apply the force at the world center[mass center] of the body
		m_pBody->ApplyForce( b2Vec2(Force.x*Mass,Force.y*Mass) , m_pBody->GetWorldCenter() );	
#else
		m_Force += Force;
#endif
		OnForceChanged();
	}
}


FORCEINLINE void TLPhysics::TPhysicsNode::AddTorque(float AngleRadians)
{
	if ( m_pBody && AngleRadians != 0.f )	
	{
#ifdef USE_BOX2D
		//	gr: apply the torque
		m_pBody->ApplyTorque( AngleRadians );	
#endif
	//	OnForceChanged();
	}
}
	
FORCEINLINE void TLPhysics::TPhysicsNode::SetVelocity(const float3& Velocity)	
{
#ifdef USE_BOX2D
	if ( m_pBody )
	{
		m_pBody->SetLinearVelocity( b2Vec2( Velocity.x, Velocity.y ) );
	}
#else
	m_Velocity = Velocity;
#endif

	OnVelocityChanged();
}


FORCEINLINE float3 TLPhysics::TPhysicsNode::GetVelocity() const			
{
#ifdef USE_BOX2D
	if ( m_pBody )
	{
		const b2Vec2& BodyVelocity = m_pBody->GetLinearVelocity();
		return float3( BodyVelocity.x, BodyVelocity.y, 0.f );
	}
	else
	{
		return float3( 0.f, 0.f, 0.f );
	}
#else
	return m_Velocity;
#endif
}


FORCEINLINE void TLPhysics::TPhysicsNode::ResetForces()
{
#ifdef USE_BOX2D
	if ( m_pBody )
	{
		//	gr: quick fudge version
		m_pBody->PutToSleep();
		m_pBody->WakeUp();
	}
#else
	m_Velocity.Set( float3() );
	m_Force.Set( float3() );
#endif
}



FORCEINLINE float TLPhysics::TPhysicsNode::GetFriction() const			
{
#ifdef USE_BOX2D
	const b2Shape* pBodyShape = GetBodyShape();
	return pBodyShape ? pBodyShape->GetFriction() : 0.f;
#else
	return m_Friction;
#endif
}


FORCEINLINE void TLPhysics::TPhysicsNode::SetFriction(float Friction)
{
#ifdef USE_BOX2D
	TLDebug_Break("todo - modify box2d");
	//b2Shape* pBodyShape = GetBodyShape();
	//if ( pBodyShape )
	//	pBodyShape->SetFriction( Friction );
#else
	m_Friction = Friction;
#endif
}

	
FORCEINLINE void TLPhysics::TPhysicsNode::SetLinearDamping(float Damping)
{
#ifdef USE_BOX2D
	if ( m_pBody )
	{
		m_pBody->SetLinearDamping( Damping );
	}
#endif
}


FORCEINLINE void TLPhysics::TPhysicsNode::SetAngularDamping(float Damping)
{
#ifdef USE_BOX2D
	if ( m_pBody )
	{
		m_pBody->SetAngularDamping( Damping );
	}
#endif
}

