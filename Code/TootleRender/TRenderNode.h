/*------------------------------------------------------

	Render object

-------------------------------------------------------*/
#pragma once

//#include "TLRender.h"
#include <TootleCore/TPtrArray.h>
#include <TootleCore/TKeyArray.h>
#include <TootleAsset/TMesh.h>
#include <TootleCore/TFlags.h>
#include <TootleCore/TLGraph.h>
#include <TootleMaths/TQuadTree.h>

namespace TLRender
{
	class TRenderNode;
	class TRenderTarget;
	class TRenderZoneNode;
}



class TLRender::TRenderNode : public TLGraph::TGraphNode<TLRender::TRenderNode>
{
private:
	enum InvalidateFlags
	{
		InvalidateDummy = 0,
		InvalidateLocalBounds,
		InvalidateWorldBounds,
		InvalidateWorldPos,
		InvalidateParentLocalBounds,
		ForceInvalidateParentsLocalBounds,		//	invalidate parents even if nothing has changed - I've needed this to invalidate parent's boxes even though current bounds are invalid
		InvalidateChildWorldBounds,				//	
		InvalidateChildWorldPos,				//	
		InvalidateParentsChildrenWorldBounds,	//	explicitly invalidate our parents' children - by default do not (otherwise we end up invalidating the whole tree as it goes up to the root down)
		InvalidateParentsChildrenWorldPos,		//	as above, but for pos
		FromChild,								//	child has instigated this invalidation
		FromParent,								//	parent has instigated this invalidation
	};
	typedef TFlags<InvalidateFlags> TInvalidateFlags;

public:
	//	gr: not actually a class...
	class RenderFlags
	{
	public:
		//	flags for all the parts of the mesh we're drawing/processing
		enum Flags
		{
			Enabled,					//	when not enabled, this (and tree) is not rendered
			DepthRead,					//	read from depth buffer (off draws over everything)
			DepthWrite,					//	write to depth buffer (off means will get drawn over)
			ResetScene,					//	position and rotation are not inherited
			CalcWorldBoundsBox,			//	always calculate world bounds box (for physics, object picking etc etc)
			CalcWorldBoundsSphere,		//	always calculate world bounds sphere (for physics, object picking etc etc)
			UseVertexColours,			//	bind vertex colours of mesh. if not set when rendering, a mesh the colours are not bound
			UseMeshLineWidth,			//	calculates mesh/world line width -> screen/pixel width
			UseNodeColour,				//	set when colour is something other than 1,1,1,1 to save some processing (off by default!)
			EnableCull,					//	enable camera/frustum/zone culling. if disabled, the whole tree below is disabled
			ForceCullTestChildren,		//	if we are not culled, still do a cull test with children. By default, when not culled, we don't test children as they should be encapsulated within our bounds
			InvalidateBoundsByChildren,	//	default behaviour is to invalidate our bounding box when child CHANGES. we can disable this for certain cases - eg. root objects, which when invalidated cause the entire tree to recalculate stuff - still invalidated when a child is ADDED to the tree (this may have to change to "first-calculation of bounds")
	
			Debug_Wireframe,			//	draw in wireframe
			Debug_Points,				//	draw a point at every vertex
			Debug_Outline,				//	render again with wireframe on
			Debug_LocalBoundsBox,		//	render our local bounds box
			Debug_WorldBoundsBox,		//	render our world bounds box
			Debug_LocalBoundsSphere,	//	render our local bounds sphere
			Debug_WorldBoundsSphere,	//	render our world bounds sphere
			Debug_LocalBoundsCapsule,	//	render our local bounds capsule
			Debug_WorldBoundsCapsule,	//	render our world bounds capsule
		};
	};

public:
	TRenderNode(TRefRef RenderNodeRef=TRef(),TRefRef TypeRef=TRef());
	virtual ~TRenderNode()					{};

	virtual void							Initialise(TLMessaging::TMessage& Message);	//	generic render node init
	virtual void							Shutdown();									//	clean-up any TPtrs back to us so we will be deallocated

	FORCEINLINE const TLMaths::TTransform&	GetTransform() const						{	return m_Transform;	}
	FORCEINLINE const float3&				GetTranslate() const						{	return m_Transform.GetTranslate() ;	}
	FORCEINLINE const float3&				GetScale() const							{	return m_Transform.GetScale() ;	}
	FORCEINLINE const TLMaths::TQuaternion&	GetRotation() const							{	return m_Transform.GetRotation() ;	}

	FORCEINLINE void						SetTransform(const TLMaths::TTransform& Transform);
	FORCEINLINE void						SetTranslate(const float3& Translate,Bool Invalidate=TRUE);
	FORCEINLINE void						SetScale(const float3& Scale,Bool Invalidate=TRUE);
	FORCEINLINE void						SetScale(float Scale,Bool Invalidate=TRUE)	{	SetScale( float3( Scale, Scale, Scale ), Invalidate );	}
	FORCEINLINE void						SetRotation(const TLMaths::TQuaternion& Rotation,Bool Invalidate=TRUE);
	
	FORCEINLINE float						GetLineWidth() const						{	return m_LineWidth;	}
	FORCEINLINE void						SetLineWidth(float Width)					{	m_LineWidth = Width;	}
	FORCEINLINE const float3&				GetWorldPos() const							{	return m_WorldPos;	}
	FORCEINLINE const float3&				GetWorldPos(Bool& IsValid) const			{	IsValid = m_WorldPosValid;	return m_WorldPos;	}
	FORCEINLINE Bool						IsWorldPosValid() const						{	return m_WorldPosValid;	}

	FORCEINLINE TFlags<RenderFlags::Flags>&	GetRenderFlags()							{	return m_RenderFlags;	}
	FORCEINLINE const TFlags<RenderFlags::Flags>&	GetRenderFlags() const				{	return m_RenderFlags;	}
	void									ClearDebugRenderFlags();
	FORCEINLINE void						SetAlpha(float Alpha)						{	if ( m_Colour.GetAlpha() != Alpha )	{	m_Colour.GetAlpha() = Alpha;	OnColourChanged();	}	}
	FORCEINLINE float						GetAlpha() const							{	return m_Colour.GetAlpha();	}
	FORCEINLINE void						SetColour(const TColour& Colour)			{	m_Colour = Colour;	OnColourChanged();	}
	FORCEINLINE const TColour&				GetColour() const							{	return m_Colour;	}
	FORCEINLINE Bool						IsColourValid() const						{	return m_RenderFlags( RenderFlags::UseNodeColour );	}
	FORCEINLINE void						OnColourChanged();							//	enable node colour if non-white
	FORCEINLINE const TRef&					GetMeshRef() const							{	return m_MeshRef;	}
	FORCEINLINE void						SetMeshRef(TRefRef MeshRef)					{	if ( m_MeshRef != MeshRef )	{	m_MeshRef = MeshRef;	OnMeshRefChanged();	}	}

	virtual TPtr<TLAsset::TMesh>&			GetMeshAsset();								//	default behaviour fetches the mesh from the asset lib with our mesh ref

	FORCEINLINE void						SetRenderNodeRef(TRefRef Ref)				{	SetNodeRef( Ref );	}
	FORCEINLINE const TRef&					GetRenderNodeRef() const					{	return GetNodeRef();	}

	virtual void							OnAdded();
	void									Copy(const TRenderNode& OtherRenderNode);	//	copy render object DOES NOT COPY CHILDREN or parent! just properties

	FORCEINLINE TBinaryTree&				GetData()									{	return m_Data;	}
	FORCEINLINE TPtr<TBinaryTree>			GetData(TRefRef DataRef)					{	return GetData().GetChild( DataRef );	}
	FORCEINLINE TPtr<TBinaryTree>			AddData(TRefRef DataRef)					{	return GetData().AddChild( DataRef );	}

	//	overloaded render routine for generic stuff. if this returns TRUE then continue with default RenderNode rendering - 
	//	if FALSE presumed we are doing psuedo rendering ourselves (creating RenderNodes and rendering them to the render target)
	virtual Bool							Draw(TRenderTarget* pRenderTarget,TRenderNode* pParent,TPtrArray<TRenderNode>& PostRenderList);	//	pre-draw routine for a render object

	FORCEINLINE void						OnTransformChanged();						//	invalidate bounds
	FORCEINLINE void						OnMeshChanged();							//	invalidate bounds
	FORCEINLINE void						OnBoundsChanged()							{	OnMeshChanged();	}

	void									CalcWorldPos(const TLMaths::TTransform& SceneTransform);	//	calculate our new world position from the latest scene transform

	const TLMaths::TBox&					CalcWorldBoundsBox(const TLMaths::TTransform& SceneTransform);	//	if invalid calculate our local bounds box (accumulating children) if out of date and return it
	const TLMaths::TBox&					CalcLocalBoundsBox();						//	return our current local bounds box and calculate if invalid
	FORCEINLINE const TLMaths::TBox&		GetWorldBoundsBox() const					{	return m_WorldBoundsBox;	}	//	return our current local bounds box, possibly invalid
	FORCEINLINE const TLMaths::TBox&		GetLastWorldBoundsBox() const				{	return m_WorldBoundsBox.IsValid() ? m_WorldBoundsBox : m_LastWorldBoundsBox;	}	
	FORCEINLINE const TLMaths::TBox&		GetLocalBoundsBox() const					{	return m_LocalBoundsBox;	}	//	return our current local bounds box, possibly invalid

	const TLMaths::TSphere&					CalcWorldBoundsSphere(const TLMaths::TTransform& SceneTransform);	//	if invalid calculate our local bounds box (accumulating children) if out of date and return it
	const TLMaths::TSphere&					CalcLocalBoundsSphere();					//	return our current local bounds box and calculate if invalid
	FORCEINLINE const TLMaths::TSphere&		GetWorldBoundsSphere() const				{	return m_WorldBoundsSphere;	}	//	return our current local bounds box, possibly invalid
	FORCEINLINE const TLMaths::TSphere&		GetLocalBoundsSphere() const				{	return m_LocalBoundsSphere;	}	//	return our current local bounds box, possibly invalid

	const TLMaths::TCapsule&				CalcWorldBoundsCapsule(const TLMaths::TTransform& SceneTransform);	//	if invalid calculate our local bounds box (accumulating children) if out of date and return it
	const TLMaths::TCapsule&				CalcLocalBoundsCapsule();					//	return our current local bounds box and calculate if invalid
	FORCEINLINE const TLMaths::TCapsule&	GetWorldBoundsCapsule() const				{	return m_WorldBoundsCapsule;	}	//	return our current local bounds box, possibly invalid
	FORCEINLINE const TLMaths::TCapsule&	GetLocalBoundsCapsule() const				{	return m_LocalBoundsCapsule;	}	//	return our current local bounds box, possibly invalid

	FORCEINLINE TPtr<TLMaths::TQuadTreeNode>*	GetRenderZoneNode(TRefRef RenderTargetRef)	{	return m_RenderZoneNodes.Find( RenderTargetRef );	}
	FORCEINLINE TPtr<TLMaths::TQuadTreeNode>*	SetRenderZoneNode(TRefRef RenderTargetRef,TPtr<TLMaths::TQuadTreeNode>& pRenderZoneNode)	{	return m_RenderZoneNodes.Add( RenderTargetRef, pRenderZoneNode );	}

	FORCEINLINE Bool						operator==(TRefRef Ref) const				{	return GetRenderNodeRef() == Ref;	}
	FORCEINLINE Bool						operator<(TRefRef Ref) const				{	return GetRenderNodeRef() < Ref;	}

protected:
	FORCEINLINE void						OnMeshRefChanged()							{	m_pMeshCache = NULL;	OnMeshChanged();	}
	//void									SetBoundsInvalid(const TInvalidateFlags& InvalidateFlags=TInvalidateFlags(InvalidateLocalBounds,InvalidateWorldBounds,InvalidateWorldPos,InvalidateParents,InvalidateChildren));	//	set all bounds as invalid
	void									SetBoundsInvalid(const TInvalidateFlags& InvalidateFlags);

protected:
	TLMaths::TTransform			m_Transform;				//	local transform 
	TColour						m_Colour;					//	colour of render node - only works if UseNodeColour is set
	float						m_LineWidth;				//	this is an overriding line width for rendering lines in the mesh. In pixel width. NOT like the mesh line width which is in a world-size.
	float3						m_WorldPos;					//	we always calc the world position on render, even if we dont calc the bounds box/sphere/etc, it's quick and handy!
	Bool						m_WorldPosValid;			//	if this is not valid then the transform of this node has changed since our last render

	//	gr: todo: almagamate all these bounds shapes into a single bounds type that does all 3 or picks the best or something
	TLMaths::TBox				m_LocalBoundsBox;			//	bounding box of self (without transformation) and children (with transformation, so relative to us)
	TLMaths::TBox				m_WorldBoundsBox;			//	bounding box of self in world space
	TLMaths::TBox				m_LastWorldBoundsBox;		//	last valid world bounds box
	TLMaths::TSphere			m_LocalBoundsSphere;		//	bounding sphere Shape of self (without transformation) and children (with transformation, so relative to us)
	TLMaths::TSphere			m_WorldBoundsSphere;		//	bounding sphere Shape of self in world space
	TLMaths::TSphere			m_LastWorldBoundsSphere;	//	
	TLMaths::TCapsule			m_LocalBoundsCapsule;		//	bounding capsule Shape of self (without transformation) and children (with transformation, so relative to us)
	TLMaths::TCapsule			m_WorldBoundsCapsule;		//	bounding capsule shape of self in world space
	TLMaths::TCapsule			m_LastWorldBoundsCapsule;	//	

	TFlags<RenderFlags::Flags>	m_RenderFlags;

	TKeyArray<TRef,TPtr<TLMaths::TQuadTreeNode> >	m_RenderZoneNodes;	//	for each render target we can have a Node(TRenderZoneNode) for Render Zones

	//	todo: turn all these into ref properties in a KeyArray to make it a bit more flexible
	TRef						m_MeshRef;
	TPtr<TLAsset::TMesh>		m_pMeshCache;

	TBinaryTree					m_Data;					//	data attached to render object
};



//---------------------------------------------------------------
//	QuadTreeNode for render nodes
//---------------------------------------------------------------
class TLRender::TRenderZoneNode : public TLMaths::TQuadTreeNode
{
public:
	TRenderZoneNode(TRefRef RenderNodeRef);

	void				CalcWorldBounds(TLRender::TRenderNode* pRenderNode,const TLMaths::TTransform& SceneTransform);	//	calculate all the world bounds we need to to do a zone test
	virtual SyncBool	IsInShape(const TLMaths::TBox2D& Shape);

protected:
	TRef				m_RenderNodeRef;		//	render node that we're linked to
	TPtr<TRenderNode>	m_pRenderNode;			//	cache of render node
};






//---------------------------------------------------------------
//	enable node colour if non-white
//---------------------------------------------------------------
FORCEINLINE void TLRender::TRenderNode::OnColourChanged()
{
	if ( m_Colour.GetRed() > TLMaths::g_NearOne &&
		m_Colour.GetGreen() > TLMaths::g_NearOne &&
		m_Colour.GetBlue() > TLMaths::g_NearOne &&
		m_Colour.GetAlpha() > TLMaths::g_NearOne )
	{
		m_RenderFlags.Clear( RenderFlags::UseNodeColour );
	}
	else
	{
		m_RenderFlags.Set( RenderFlags::UseNodeColour );
	}
}



FORCEINLINE void TLRender::TRenderNode::SetTransform(const TLMaths::TTransform& Transform)
{
	//	gr: note, no checks using this function atm...
	m_Transform = Transform;	
	OnTransformChanged();
}

FORCEINLINE void TLRender::TRenderNode::SetTranslate(const float3& Translate,Bool Invalidate)				
{	
	m_Transform.SetTranslate( Translate );

	if ( Invalidate )	
		OnTransformChanged();	
}

FORCEINLINE void TLRender::TRenderNode::SetScale(const float3& Scale,Bool Invalidate)						
{	
	m_Transform.SetScale( Scale );	

	if ( Invalidate )	
		OnTransformChanged();	
}

FORCEINLINE void TLRender::TRenderNode::SetRotation(const TLMaths::TQuaternion& Rotation,Bool Invalidate)	
{	
	m_Transform.SetRotation( Rotation );	

	if ( Invalidate )	
		OnTransformChanged();	
}

//---------------------------------------------------------------
//	invalidate bounds when pos/rot/scale
//---------------------------------------------------------------
FORCEINLINE void TLRender::TRenderNode::OnTransformChanged()						
{	
	SetBoundsInvalid( TInvalidateFlags( 
						//HasChildren() ? InvalidateLocalBounds : InvalidateDummy, //	gr: not needed?
						InvalidateWorldPos,				//	world pos must have changed - may be able to reduce this to just Translate changes
						InvalidateWorldBounds,			//	shape of mesh must have changed
						InvalidateParentLocalBounds,	//	our parent's LOCAL bounds has now changed as it's based on it's children (this)
						InvalidateChildWorldBounds,		//	invalidate the children's world bounds
						InvalidateChildWorldPos		//	invalidate the children's world pos too
					) );	
}


//---------------------------------------------------------------
//	invalidate bounds when mesh has changed
//---------------------------------------------------------------
FORCEINLINE void TLRender::TRenderNode::OnMeshChanged()								
{	
	SetBoundsInvalid( TInvalidateFlags( 
						InvalidateLocalBounds,			//	our local shape has probbaly changed
						InvalidateWorldBounds,			//	this also affects world bounds
						InvalidateParentLocalBounds		//	and so shape of parents may have changed as it encapsulates us
						) );	
}

