/*------------------------------------------------------

	Self contained physics node. Handles physics movement,
	collision etc

-------------------------------------------------------*/
#pragma once

#include <TootleCore/TLGraph.h>
#include <TootleCore/TFlags.h>
#include <TootleCore/TLMaths.h>
#include <TootleCore/TTransform.h>

#include "TCollisionShape.h"

#include <box2d/source/common/b2Settings.h> // required for float32

#include "TCollisionInfo.h"

// Forward declarations
class b2World;
class b2Fixture;
class b2Body;
struct b2Vec2;


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
	class TCollisionShape;					//	shape with a ref
	class TCollisionInfo;

	extern float3		g_WorldUp;			//	gr: currently a global, change to be on the graph, or per node at some point so individual nodes can have their own gravity direction. Depends on what we need it for
	extern float3		g_WorldUpNormal;	//	gr: currently a global, change to be on the graph, or per node at some point so individual nodes can have their own gravity direction. Depends on what we need it for
	extern float		g_GravityMetresSec;	//	gravity in metres per second (1unit being 1metre)
};


//------------------------------------------------------
//	
//------------------------------------------------------
class TLPhysics::TPhysicsNode : public TLGraph::TGraphNode<TLPhysics::TPhysicsNode>
{
	friend class TLPhysics::TPhysicsgraph;
	friend class TLPhysics::TJoint;
	
public:
	struct Properties : public TLMaths::TTransform::Properties
	{
		enum
		{
			Owner			= TRef_Static(O,w,n,e,r),
			Type			= TRef_Static4(T,y,p,e),
			IsStatic		= TRef_Static(f,S,t,a,t),
			AllowRotate		= TRef_Static(f,R,o,t,a),
			HasGravity		= TRef_Static(f,G,r,a,v),
			PhysicsFlags	= TRef_Static(P,F,l,a,g),
			PhysicsFlagsOn	= TRef_Static(P,F,S,e,t),
			PhysicsFlagsOff	= TRef_Static(P,F,C,l,e),
			Friction		= TRef_Static(F,r,i,c,t),
			Bounce			= TRef_Static(B,o,u,n,c),
			Damping			= TRef_Static(D,a,m,p,i),
			RelativeMovement	= TRef_Static(R,e,l,M,o),
			CollisionDatum		= TRef_Static(c,o,l,d,a),
			CollisionDatum_MeshRef	= TRef_Static(M,e,s,h,R),
			CollisionShape		= TRef_Static(c,o,l,s,h),
			CollisionShape_Ref	= TRef_Static3(R,e,f),
			CollisionShape_IsSensor	= TRef_Static(S,e,n,s,o),
			Joint				= TRef_Static(J,o,i,n,t),
			Joint_Node			= TRef_Static4(N,o,d,e),
			Joint_AnchorPos		= TRef_Static(A,n,c,P,o),
			Joint_NodeAnchorPos	= TRef_Static(O,t,h,A,n),
		};
	};
	
public:
	enum Flags
	{
		Flag_HasGravity = 0,	//	has generic gravity force applied
		Flag_RestOnStatic,		//	stick to floor when we hit it (stops tiny vibration bounce)
		Flag_Static,			//	does not move when collided wtih
		Flag_HasCollision,		//	expecting a valid collision shape - clear this to DISABLE collision, but still keep shape etc
		Flag_Enabled,			//	if not enabled, graph does not update this node
		Flag_Rotate,			//	if disabled (on by default) then box2d's collision doesn't rotate objects
		Flag_UseOwnGravity,		//	use the gravity vector on the node rather than the defualt global one on the graph
		Flag_SmoothTranslateChanges,	//	if flaged then we use much more precise detection for translate changes (stops jitter in movement)
		Flag_SmoothRotationChanges,		//	if flaged then we use much more precise detection for rotation changes (stops jitter in rotation)
	};

public:
	TPhysicsNode(TRefRef NodeRef,TRefRef TypeRef=TRef());

	virtual void				Update(float Timestep);						//	physics update
	virtual void				Shutdown();									//	cleanup
	virtual void				PostUpdate(float Timestep,TLPhysics::TPhysicsgraph& Graph);			//	after collisions are handled
	virtual void				UpdateNodeData();							//	exposed public for the SceneNode_Object access when serialising

	FORCEINLINE TRefRef			GetOwnerSceneNodeRef() const				{	return m_OwnerSceneNode;	}

	FORCEINLINE const float3&	GetPosition() const							{	return GetTransform().GetTranslate();	}
	void						SetPosition(const float3& Position);
	virtual void				SetTransform(const TLMaths::TTransform& NewTransform,Bool PublishChanges=TRUE);	//	explicit change of transform
	const TLMaths::TTransform&	GetTransform() const					{	return m_Transform;	}
	virtual const TLMaths::TTransform&	GetRenderTransform() const		{	return GetTransform();	}

	FORCEINLINE Bool			IsEnabled() const					{	return m_PhysicsFlags.IsSet( TPhysicsNode::Flag_Enabled );	}
	FORCEINLINE void			SetEnabled(Bool Enabled);			//	enable/disable node (physics processing and collision). Disabling makes it invisible to the box world
	TFlags<Flags>&				GetPhysicsFlags()					{	return m_PhysicsFlags;	}
	const TFlags<Flags>&		GetPhysicsFlags() const				{	return m_PhysicsFlags;	}
	virtual Bool				IsStatic() const					{	return m_PhysicsFlags.IsSet( TPhysicsNode::Flag_Static );	}

	void						AddForce(const float3& Force,Bool MassRelative=FALSE);			//	apply a force to the body
	void						AddMovement(const float3& Movement,Bool MassRelative=FALSE);	//	apply a linear movement to the body (impulse)
	FORCEINLINE void			AddTorque(float AngleRadians);
	FORCEINLINE void			SetVelocity(const float3& Velocity);
	FORCEINLINE float3			GetVelocity() const;
	FORCEINLINE void			ResetForces();					//	reset all forces to zero
	
	FORCEINLINE void			OnFrictionChanged()					{	OnShapeDefintionChanged();	}
	FORCEINLINE void			OnBounceChanged()					{	OnShapeDefintionChanged();	}
	FORCEINLINE void			OnDampingChanged();
	void						OnShapeDefintionChanged();

	virtual void				OnTransformChanged(u8 TransformChangedBits)					{	m_TransformChangedBits |= TransformChangedBits;	if ( TransformChangedBits != 0x0 )	SetBodyTransform( TransformChangedBits );	}
	virtual void				OnTransformChangedNoPublish(u8 TransformChangedBits)		{	SetBodyTransform(TransformChangedBits);	}
	FORCEINLINE void			OnTranslationChanged()				{	OnTransformChanged( TLMaths_TransformBitTranslate );	}
	FORCEINLINE void			OnRotationChanged()					{	OnTransformChanged( TLMaths_TransformBitRotation );	}
	FORCEINLINE void			OnScaleChanged()					{	OnTransformChanged( TLMaths_TransformBitScale );	}
	void						PublishTransformChanges();			//	send transform changes as per m_TransformChanges

	FORCEINLINE Bool			HasCollision() const										{	return m_PhysicsFlags( Flag_HasCollision );	}
	FORCEINLINE void			EnableCollision(Bool Enable=TRUE);							//	enable collision, regardless of existance of shape
	FORCEINLINE Bool			IsAllowedCollisionWithNode(TRefRef NodeRef)					{	return !m_NonCollisionNodes.Exists( NodeRef );	}
	FORCEINLINE void			EnableCollisionWithNode(TRefRef NodeRef,Bool Enable)		{	if ( Enable )	m_NonCollisionNodes.Remove( NodeRef );	else	m_NonCollisionNodes.AddUnique( NodeRef );	}

	void						SetCollisionNone();
	FORCEINLINE TRef			SetCollisionShape(const TPtr<TLMaths::TShape>& pShape)						{	return AddCollisionShape( pShape, FALSE, TRef() );	}	//	wrapper for new function
	TRef						AddCollisionShape(const TPtr<TLMaths::TShape>& pShape,Bool IsSensor,TRef ShapeRef);	//	setup collision shape from a shape, add to list, replace existing shape if it already exists
	FORCEINLINE Bool			RemoveCollisionShape(TRefRef ShapeRef)										{	TCollisionShape* pCollisonShape = GetCollisionShape( ShapeRef );	return pCollisonShape ? RemoveCollisionShape( *pCollisonShape ) : FALSE;	}
	Bool						RemoveCollisionShape(TCollisionShape& CollisionShape);						//	remove a collision shape, and it's body shape from the body. returns false if doesn't exist
	TCollisionShape*			GetCollisionShape(TRefRef ShapeRef)											{	return m_CollisionShapes.Find( ShapeRef );	}
	void						GetCollisionShapesWorld(TPtrArray<TLMaths::TShape>& ShapeArray) const;		//	convert the body shapes to native TShapes in world space - these only return the box2D shapes - circle and polygon
	const TArray<TCollisionShape>&	GetCollisionShapesLocal() const											{	return m_CollisionShapes;	}	//	get all the body shapes in their original local shape (note: also UNSCALED)

	FORCEINLINE float3			GetForce() const							{	return float3();	}
	FORCEINLINE float3			GetVelocityAndForce() const					{	return float3();	}
	FORCEINLINE void			SetLinearDamping(float Damping);
	FORCEINLINE void			SetAngularDamping(float Damping);
	
	FORCEINLINE float			GetFriction() const							{	return m_Friction;	}
	FORCEINLINE void			SetFriction(float Friction);

	FORCEINLINE float			GetBounce() const							{ return m_Bounce;	}
	FORCEINLINE void			SetBounce(float fBounce)					{ m_Bounce = fBounce; }

	FORCEINLINE Bool			operator<(TRefRef NodeRef) const			{	return GetNodeRef() < NodeRef;	}
	FORCEINLINE Bool			operator==(TRefRef NodeRef) const			{	return GetNodeRef() == NodeRef;	}
	FORCEINLINE Bool			operator==(const TPhysicsNode& That) const	{	return GetNodeRef() == That.GetNodeRef();	}
	FORCEINLINE Bool			operator<(const TSorter<TPhysicsNode*,TRef>& That) const	{	return GetNodeRef() < That.This()->GetNodeRef();	}
	FORCEINLINE Bool			operator<(const TSorter<TPhysicsNode,TRef>& That) const	{	return GetNodeRef() < That->GetNodeRef();	}

protected:
	virtual void				Initialise(TLMessaging::TMessage& Message);	
	virtual void				SetProperty(TLMessaging::TMessage& Message);	
	virtual void				ProcessMessage(TLMessaging::TMessage& Message);
	void						PostUpdateAll(float Timestep,TLPhysics::TPhysicsgraph& Graph);		//	update tree: update self, and children and siblings

	TCollisionInfo*				OnCollision();											//	called when we get a collision. return a collision info to write data into. return NULL to pre-empt not sending any collision info out (eg. if no subscribers)
	void						OnEndCollision(TRefRef ShapeRef,TLPhysics::TPhysicsNode& OtherNode,TRefRef OtherShapeRef);	//	called when we are no longer colliding with a node
	void						PublishCollisions();									//	send out our list of collisions (and end collisions)
	void						OnCollisionEnabledChanged(Bool IsNowEnabled);			//	called when collision is enabled/disabled - changes group of box2D body so it won't do collision checks
	void						OnNodeEnabledChanged(Bool IsNowEnabled);				//	called when node is enabled/disabled
	void						OnCollisionShapeAdded(TCollisionShape& CollisionShape);	//	collision shape added to the list & body
	void						OnCollisionShapeChanged(TCollisionShape& CollisionShape);	//	collision shape changed
	void						OnCollisionShapeRemoved(TRefRef CollisionShapeRef);		//	collision shape added to the list & body

	Bool						ImportCollisionShape(TBinaryTree& CollisionShapeData);	//	create a collision shape from init data
	Bool						ImportCollisionShapeFromDatum(TBinaryTree& CollisionShapeData);	//	create a collision shape from init data

	//	box2d interface
	Bool						CreateBody(b2World& World);							//	create the body in the world
	SyncBool					CreateBodyShape(TCollisionShape& CollisionShape);	//	create/recreate this shape on the body. if FALSE - it failed, if WAIT then we're waiting for a body to add it to
	b2Fixture*					GetBodyShape(TRefRef ShapeRef);						//	get the body shape (fixture) for this shape 
	FORCEINLINE b2Fixture*		GetBodyShape(const TCollisionShape& CollisionShape)	{	return GetBodyShape( CollisionShape.GetShapeRef() );	}
	FORCEINLINE b2Body*			GetBody()											{	return m_pBody;	}
	FORCEINLINE const b2Body*	GetBody() const										{	return m_pBody;	}
	virtual void				OnBodyTransformChanged(u8 TransformChangedBits)		{	m_TransformChangedBits |= TransformChangedBits;	}
	void						GetBodyTransformValues(b2Vec2& Translate,float32& AngleRadians);	//	get values to put INTO the box2D body transform from our transform
	void						SetBodyTransform(u8 TransformChangedBits);			//	reset the body's transform
	void						OnBodyShapeAdded(TCollisionShape& CollisionShape);	//	body shape has been added
	void						OnBodyShapeRemoved();								//	body shape has been removed

protected:
	float					m_Bounce;					//	0..1
	float					m_Friction;					//	0..1
	float					m_Damping;					//	0...infinate, but smaller numbers are better
	float3					m_WorldUp;					//	normalised vector for the world "up". This is used for gravity specified per-node
	float3					m_RelativeMovement;			//	per-frame linear movement, relative to our rotation

	TLMaths::TTransform		m_Transform;				//	world transform of shape
	u8						m_TransformChangedBits;		//	dont broadcast trasnform changes until post update - TLMaths_TransformBit_XXX
	u8						m_BodyTransformChangedBits;	//	if not zero then the body transform needs setting. Generally this means if the node is disabled and has moved then we need to set it again when enabling.

	TRef					m_OwnerSceneNode;			//	"Owner" scene node - if this is set then we automaticcly process some stuff
	TFlags<Flags>			m_PhysicsFlags;

	THeapArray<TCollisionShape>	m_CollisionShapes;			//	list of collision shapes
	THeapArray<TCollisionInfo>	m_Collisions;				//	list of collisions during our last update - published in PostUpdate to subscribers
	THeapArray<TRef>			m_NonCollisionNodes;		//	list of nodes we're explicitly not allowed to collide with

	b2Body*					m_pBody;					//	box2d body
};


FORCEINLINE bool operator==(const TLPhysics::TPhysicsNode* pNode,const TRef& Ref)	{	return pNode ? (*pNode == Ref) : false;	}




FORCEINLINE void TLPhysics::TPhysicsNode::EnableCollision(Bool Enable)
{	
	if ( m_PhysicsFlags( Flag_HasCollision ) != Enable )
	{
		m_PhysicsFlags.Set( Flag_HasCollision, Enable );	
		OnCollisionEnabledChanged( Enable );	
	}
}


FORCEINLINE void TLPhysics::TPhysicsNode::SetEnabled(Bool Enable)
{	
	if ( m_PhysicsFlags( Flag_Enabled ) != Enable )	
	{	
		m_PhysicsFlags.Set( Flag_Enabled, Enable );	
		OnNodeEnabledChanged( Enable );	
	}	
}


