/*------------------------------------------------------

	Render object

-------------------------------------------------------*/
#pragma once

//#include "TLRender.h"
#include <TootleCore/TPtrArray.h>
#include <TootleCore/TKeyArray.h>
#include <TootleAsset/TMesh.h>
#include <TootleAsset/TTexture.h>
#include <TootleAsset/TShader.h>
#include <TootleCore/TFlags.h>
#include <TootleCore/TLGraph.h>
#include <TootleMaths/TQuadTree.h>
#include <TootleMaths/TCapsule.h>
#include "TEffect.h"
#include "TRasteriser.h"

namespace TLRender
{
	class TRenderNode;
	class TRenderTarget;
	class TRenderZoneNode;
	class TRendergraph;

#define TLRender_TRenderNode_DatumBoundsBox			TRef_Static(UNDERSCORE,B,n,b,THREE)
#define TLRender_TRenderNode_DatumBoundsBox2D		TRef_Static(UNDERSCORE,B,n,b,TWO)
#define TLRender_TRenderNode_DatumBoundsSphere		TRef_Static(UNDERSCORE,B,n,s,THREE)
#define TLRender_TRenderNode_DatumBoundsSphere2D	TRef_Static(UNDERSCORE,B,n,s,TWO)
};

namespace TLMaths
{
	template<class SHAPETYPE>
	class TBoundsShape;
}


template<class SHAPETYPE>
class TLMaths::TBoundsShape
{
public:
	TBoundsShape() : m_WorldValid ( SyncFalse )	{}

	const SHAPETYPE&	CalcWorldShape(TLRender::TRenderNode& RenderNode);	//	if invalid calculate our local bounds box (accumulating children) if out of date and return it
	FORCEINLINE Bool	IsWorldShapeValid()		{	return ( m_WorldValid == SyncTrue );	}					//	return Bool of if it's up to date
	FORCEINLINE void	SetWorldShapeOld()		{	if ( m_WorldValid == SyncTrue )	m_WorldValid = SyncWait;	}	//	if up-to-date (TRUE) then make old (WAIT)
	
	FORCEINLINE Bool	IsLocalShapeValid()		{	return m_LocalShape.IsValid();	}
	FORCEINLINE void	SetLocalShapeInvalid()	{	m_LocalShape.SetInvalid();	}

public:
	SyncBool		m_WorldValid;	//	Validity of world-shape - SyncWait if valid, but old
	SHAPETYPE		m_LocalShape;	//	
	SHAPETYPE		m_WorldShape;	//	
};


//---------------------------------------------------------------
//	Node in the render tree
//---------------------------------------------------------------
class TLRender::TRenderNode : public TLGraph::TGraphNode<TLRender::TRenderNode>
{
public:
	friend class TLMaths::TBoundsShape<TLMaths::TShapeBox>;
	friend class TLMaths::TBoundsShape<TLMaths::TShapeBox2D>;
	friend class TLMaths::TBoundsShape<TLMaths::TShapeSphere>;
	friend class TLMaths::TBoundsShape<TLMaths::TShapeSphere2D>;
	friend class TLRender::TRendergraph;

	struct Properties : public TLMaths::TTransform::Properties
	{
		enum
		{
			LineWidth	= TRef_Static(L,i,n,e,W),
			PointSize	= TRef_Static(P,o,i,n,t),
			Mesh		= TRef_Static(M,e,s,h,R),
			Texture		= TRef_Static(T,e,x,t,u),
			Enabled		= TRef_Static(E,n,a,b,l),
			RenderFlags	= TRef_Static(R,F,l,a,g),
			RenderFlagsOn	= TRef_Static(R,F,S,e,t),
			RenderFlagsOff	= TRef_Static(R,F,C,l,e),
			Colour		= TRef_Static(C,o,l,o,u),
			AttachDatum	= TRef_Static(A,t,t,a,c),
			Debug_Datum	= TRef_Static(D,b,g,D,a),
			Debug_DatumRemove = TRef_Static(R,m,D,b,g),
		};
	};
	
	struct Messages
	{
		enum
		{
			SetTransform	= TRef_Static(S,e,t,T,r),
			LocalTransform	= TRef_Static(L,o,c,T,r),
			DoTransform		= TRef_Static(D,o,T,r,a),
		};
	};
	
protected:
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
		ForceInvalidateChildren,				//	does child invalidation even if our world bounds didn't change
	};
	typedef TFlags<InvalidateFlags> TInvalidateFlags;

public:
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
			ResetColour,				//	scene's colour is not inherited and reset to this node's colour
			UseVertexColours,			//	bind vertex colours of mesh. if not set when rendering, a mesh the colours are not bound
			UseFloatColours,			//	force system to use floating point colour buffer (slower than bytes, but better colour range as 32 bit instead of 8)
			UseVertexUVs,				//	bind vertex UVs of mesh. if not set when rendering we have no texture mapping
			UseMeshLineWidth,			//	calculates mesh/world line width -> screen/pixel width
			UseNodeColour,				//	set when colour is something other than 1,1,1,1 to save some processing (off by default!)
			UsePointSprites,			//	render points of a mesh as point sprites
			EnableCull,					//	enable camera/frustum/zone culling. if disabled, the whole tree below is disabled
			ForceCullTestChildren,		//	if we are not culled, still do a cull test with children. By default, when not culled, we don't test children as they should be encapsulated within our bounds
			InvalidateBoundsByChildren,	//	default behaviour is to invalidate our bounding box when child CHANGES. we can disable this for certain cases - eg. root objects, which when invalidated cause the entire tree to recalculate stuff - still invalidated when a child is ADDED to the tree (this may have to change to "first-calculation of bounds")
			AddBlending,				//	will ADD when rendering colours/textures etc
	
			Debug_Wireframe,			//	draw in wireframe (all white outlines)
			Debug_ColourWireframe,		//	when drawing in wireframe keep colours
			Debug_Points,				//	draw a point at every vertex
			Debug_Outline,				//	render again with wireframe on
			Debug_Position,				//	draws a 3axis cross at 0,0,0 on the render node
			Debug_LocalBoundsBox,		//	render our local bounds box
			Debug_LocalBoundsSphere,	//	render our local bounds sphere
			Debug_Datums,				//	render all our datums (includes bounds)
			
			//	now deprecated - we know the local -> world datum conversion works
			Debug_WorldBoundsBox = Debug_LocalBoundsBox,
			Debug_WorldBoundsSphere = Debug_LocalBoundsSphere,
		};
	};

public:
	TRenderNode(TRefRef RenderNodeRef=TRef(),TRefRef TypeRef=TRef());

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
	FORCEINLINE float						GetPointSpriteSize() const					{	return m_PointSpriteSize;	}
	FORCEINLINE void						SetPointSpriteSize(float Width)				{	m_PointSpriteSize = Width;	}

	FORCEINLINE TFlags<RenderFlags::Flags>&	GetRenderFlags()							{	return m_RenderFlags;	}
	FORCEINLINE const TFlags<RenderFlags::Flags>&	GetRenderFlags() const				{	return m_RenderFlags;	}
	void									ClearDebugRenderFlags();
	FORCEINLINE Bool						IsEnabled() const							{	return m_RenderFlags( RenderFlags::Enabled );	}
	FORCEINLINE void						SetEnabled(Bool Enabled)					{	m_RenderFlags.Set( RenderFlags::Enabled, Enabled );	OnPropertyChanged(Properties::Enabled);	OnPropertyChanged(Properties::RenderFlags);	}

	FORCEINLINE void						SetAlpha(float Alpha)						{	if ( m_Colour.GetAlphaf() != Alpha )	{	m_Colour.GetAlphaf() = Alpha;	OnColourChanged();	}	}
	FORCEINLINE float						GetAlpha() const							{	return m_Colour.GetAlphaf();	}
	FORCEINLINE void						SetColour(const TColour& Colour)			{	m_Colour = Colour;	OnColourChanged();	}
	FORCEINLINE const TColour&				GetColour() const							{	return m_Colour;	}
	FORCEINLINE Bool						IsColourValid() const						{	return m_RenderFlags( RenderFlags::UseNodeColour );	}
	FORCEINLINE void						OnColourChanged();							//	enable node colour if non-white
	FORCEINLINE const TRef&					GetMeshRef() const							{	return m_MeshRef;	}
	FORCEINLINE void						SetMeshRef(TRefRef MeshRef)					{	if ( m_MeshRef != MeshRef )	{	m_MeshRef = MeshRef;	OnMeshRefChanged();	}	}
	FORCEINLINE const TRef&					GetTextureRef() const						{	return m_TextureRef;	}
	FORCEINLINE void						SetTextureRef(TRefRef TextureRef)			{	if ( m_TextureRef != TextureRef )	{	m_TextureRef = TextureRef;	OnTextureRefChanged();	}	}

	TPtr<TLRender::TEffect>& 				AddEffect(TBinaryTree& EffectData);			//	create an effect from plain data

	virtual TLAsset::TMesh*					GetMeshAsset();								//	default behaviour fetches the mesh from the asset lib with our mesh ref
	virtual TLAsset::TTexture*				GetTextureAsset();							//	default behaviour fetches the texutre from the asset lib with our texture ref
	TPtrArray<TEffect>&						GetEffects()								{	return m_Effects;	}

	FORCEINLINE void						SetRenderNodeRef(TRefRef Ref)				{	SetNodeRef( Ref );	}
	DEPRECATED virtual TRef					GetRenderNodeRef() const					{	return GetNodeRef();	}
	FORCEINLINE TRefRef						GetOwnerSceneNodeRef() const				{	return m_OwnerSceneNode;	}

	virtual void							OnAdded();
	virtual void							OnMoved(const TLRender::TRenderNode& OldParent);
	void									Copy(const TRenderNode& OtherRenderNode);	//	copy render object DOES NOT COPY CHILDREN or parent! just properties

	DEPRECATED FORCEINLINE TBinaryTree&			GetData()									{	return GetNodeData();	}
	DEPRECATED FORCEINLINE TPtr<TBinaryTree>&	GetData(TRefRef DataRef)					{	return GetNodeData().GetChild( DataRef );	}
	DEPRECATED FORCEINLINE TPtr<TBinaryTree>&	AddData(TRefRef DataRef)					{	return GetNodeData().AddChild( DataRef );	}
	virtual void							UpdateNodeData();							

	void									SetAttachDatum(TRefRef DatumRef);			//	change the datum we're attached to. Sets the data and does an immediate translate as required
	FORCEINLINE TRef						GetAttachDatum()							{	TRef DatumRef;	return GetNodeData().ImportData("AttachDatum", DatumRef ) ? DatumRef : TRef();	}

	//	overloaded render routine for generic stuff. if this returns TRUE then continue with default RenderNode rendering - 
	//	if FALSE presumed we are doing psuedo rendering ourselves (creating RenderNodes and rendering them to the render target)
	DEPRECATED virtual Bool					Draw(TRenderTarget* pRenderTarget,TRenderNode* pParent,TPtrArray<TRenderNode>& PostRenderList);	//	pre-draw routine for a render object

	virtual const TLRaster::TRasterData*	Render(TArray<TLRaster::TRasterData>& MeshRasterData,TArray<TLRaster::TRasterSpriteData>& SpriteRasterData,const TColour& SceneColour);	//	Render function. Make up RasterData's and add them to the list. Return a pointer to the main raster data created if applicable
	virtual void							Debug_Render(TArray<TLRaster::TRasterData>& MeshRenderData,TArray<TLRaster::TRasterSpriteData>& RasterSpriteData,const TLRaster::TRasterData* pMainRaster,TPtrArray<TLAsset::TMesh>& TemporaryMeshes);	//	render debug raster data

	virtual void							PreDrawChildren(TLRender::TRenderTarget& RenderTarget,TLMaths::TTransform& SceneTransform)	{}	//	only called if we HAVE children
	virtual void							PostDrawChildren(TLRender::TRenderTarget& RenderTarget)										{}	//	only called if we HAVE children

	virtual void							OnTransformChanged(u8 TransformChangedBits=TLMaths_TransformBitAll);	//	invalidate bounds
	FORCEINLINE void						OnMeshChanged();							//	invalidate bounds
	FORCEINLINE void						OnBoundsChanged()							{	OnMeshChanged();	}

	void									SetWorldTransform(const TLMaths::TTransform& SceneTransform);	//	set new world transform
	virtual Bool							SetWorldTransformOld(Bool SetPosOld,Bool SetTransformOld,Bool SetShapesOld);				//	downgrade all world shape/transform states from valid to old. returns if anyhting was downgraded
	FORCEINLINE Bool						SetWorldTransformOld()						{	return SetWorldTransformOld( TRUE, TRUE, TRUE );	}
	FORCEINLINE SyncBool					IsWorldTransformValid() const				{	return m_WorldTransformValid;	}
	virtual const TLMaths::TTransform&		GetWorldTransform(TRenderNode* pRootNode=NULL,Bool ForceCalculation=FALSE);	//	return the world transform. will explicitly calculate the world transform if out of date. This is a bit rendundant as it's calculated via the render but sometimes we need it outside of that. If WorldTransform is Valid(TRUE) then this is not recalculated. THe root render node should be provided (but in reality not a neccessity, see trac: http://grahamgrahamreeves.getmyip.com:1984/Trac/wiki/KnownIssues )
	virtual const TLMaths::TTransform&		GetChildWorldTransform(TRenderNode* pRootNode=NULL,Bool ForceCalculation=FALSE)		{	return GetWorldTransform( pRootNode, ForceCalculation );	}	//	return the world transform for our children. called by the child from GetWorldTransform. Overload if you transform your children in a specific way without using the base trasnform
	
	const float3&							GetWorldPos();								//	calculate our new world position from the latest scene transform
	FORCEINLINE const float3&				GetWorldPos(SyncBool& IsValid) 				{	GetWorldPos();	IsValid = m_WorldPosValid;	return m_WorldPos;	}
	FORCEINLINE const float3&				GetWorldPosConst() const					{	return m_WorldPos;	}

	Bool									GetLocalPos(float3& LocalPos,const float3& WorldPos);		//	calculate a local-relative position from a world position. this will be in THIS's space. so if this has a scale of 10, the relative position will be 1/10th of the world's offset. The end result, multiplied by this's world transform should produce the original world pos

	//	get world shapes/datums - recalculates if old
	const TLMaths::TShapeBox&				GetWorldBoundsBox()							{	return m_BoundsBox.CalcWorldShape( *this );	}
	const TLMaths::TShapeBox2D&				GetWorldBoundsBox2D()						{	return m_BoundsBox2D.CalcWorldShape( *this );	}
	const TLMaths::TShapeSphere&			GetWorldBoundsSphere()						{	return m_BoundsSphere.CalcWorldShape( *this );	}
	const TLMaths::TShapeSphere2D&			GetWorldBoundsSphere2D()					{	return m_BoundsSphere2D.CalcWorldShape( *this );	}

	//	get world shape/datum - no recalcualtion, also returns age (False=Invalid, Wait=Old, True=UpToDate)
	const TLMaths::TShapeBox&				GetWorldBoundsBox(SyncBool& Validity) const			{	Validity = m_BoundsBox.m_WorldValid;		return m_BoundsBox.m_WorldShape;	}
	const TLMaths::TShapeBox2D&				GetWorldBoundsBox2D(SyncBool& Validity) const		{	Validity = m_BoundsBox2D.m_WorldValid;		return m_BoundsBox2D.m_WorldShape;	}
	const TLMaths::TShapeSphere&			GetWorldBoundsSphere(SyncBool& Validity) const		{	Validity = m_BoundsSphere.m_WorldValid;		return m_BoundsSphere.m_WorldShape;	}
	const TLMaths::TShapeSphere2D&			GetWorldBoundsSphere2D(SyncBool& Validity) const	{	Validity = m_BoundsSphere2D.m_WorldValid;	return m_BoundsSphere2D.m_WorldShape;	}

	virtual TPointerArray<TRenderNode>&			GetLocalBoundsChildren()						{	return GetChildren();	}	//	specialise this for your rendernode to have control over the local bounds calculation
	
	template<class SHAPETYPE> const SHAPETYPE&	GetLocalBounds()							{ TLDebug_Break("Specialise this for shapes we don't currently support");	static SHAPETYPE g_DummyShape;	return g_DummyShape; }
	const TLMaths::TShapeBox&				GetLocalBoundsBox() 							{	CalcLocalBounds( m_BoundsBox.m_LocalShape );			return m_BoundsBox.m_LocalShape;	}
	const TLMaths::TShapeBox2D&				GetLocalBoundsBox2D() 							{	CalcLocalBounds( m_BoundsBox2D.m_LocalShape );			return m_BoundsBox2D.m_LocalShape;	}
	const TLMaths::TShapeSphere&			GetLocalBoundsSphere()							{	CalcLocalBounds( m_BoundsSphere.m_LocalShape );			return m_BoundsSphere.m_LocalShape;	}
	const TLMaths::TShapeSphere2D&			GetLocalBoundsSphere2D()						{	CalcLocalBounds( m_BoundsSphere2D.m_LocalShape );		return m_BoundsSphere2D.m_LocalShape;	}
	virtual void							GetLocalDatums(TArray<const TLMaths::TShape*>& LocalDatums);	//	get all datums in the mesh and render node (ie. includes bounds)
	virtual const TLMaths::TShape*			GetLocalDatum(TRefRef DatumRef);				//	extract a datum from our mesh - unless a special ref is used to get bounds shapes
	template<class SHAPETYPE>
	FORCEINLINE const SHAPETYPE*			GetLocalDatum(TRefRef DatumRef);				//	get a datum of a specific type - returns NULL if it doesn't exist or is of a different type
	Bool									GetLocalDatumPos(TRefRef DatumRef,float3& Position);	//	get the position of a datum in local space. returns FALSE if no such datum
	void									GetWorldDatums(TPtrArray<TLMaths::TShape>& WorldDatums,Bool KeepShape=FALSE,Bool ForceCalc=FALSE);	//	get all datums in the mesh and render node (ie. includes bounds) in world space. Use very sparingly! (ie. debug only)
	TPtr<TLMaths::TShape>					GetWorldDatum(TRefRef DatumRef,Bool KeepShape=FALSE,Bool ForceCalc=FALSE);	//	extract a datum  and transform it into a new world space shape. Using KeepShape will ensure a shape type stays the same, eg, rotated box doesn't turn into a poly and stays as a box
	Bool									GetWorldDatumPos(TRefRef DatumRef,float3& Position,Bool KeepShape=FALSE,Bool ForceCalc=FALSE);	//	get the position of a datum in world space. returns FALSE if no such datum. Currently will recalc the world transform if it's out of date. 

	const TArray<TRef>&						Debug_GetDebugRenderDatums() const				{	return m_Debug_RenderDatums;	}

	FORCEINLINE TLMaths::TQuadTreeNode*		GetRenderZoneNode(TRefRef RenderTargetRef)	{	TLMaths::TQuadTreeNode** ppZoneNode = m_RenderZoneNodes.Find( RenderTargetRef );		return ppZoneNode ? (*ppZoneNode) : NULL;	}
	FORCEINLINE bool						SetRenderZoneNode(TRefRef RenderTargetRef,TLMaths::TQuadTreeNode* pRenderZoneNode)	{	return m_RenderZoneNodes.Add( RenderTargetRef, pRenderZoneNode ) != NULL;	}

	FORCEINLINE Bool						operator<(TRefRef NodeRef) const			{	return GetNodeRef() < NodeRef;	}
	FORCEINLINE Bool						operator==(TRefRef NodeRef) const			{	return GetNodeRef() == NodeRef;	}
	FORCEINLINE Bool						operator==(const TRenderNode& That) const				{	return GetNodeRef() == That.GetNodeRef();	}
	FORCEINLINE Bool						operator<(const TSorter<TRenderNode*,TRef>& That) const	{	return GetNodeRef() < That.This()->GetNodeRef();	}
	FORCEINLINE Bool						operator<(const TSorter<TRenderNode,TRef>& That) const	{	return GetNodeRef() < That->GetNodeRef();	}

protected:
	virtual void							Initialise(TLMessaging::TMessage& Message);	//	generic render node init
	virtual void 							Update(float Timestep);	
	virtual void							ProcessMessage(TLMessaging::TMessage& Message);
	virtual void							SetProperty(TLMessaging::TMessage& Message);	//	SetProperty message - made into virtual func as it's will be commonly used.

	Bool									CreateChildNode(TBinaryTree& ChildInitData);	//	create a child node from plain data

	FORCEINLINE void						OnMeshRefChanged()							{	this->UnsubscribeFrom( m_pMeshCache );	m_pMeshCache = NULL;	OnMeshChanged();	OnPropertyChanged( Properties::Mesh );	}
	FORCEINLINE void						OnTextureRefChanged()						{	this->UnsubscribeFrom( m_pTextureCache );	m_pTextureCache = NULL;	OnPropertyChanged( Properties::Texture );	}
	void									OnAttachDatumChanged();						//	called when attach datum needs repositioning
	//void									SetBoundsInvalid(const TInvalidateFlags& InvalidateFlags=TInvalidateFlags(InvalidateLocalBounds,InvalidateWorldBounds,InvalidateWorldPos,InvalidateParents,InvalidateChildren));	//	set all bounds as invalid
	void									SetBoundsInvalid(const TInvalidateFlags& InvalidateFlags);

	template<class SHAPETYPE> void			CalcLocalBounds(SHAPETYPE& Shape);

protected:
	TLMaths::TTransform			m_Transform;				//	local transform of node
	TLMaths::TTransform			m_WorldTransform;			//	last SceneTransform
	SyncBool					m_WorldTransformValid;		//	m_WorldTransform is... false - invalid (never calculated before). Wait - Last one is valid, but needs updating on next render. True - most update to date and doesnt need updating again

	TColour						m_Colour;					//	colour of render node - only works if UseNodeColour is set
	float						m_LineWidth;				//	this is an overriding line width for rendering lines in the mesh. In pixel width. NOT like the mesh line width which is in a world-size.
	float						m_PointSpriteSize;			//	size of point sprites in world space. API renders these in pixel sizes so we do a conversion in the render
	float3						m_WorldPos;					//	we always calc the world position on render, even if we dont calc the bounds box/sphere/etc, it's quick and handy!
	SyncBool					m_WorldPosValid;			//	if this is not valid then the transform of this node has changed since our last render
	Bool						m_AttachDatumValid;			//	if we're using an attach datum, and this is FALSE then we need to re-position ourself to the datum

	//	gr: todo: almagamate all these bounds shapes into a single bounds type that does all 3 or picks the best or something
	//	gr: also todo; turn this into datums with special refs. Then add a bounds-checking type as above
	TLMaths::TBoundsShape<TLMaths::TShapeBox>		m_BoundsBox;
	TLMaths::TBoundsShape<TLMaths::TShapeBox2D>		m_BoundsBox2D;
	TLMaths::TBoundsShape<TLMaths::TShapeSphere>	m_BoundsSphere;
	TLMaths::TBoundsShape<TLMaths::TShapeSphere2D>	m_BoundsSphere2D;

	TFlags<RenderFlags::Flags>	m_RenderFlags;

	TKeyArray<TRef,TLMaths::TQuadTreeNode*>		m_RenderZoneNodes;	//	for each render target we can have a Node(TRenderZoneNode) for Render Zones

	//	todo: turn all these into ref properties in a KeyArray to make it a bit more flexible
	//	gr: restore these to TPtr's asap! bug in asset management; http://grahamgrahamreeves.getmyip.com:1984/Trac/changeset/776
	TRef						m_MeshRef;
	TPtr<TLAsset::TMesh>		m_pMeshCache;
	TRef						m_TextureRef;
	TPtr<TLAsset::TTexture>		m_pTextureCache;
	TPtrArray<TEffect>			m_Effects;				//	effects attached to render node

	THeapArray<TRef>			m_Debug_RenderDatums;	//	list of datums to debug-render

	TRef						m_OwnerSceneNode;		//	"Owner" scene node - if this is set then we automaticcly process some stuff (ie. catching OnTransform of a scene node we set our transform)
};

FORCEINLINE bool operator==(const TLRender::TRenderNode* pNode,const TRef& Ref)	{	return pNode ? (*pNode == Ref) : false;	}



namespace TLRender
{
	template<> FORCEINLINE const TLMaths::TShapeBox&		TRenderNode::GetLocalBounds()		{	CalcLocalBounds( m_BoundsBox.m_LocalShape );		return m_BoundsBox.m_LocalShape;	}
	template<> FORCEINLINE const TLMaths::TShapeBox2D&		TRenderNode::GetLocalBounds()		{	CalcLocalBounds( m_BoundsBox2D.m_LocalShape );		return m_BoundsBox2D.m_LocalShape;	}
	template<> FORCEINLINE const TLMaths::TShapeSphere&		TRenderNode::GetLocalBounds()		{	CalcLocalBounds( m_BoundsSphere.m_LocalShape );		return m_BoundsSphere.m_LocalShape;	}
	template<> FORCEINLINE const TLMaths::TShapeSphere2D&	TRenderNode::GetLocalBounds()		{	CalcLocalBounds( m_BoundsSphere2D.m_LocalShape );	return m_BoundsSphere2D.m_LocalShape;	}
}

//---------------------------------------------------------------
//	QuadTreeNode for render nodes
//---------------------------------------------------------------
class TLRender::TRenderZoneNode : public TLMaths::TQuadTreeNode
{
public:
	TRenderZoneNode(TRefRef RenderNodeRef);

	virtual SyncBool	IsInShape(const TLMaths::TBox2D& Shape);
	virtual Bool		HasZoneShape();

protected:
	TRef				m_RenderNodeRef;		//	render node that we're linked to
	TRenderNode*		m_pRenderNode;			//	cache of render node
};






//---------------------------------------------------------------
//	enable node colour if non-white
//---------------------------------------------------------------
FORCEINLINE void TLRender::TRenderNode::OnColourChanged()
{
	if ( m_Colour.GetRedf() > TLMaths_NearOne &&
		m_Colour.GetGreenf() > TLMaths_NearOne &&
		m_Colour.GetBluef() > TLMaths_NearOne &&
		m_Colour.GetAlphaf() > TLMaths_NearOne )
	{
		m_RenderFlags.Clear( RenderFlags::UseNodeColour );
	}
	else
	{
		m_RenderFlags.Set( RenderFlags::UseNodeColour );
	}
	OnPropertyChanged( Properties::RenderFlags );
	OnPropertyChanged( Properties::Colour );
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
		OnTransformChanged( TLMaths_TransformBitTranslate );	
}

FORCEINLINE void TLRender::TRenderNode::SetScale(const float3& Scale,Bool Invalidate)						
{	
	m_Transform.SetScale( Scale );	

	if ( Invalidate )	
		OnTransformChanged( TLMaths_TransformBitScale );	
}

FORCEINLINE void TLRender::TRenderNode::SetRotation(const TLMaths::TQuaternion& Rotation,Bool Invalidate)	
{	
	m_Transform.SetRotation( Rotation );	

	if ( Invalidate )	
		OnTransformChanged( TLMaths_TransformBitRotation );	
}

//---------------------------------------------------------------
//	invalidate bounds when pos/rot/scale
//---------------------------------------------------------------
FORCEINLINE void TLRender::TRenderNode::OnTransformChanged(u8 TransformChangedBits)						
{
	if ( TransformChangedBits != 0x0 )
	{
		Bool TranslateChanged = (TransformChangedBits & TLMaths_TransformBitTranslate) != 0x0;
		Bool ScaleChanged = (TransformChangedBits & TLMaths_TransformBitScale) != 0x0;
		Bool RotationChanged = (TransformChangedBits & TLMaths_TransformBitRotation) != 0x0;
		SetBoundsInvalid( TInvalidateFlags( 
							//HasChildren() ? InvalidateLocalBounds : InvalidateDummy, //	gr: not needed?
							TranslateChanged ? InvalidateWorldPos : InvalidateDummy,				//	world pos must have changed - may be able to reduce this to just Translate changes
							InvalidateWorldBounds,			//	shape of mesh must have changed
							InvalidateParentLocalBounds,	//	our parent's LOCAL bounds has now changed as it's based on it's children (this)
							InvalidateChildWorldBounds,		//	invalidate the children's world bounds
							InvalidateChildWorldPos		//	invalidate the children's world pos too
						) );	

		if ( TranslateChanged )	OnPropertyChanged( Properties::Translation );
		if ( ScaleChanged )		OnPropertyChanged( Properties::Scale );
		if ( RotationChanged )	OnPropertyChanged( Properties::Rotation );
	}
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



//---------------------------------------------------------------
//	get a datum of a specific type - returns NULL if it doesn't exist or is of a different type
//---------------------------------------------------------------
template<class SHAPETYPE> 
FORCEINLINE const SHAPETYPE* TLRender::TRenderNode::GetLocalDatum(TRefRef DatumRef)
{
	const TLMaths::TShape* pDatum = GetLocalDatum( DatumRef );
	if ( !pDatum )
		return NULL;

	//	check matching type
	if ( pDatum->GetShapeType() != SHAPETYPE::GetShapeType_Static() )
		return NULL;

	//	cast to desired type
	return static_cast<const SHAPETYPE*>( pDatum );
}


template<class SHAPETYPE> 
void TLRender::TRenderNode::CalcLocalBounds(SHAPETYPE& Shape)
{
	//	if bounds is valid, doesnt need recalculating
	if ( Shape.IsValid() )
		return;
	
	//	get bounds from mesh
	TLAsset::TMesh* pMesh = GetMeshAsset();
	if ( pMesh )
	{
		//	copy bounds of mesh to use as our own
		Shape = pMesh->GetBounds<SHAPETYPE>();
	}

	//	accumulate children's bounds
	TArray<TLRender::TRenderNode*>& NodeChildren = GetLocalBoundsChildren();
	for ( u32 c=0;	c<NodeChildren.GetSize();	c++ )
	{
		TLRender::TRenderNode& Child = *NodeChildren[c];

		//	don't accumualte a child that is does not have an inherited transform
		if ( Child.GetRenderFlags().IsSet(RenderFlags::ResetScene) )
			continue;
			
		//	get child's bounds
		const SHAPETYPE& ChildBounds = Child.GetLocalBounds<SHAPETYPE>();
		if ( !ChildBounds.IsValid() )
			continue;

		//	gr: need to omit translate?
		SHAPETYPE ChildBoundsTransformed = ChildBounds;
		ChildBoundsTransformed.m_Shape.Transform( Child.GetTransform() );

		//	accumulate child
		Shape.m_Shape.Accumulate( ChildBoundsTransformed.m_Shape );
	}
}



template<class SHAPETYPE>
const SHAPETYPE& TLMaths::TBoundsShape<SHAPETYPE>::CalcWorldShape(TLRender::TRenderNode& RenderNode)
{
	//	... world transform must be valid (or old/wait) and our bounds are not this new, so recalculate
	SyncBool IsWorldTransformValid = RenderNode.m_WorldTransformValid;

	//	check validity synchronoisation is correct
	if ( m_WorldValid == SyncTrue && IsWorldTransformValid != SyncTrue )
	{
		TLDebug_Break("Mis-match in age of world shape - shape should always be same or older as the transform");
		m_WorldValid = IsWorldTransformValid;
	}

	//	world bounds is up to date
	if ( m_WorldValid == IsWorldTransformValid )
		return m_WorldShape;

	//	world transform isn't valid at all
	if ( IsWorldTransformValid == SyncFalse )
	{
		//	gr: shouldn't be valid...
		m_WorldShape.SetInvalid();
		return m_WorldShape;
	}

	//	get/recalc local bounds box
	if ( !m_LocalShape.IsValid() )
	{
		RenderNode.CalcLocalBounds( m_LocalShape );

		//	still not valid
		if ( !m_LocalShape.IsValid() )
		{
			//	gr: shouldn't be valid...
			m_WorldShape.SetInvalid();
			return m_WorldShape;
		}
	}

	//	tranform our local bounds into the world bounds
	m_WorldShape = m_LocalShape;
	m_WorldShape.m_Shape.Transform( RenderNode.m_WorldTransform );
	
	//	update state (matches world transform state)
	m_WorldValid = IsWorldTransformValid;

	return m_WorldShape;
}


