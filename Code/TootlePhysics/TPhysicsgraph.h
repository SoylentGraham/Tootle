/*

	physics node graph - 
	todo:
		turn into a quad/oct/bsp tree via nodes to replace the collision zones

*/
#pragma once

#include <TootleCore/TLGraph.h>

#include <box2d/source/dynamics/b2WorldCallbacks.h> // Required for the callback classes

#include "TPhysicsNode.h"
#include "TJoint.h"

#include <TootleMaths/TQuadTree.h>

namespace TLPhysics
{
	class TPhysicsgraph;
	class TPhysicsNode;
	class TPhysicsNodeFactory;

	extern TPtr<TPhysicsgraph> g_pPhysicsgraph;
};


class b2World;
class b2Fixture;


//-----------------------------------------------------
//	TPhysicsgraph class
//-----------------------------------------------------
class TLPhysics::TPhysicsgraph : public TLGraph::TGraph<TLPhysics::TPhysicsNode>, public b2ContactListener, public b2ContactFilter
{
public:
	TPhysicsgraph() :
		TLGraph::TGraph<TLPhysics::TPhysicsNode>	( "Physics" )
	{
		m_pWorld = NULL;
	}

	void					SetRootCollisionZone(TPtr<TLMaths::TQuadTreeZone>& pZone,Bool AllowSleep=TRUE);	//	set a new root collision zone. Allow sleep to speed up idle objects, BUT without gravity, joints don't update/constrain properly... looking for a good soluition to this

	void					GetNodesInShape(const TLMaths::TShapePolygon2D& Shape,TPointerArray<TLPhysics::TPhysicsNode>& NearPhysicsNodes);
	void					GetNodesInShape(const TLMaths::TSphere2D& Shape,TPointerArray<TLPhysics::TPhysicsNode>& NearPhysicsNodes,Bool StrictSphere);	//	get all the nodes in this shape - for spheres optionally do strict sphere checks - box2D uses Boxes for its query so it can return objects outside the sphere. this does an extra loop to make sure distance is within the radius
	void					GetNodesInShape(const TLMaths::TBox2D& Shape,TPointerArray<TLPhysics::TPhysicsNode>& NearPhysicsNodes);						//	get all the nodes in this shape

	FORCEINLINE const b2World*			GetWorld()					const {	return m_pWorld;	}				//	box2d's world
	FORCEINLINE const TLMaths::TBox2D&	GetWorldShape()				const { return m_WorldShape; }

	FORCEINLINE void				AddJoint(const TJoint& Joint)	{	m_NodeJointQueue.Add( Joint );	};	//	add a joint to be created on next update
	FORCEINLINE void				RefilterShape(b2Fixture* pShape)	{	m_RefilterQueue.Add( pShape );	}	//	add to list of shapes that need refiltering
	FORCEINLINE Bool				RemoveRefilterShape(b2Fixture* pShape)		{	return m_RefilterQueue.Remove( pShape );	}	//	remvoe from the list of shapes that need refiltering

	// Test routines
	FORCEINLINE void		SetGravityX(float fValue)		{	if ( g_WorldUp.x == fValue )	return;		g_WorldUp.x = fValue;	CalcWorldUpNormal();	}
	FORCEINLINE void		SetGravityY(float fValue)		{	if ( g_WorldUp.y == fValue )	return;		g_WorldUp.y = fValue;	CalcWorldUpNormal();	}
	FORCEINLINE void		SetGravityZ(float fValue)		{	if ( g_WorldUp.z == fValue )	return;		g_WorldUp.z = fValue;	CalcWorldUpNormal();	}
	
protected:

	virtual SyncBool		Initialise();
	virtual SyncBool		Shutdown();

	virtual void			UpdateGraph(float TimeStep);
	

	virtual void			OnNodeRemoving(TLPhysics::TPhysicsNode& Node);
	virtual void			OnNodeAdded(TLPhysics::TPhysicsNode& Node,Bool SendAddedMessage);

	void					CalcWorldUpNormal();					//	world up has changed, recalc the normal
	
	SyncBool				CreateJoint(const TJoint& Joint);		//	create joint. if Wait then we're waiting for a node to be created still
	void					RemoveJoint(TRefRef NodeA,TRefRef NodeB);	//	remove joint between these two nodes
	void					RemoveJoint(TRefRef NodeA);				//	remove all joints involving this node

private:
	//	box2d listener functions
	virtual void			BeginContact(b2Contact* contact);		//	new collision with shape
	virtual void			EndContact(b2Contact* contact);			//	no longer colliding with shape
	virtual bool			ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB);

protected:
	TLMaths::TBox2D					m_WorldShape;					//  World shape - stored so we can access it as you can't get this from box2d
	b2World*						m_pWorld;						//	box2d's world
	THeapArray<TJoint>				m_NodeJoints;					//	list of joints created
	THeapArray<TJoint>				m_NodeJointQueue;				//	list of joints that are to be created in the next update
	TPointerArray<b2Fixture>		m_RefilterQueue;				//	queue of box2d shapes that need refiltering
};

TLGraph_DeclareGraph(TLPhysics::TPhysicsNode);



//----------------------------------------------------------
//	Generic physics node factory
//----------------------------------------------------------
class TLPhysics::TPhysicsNodeFactory : public TNodeFactory<TPhysicsNode>
{
public:
	virtual TPhysicsNode*	CreateObject(TRefRef InstanceRef,TRefRef TypeRef);
};

