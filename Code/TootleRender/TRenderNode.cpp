#include "TRenderNode.h"
#include "TRendergraph.h"

#include <TootleScene/TScenegraph.h>

//#define DEBUG_PrintBoundsInvalidationChanges


void Debug_PrintInvalidate(const TLRender::TRenderNode* pObject,const char* pSpaceType,const char* pShapeType)
{
#ifdef DEBUG_PrintBoundsInvalidationChanges
	TTempString DebugString;
	pObject->GetRenderNodeRef().GetString( DebugString );
	DebugString.Appendf(" %s %s invalidated", pSpaceType, pShapeType );
	TLDebug_Print( DebugString );
#endif
}


void Debug_PrintCalculating(const TLRender::TRenderNode* pObject,const char* pSpaceType,const char* pShapeType)
{
#ifdef DEBUG_PrintBoundsInvalidationChanges
	TTempString DebugString;
	pObject->GetRenderNodeRef().GetString( DebugString );
	DebugString.Appendf(" %s %s calculating...", pSpaceType, pShapeType );
	TLDebug_Print( DebugString );
#endif
}





TLRender::TRenderZoneNode::TRenderZoneNode(TRefRef RenderNodeRef) :
	m_RenderNodeRef		( RenderNodeRef ),
	m_pRenderNode		( TLRender::g_pRendergraph->FindNode( m_RenderNodeRef ) )
{
}


//------------------------------------------------------------------
//	test if we are inside this zone's shape
//------------------------------------------------------------------
SyncBool TLRender::TRenderZoneNode::IsInShape(const TLMaths::TBox2D& Shape)
{
	//	grab render node if we don't have it
	if ( !m_pRenderNode )
	{
		m_pRenderNode = TLRender::g_pRendergraph->FindNode( m_RenderNodeRef );
	}

	//	get c-pointer
	TLRender::TRenderNode* pRenderNode = m_pRenderNode;
	if ( !pRenderNode )
	{
#ifdef _DEBUG
		TTempString Debug_String("TRenderZoneNode is linked to a node ");
		m_RenderNodeRef.GetString( Debug_String );
		Debug_String.Append(" that doesnt exist?");
		TLDebug_Print(Debug_String);
#endif
		return SyncFalse;
	}

	const TLMaths::TBox2D& ZoneShape = Shape;

	//	test world pos first, quickest test :)
	Bool WorldPosIsValid;
	const float3& WorldPos = pRenderNode->GetWorldPos( WorldPosIsValid );
	if ( !WorldPosIsValid )
	{
		//	gr: if world pos is not valid, then MOST likely this is being called as a zone has been split and we're testing to
		//		see if the node needs moving. As we've not rendered yet, we might be out of date... all of the other bounds
		//		should be out of date too, so just wait and we'll move when we can...
		//TLDebug_Break("World pos is not valid during RenderNode Zone test");
		return SyncWait;
	}

	if ( ZoneShape.GetIntersection( WorldPos ) )
		return SyncTrue;

	Bool AnyBoundsValid = FALSE;

	//	find a valid world bounds to test with (go in fastest order...)
	const TLMaths::TSphere& WorldBoundsSphere = pRenderNode->GetWorldBoundsSphere();
	if ( WorldBoundsSphere.IsValid() )
	{
		if ( ZoneShape.GetIntersection( WorldBoundsSphere ) )
			return SyncTrue;
		AnyBoundsValid = TRUE;
	}

	//	gr: this is so expensive, we're not going to use it
	/*
	const TLMaths::TBox& WorldBoundsBox = pRenderNode->GetWorldBoundsBox();
	if ( WorldBoundsBox.IsValid() )
	{
		if ( ZoneShape.GetIntersection( WorldBoundsBox ) )
			return SyncTrue;
		AnyBoundsValid = TRUE;
	}
	*/

	//	none of the bounds are valid, this is bad
	if ( !AnyBoundsValid )
	{
#ifdef _DEBUG
		//	if there is no mesh, then this is understandable and wont throw up an error
		TPtr<TLAsset::TMesh>& pMesh = pRenderNode->GetMeshAsset();
		if ( pMesh )
		{
			//	gr: if we get here whilst the zone we're in is splitting, then we return Wait, 
			//	which means the node isn't ready to be moved around
			//	this is a case of the zone is splitting, but our stuff is invalidated, so cant tell
			//	where we are any more
			//	although in this case I have, the ZoneOutOfDate == FALSE, but our world bounds ARE invalid?
			//TLDebug_Break("No valid world bounds shapes during RenderNode Zone test");
			return SyncWait;
		}
#endif
	}

	//	none of our bounds shapes intersected this zone's shape
	return SyncFalse;
}








TLRender::TRenderNode::TRenderNode(TRefRef RenderNodeRef,TRefRef TypeRef) :
	TLGraph::TGraphNode<TLRender::TRenderNode>	( RenderNodeRef, TypeRef ),
	m_Data						( "Data" ),
	m_LineWidth					( 0.f ),
	m_WorldPosValid				( SyncFalse ),
	m_WorldBoundsBoxValid		( SyncFalse ),
	m_WorldBoundsSphereValid	( SyncFalse ),
	m_WorldTransformValid		( SyncFalse ),
	m_Colour					( 1.f, 1.f, 1.f, 1.f )
{
	//	setup defualt render flags
	m_RenderFlags.Set( RenderFlags::DepthRead );
	m_RenderFlags.Set( RenderFlags::DepthWrite );
	m_RenderFlags.Set( RenderFlags::Enabled );
	m_RenderFlags.Set( RenderFlags::UseVertexColours );
	m_RenderFlags.Set( RenderFlags::UseMeshLineWidth );
	m_RenderFlags.Set( RenderFlags::EnableCull );
	m_RenderFlags.Set( RenderFlags::InvalidateBoundsByChildren );
}



//------------------------------------------------------------
//	copy render object DOES NOT COPY CHILDREN or parent! just properties
//------------------------------------------------------------
void TLRender::TRenderNode::Copy(const TRenderNode& OtherRenderNode)
{
	TLDebug_Break("still used? - code is out of date");
	/*
	m_Transform				= OtherRenderNode.m_Transform;
	m_Colour				= OtherRenderNode.m_Colour;
	m_LocalBoundsBox		= OtherRenderNode.m_LocalBoundsBox;
	m_WorldBoundsBox		= OtherRenderNode.m_WorldBoundsBox;
	m_LocalBoundsSphere		= OtherRenderNode.m_LocalBoundsSphere;
	m_WorldBoundsSphere		= OtherRenderNode.m_WorldBoundsSphere;
	m_RenderFlags			= OtherRenderNode.m_RenderFlags;
	m_MeshRef				= OtherRenderNode.m_MeshRef;
	m_Data					= OtherRenderNode.m_Data;

	SetRenderNodeRef( OtherRenderNode.GetRenderNodeRef() );
	*/
}

//------------------------------------------------------------
//	default behaviour fetches the mesh from the asset lib with our mesh ref
//------------------------------------------------------------
TPtr<TLAsset::TMesh>& TLRender::TRenderNode::GetMeshAsset()
{
	//	re-fetch mesh if we need to
	if ( GetMeshRef().IsValid() && !m_pMeshCache )
	{
		m_pMeshCache = TLAsset::GetAsset( GetMeshRef(), TRUE );
		
		//	got a mesh, check it's the right type
#ifdef _DEBUG
		if ( m_pMeshCache )
		{
			if ( m_pMeshCache->GetAssetType() != "mesh" )
			{
				TLDebug_Break("MeshRef of render node is not a mesh");
				m_pMeshCache = NULL;
			}
		}
#endif
	}

	return m_pMeshCache;
}


//------------------------------------------------------------
//	pre-draw routine for a render object
//------------------------------------------------------------
Bool TLRender::TRenderNode::Draw(TRenderTarget* pRenderTarget,TRenderNode* pParent,TPtrArray<TRenderNode>& PostRenderList)
{
	//	gr: todo: merge flags, colours etc from parent
	return TRUE;
}		

//------------------------------------------------------------
//	calculate our local bounds box (accumulating children) if out of date and return it
//	SceneMatrix and SceneScale include OUR local matrix/scale
//------------------------------------------------------------
const TLMaths::TBox& TLRender::TRenderNode::GetWorldBoundsBox()
{
	//	world bounds is up to date
	if ( m_WorldBoundsBoxValid == m_WorldTransformValid )
		return m_WorldBoundsBox;

	//	world transform isn't calculated
	if ( m_WorldTransformValid == SyncFalse )
	{
		//	gr: shouldn't be valid...
		m_WorldBoundsBox.SetInvalid();
		return m_WorldBoundsBox;
	}

	//	... world transform must be valid (or old/wait) and our bounds are not this new, so recalculate

	//	get/recalc local bounds box
	if ( !CalcLocalBoundsBox().IsValid() )
	{
		//	gr: shouldn't be valid...
		m_WorldBoundsBox.SetInvalid();
		return m_WorldBoundsBox;
	}
	
	Debug_PrintCalculating( this, "world", "box" );

	//	tranform our local bounds into the world bounds
	m_WorldBoundsBox = m_LocalBoundsBox;
	m_WorldBoundsBox.Transform( m_WorldTransform );
	
	//	update state (matches world transform state)
	m_WorldBoundsBoxValid = m_WorldTransformValid;

	return m_WorldBoundsBox;
}


//------------------------------------------------------------
//	return our current local bounds box and calculate if invalid
//------------------------------------------------------------
const TLMaths::TBox& TLRender::TRenderNode::CalcLocalBoundsBox()
{
	//	if bounds is valid, doesnt need recalculating
	if ( m_LocalBoundsBox.IsValid() )
		return m_LocalBoundsBox;
	
	Debug_PrintCalculating( this, "local", "box" );

	//	get bounds from mesh
	TPtr<TLAsset::TMesh>& pMesh = GetMeshAsset();
	if ( pMesh )
	{
		TLMaths::TBox& MeshBounds = pMesh->CalcBoundsBox();
		
		//	copy bounds of mesh to use as our own
		if ( MeshBounds.IsValid() )
		{
			m_LocalBoundsBox = MeshBounds;
		}
	}

	//	no children, just return what we have
	if ( !HasChildren() )
	{
		/*//	gr: this doesnt work, doesnt get invalidated when mesh does turn up...
		//	no bounds from mesh, and no children, so make up a VALID, but empty bounds box
		if ( !m_LocalBoundsBox.IsValid() )
		{
			Debug_PrintCalculating( this, "local", "EMPTY BOX" );
			m_LocalBoundsBox.Set( float3(0,0,0), float3(0,0,0) );
		}
		*/
		
		return m_LocalBoundsBox;
	}

	//	accumulate children's bounds
	TPtrArray<TLRender::TRenderNode>& NodeChildren = GetChildren();
	for ( u32 c=0;	c<NodeChildren.GetSize();	c++ )
	{
		TLRender::TRenderNode& Child = *NodeChildren[c];

		//	don't accumualte a child that is does not have an inherited transform
		if ( Child.GetRenderFlags().IsSet(RenderFlags::ResetScene) )
			continue;
			
		//	get child's bounds
		const TLMaths::TBox& ChildBounds = Child.CalcLocalBoundsBox();
		if ( !ChildBounds.IsValid() )
			continue;

		//	gr: need to omit translate?
		TLMaths::TBox TransformedChildBounds = ChildBounds;
		TransformedChildBounds.Transform( Child.GetTransform() );

		//	accumulate child
		m_LocalBoundsBox.Accumulate( TransformedChildBounds );
	}

	//	all done! invalid or not, this is our bounds
	return m_LocalBoundsBox;
}


//------------------------------------------------------------
//	mark out bounds box as invalid, and invalidate parents bounds too
//------------------------------------------------------------
void TLRender::TRenderNode::SetBoundsInvalid(const TInvalidateFlags& InvalidateFlags)
{
	//	if we're set NOT to invalidate on child changes, dont
	if ( InvalidateFlags( FromChild ) && !GetRenderFlags().IsSet( RenderFlags::InvalidateBoundsByChildren ) )
	{
		//	unless it's forced
		if ( !InvalidateFlags( ForceInvalidateParentsLocalBounds ) )
			return;
	}

	Bool InvWorld = InvalidateFlags(InvalidateWorldBounds);
	Bool InvLocal = InvalidateFlags(InvalidateLocalBounds);
	Bool InvPos = InvalidateFlags(InvalidateWorldPos);

	//	no change
	if ( !InvLocal && !InvWorld && !InvPos )
		return;

	Bool ThisLocalBoundsChanged = FALSE;
	Bool ThisWorldBoundsChanged = FALSE;
	Bool HasSetRenderZoneInvalid = FALSE;


	//	invalidate local bounds
	if ( InvLocal )
	{
		if ( m_LocalBoundsBox.IsValid()  )
		{
			Debug_PrintInvalidate( this, "local", "box" );
			m_LocalBoundsBox.SetInvalid();
			ThisLocalBoundsChanged = TRUE;
		}

		if ( m_LocalBoundsSphere.IsValid()  )
		{
			Debug_PrintInvalidate( this, "local", "Sphere" );
			m_LocalBoundsSphere.SetInvalid();
			ThisLocalBoundsChanged = TRUE;
		}
	}

	//	if invalidating local, world must be invalidated (but only do it if the local bounds were changed)
	if ( InvWorld || (InvLocal&&ThisLocalBoundsChanged) )
	{
		//	downgrade validation of world shapes
		SetWorldTransformOld();
		Debug_PrintInvalidate( this, "local", "all" );

		//	invalidate the zone of our RenderNodeZones - if our world bounds has changed then we
		//	may have moved to a new zone
		if ( !HasSetRenderZoneInvalid )
		{
			for ( u32 z=0;	z<m_RenderZoneNodes.GetSize();	z++ )
				m_RenderZoneNodes.ElementAt(z)->SetZoneOutOfDate();
			HasSetRenderZoneInvalid = TRUE;
		}
	}

	//	invalidate world pos
	if ( InvPos )
	{
		if ( m_WorldPosValid == SyncTrue )
			m_WorldPosValid = SyncWait;

		if ( !HasSetRenderZoneInvalid )
		{
			for ( u32 z=0;	z<m_RenderZoneNodes.GetSize();	z++ )
				m_RenderZoneNodes.ElementAt(z)->SetZoneOutOfDate();
			HasSetRenderZoneInvalid = TRUE;
		}
	}

	//	invalidate parent if local changes
	if ( InvalidateFlags(InvalidateParentLocalBounds) || InvalidateFlags(ForceInvalidateParentsLocalBounds) )
	{
		TRenderNode* pParent = GetParent();
		if ( pParent )
		{
			//	get parent's invalidate flags
			TInvalidateFlags ParentInvalidateFlags;
			ParentInvalidateFlags.Set( FromChild );
			ParentInvalidateFlags.Set( InvalidateLocalBounds );

			//	gr: unless explicitly set, dont invalidate all of the parent's children
			if ( ParentInvalidateFlags(InvalidateParentsChildrenWorldBounds) )
				ParentInvalidateFlags.Set( InvalidateChildWorldBounds );
			if ( ParentInvalidateFlags(InvalidateParentsChildrenWorldPos) )
				ParentInvalidateFlags.Set( InvalidateChildWorldPos );

			//	invalidate parent
			pParent->SetBoundsInvalid(ParentInvalidateFlags);
		}
	}

	//	invalidate world bounds of children
	if ( HasChildren() && (InvalidateFlags(InvalidateChildWorldBounds) || InvalidateFlags(InvalidateChildWorldPos) )  )
	{
		//	calculate child's invalidate flags
		TInvalidateFlags ChildInvalidateFlags;
		ChildInvalidateFlags.Set( FromParent );

		if ( InvalidateFlags(InvalidateChildWorldBounds) )
		{
			ChildInvalidateFlags.Set( InvalidateWorldBounds );
			ChildInvalidateFlags.Set( InvalidateChildWorldBounds );
		}

		if ( InvalidateFlags(InvalidateChildWorldPos) )
		{
			ChildInvalidateFlags.Set( InvalidateWorldPos );
			ChildInvalidateFlags.Set( InvalidateChildWorldPos );
		}

#ifdef TLGRAPH_OWN_CHILDREN
		TPtrArray<TLRender::TRenderNode>& NodeChildren = GetChildren();
		for ( u32 c=0;	c<NodeChildren.GetSize();	c++ )
		{
			TLRender::TRenderNode* pChild = NodeChildren[c];
#else
		TPtr<TRenderNode> pChild = GetChildFirst();
		while ( pChild )
		{
#endif
			pChild->SetBoundsInvalid( ChildInvalidateFlags );
			
			#ifndef TLGRAPH_OWN_CHILDREN
			pChild = pChild->GetNext();
			#endif
		}
	}
}


//------------------------------------------------------------
//	calculate our new world position from the latest scene transform
//------------------------------------------------------------
const float3& TLRender::TRenderNode::GetWorldPos()
{
	//	validity up to date
	if ( m_WorldPosValid == m_WorldTransformValid )
		return m_WorldPos;

	//	cant calculate
	if ( m_WorldTransformValid == SyncFalse )
	{
		m_WorldPosValid = SyncFalse;
		return m_WorldPos;
	}

	//	calc new world pos

	//	"center" of local node is the base world position
	if ( m_Transform.HasTranslate() )
		m_WorldPos.Set( m_Transform.GetTranslate() );
	else
		m_WorldPos.Set( 0.f, 0.f, 0.f );

	//	transform
	m_WorldTransform.TransformVector( m_WorldPos );

	//	is as valid as the transform we just applied
	m_WorldPosValid = m_WorldTransformValid;

	return m_WorldPos;
}

//------------------------------------------------------------
//	
//------------------------------------------------------------
const TLMaths::TSphere& TLRender::TRenderNode::GetWorldBoundsSphere()
{
	//	world bounds is up to date
	if ( m_WorldBoundsSphereValid == m_WorldTransformValid )
		return m_WorldBoundsSphere;

	//	world transform isn't calculated
	if ( m_WorldTransformValid == SyncFalse )
	{
		//	gr: shouldn't be valid...
		m_WorldBoundsSphere.SetInvalid();
		return m_WorldBoundsSphere;
	}

	//	... world transform must be valid (or old/wait) and our bounds are not this new, so recalculate

	//	get/recalc local bounds box
	if ( !CalcLocalBoundsSphere().IsValid() )
	{
		//	gr: shouldn't be valid...
		m_WorldBoundsSphere.SetInvalid();
		return m_WorldBoundsSphere;
	}

	Debug_PrintCalculating( this, "world", "sphere" );

	//	tranform our local bounds into the world bounds
	m_WorldBoundsSphere = m_LocalBoundsSphere;
	m_WorldBoundsSphere.Transform( m_WorldTransform );
	
	//	update state (matches world transform state)
	m_WorldBoundsSphereValid = m_WorldTransformValid;

	return m_WorldBoundsSphere;
}


//------------------------------------------------------------
//	
//------------------------------------------------------------
const TLMaths::TSphere& TLRender::TRenderNode::CalcLocalBoundsSphere()
{
	//	if bounds is valid, doesnt need recalculating
	if ( m_LocalBoundsSphere.IsValid() )
		return m_LocalBoundsSphere;

	//	get bounds from mesh
	TPtr<TLAsset::TMesh>& pMesh = GetMeshAsset();
	if ( pMesh )
	{
		//	copy bounds of mesh to use as our own
		const TLMaths::TSphere& MeshBounds = pMesh->CalcBoundsSphere();	
		if ( MeshBounds.IsValid() )
		{
			m_LocalBoundsSphere = MeshBounds;
		}
	}

	//	no children, just return what we have
	if ( !HasChildren() )
		return m_LocalBoundsSphere;

	//	accumulate children's bounds
	TPtrArray<TLRender::TRenderNode>& NodeChildren = GetChildren();
	for ( u32 c=0;	c<NodeChildren.GetSize();	c++ )
	{
		TLRender::TRenderNode& Child = *NodeChildren[c];
		
		if ( Child.GetRenderFlags().IsSet(RenderFlags::ResetScene) )
			continue;

		//	get child's bounds
		const TLMaths::TSphere& ChildBounds = Child.CalcLocalBoundsSphere();
		if ( !ChildBounds.IsValid() )
			continue;

		//	gr: omit translate?
		TLMaths::TSphere TransformedChildBounds = ChildBounds;
		TransformedChildBounds.Transform( Child.GetTransform() );

		//	accumulate child
		m_LocalBoundsSphere.Accumulate( TransformedChildBounds );
	}

	//	all done! invalid or not, this is our bounds
	return m_LocalBoundsSphere;
}


void TLRender::TRenderNode::ClearDebugRenderFlags()						
{
	m_RenderFlags.Clear( RenderFlags::Debug_Wireframe );
	m_RenderFlags.Clear( RenderFlags::Debug_Points );
	m_RenderFlags.Clear( RenderFlags::Debug_Outline );

	m_RenderFlags.Clear( RenderFlags::Debug_LocalBoundsBox );
	m_RenderFlags.Clear( RenderFlags::Debug_WorldBoundsBox );
	m_RenderFlags.Clear( RenderFlags::Debug_LocalBoundsSphere );
	m_RenderFlags.Clear( RenderFlags::Debug_WorldBoundsSphere );
}


void TLRender::TRenderNode::OnAdded()
{
	TLGraph::TGraphNode<TLRender::TRenderNode>::OnAdded();
/*	//	gr: printing out too often atm
#ifdef _DEBUG
	TTempString RefString;
	GetRenderNodeRef().GetString( RefString );
	RefString.Append(" added to graph... invalidating...");
	TLDebug_Print( RefString );
#endif
*/
	//	invalidate bounds of self IF child affects bounds
	if ( !GetRenderFlags().IsSet( RenderFlags::ResetScene ) )
	{
		SetBoundsInvalid( TInvalidateFlags( InvalidateLocalBounds, ForceInvalidateParentsLocalBounds ) );
	}
}


//---------------------------------------------------------
//	generic render node init
//---------------------------------------------------------
void TLRender::TRenderNode::Initialise(TLMessaging::TMessage& Message)
{
	//	read init data
	/*
#ifdef _DEBUG
		TLDebug_Print("Init message data: ");
		Message.Debug_PrintTree();
#endif
		*/

	if(Message.ImportData("Owner", m_OwnerSceneNode))
	{
		// Get the scenegraph node
		TPtr<TLScene::TSceneNode> pOwner = TLScene::g_pScenegraph->FindNode(m_OwnerSceneNode);

		if(pOwner.IsValid())
		{
			pOwner->SubscribeTo(this);
			SubscribeTo(pOwner);

			/*
			TPtr<TLMessaging::TEventChannel>& pEventChannel = pOwner->FindEventChannel("OnTransform");

			if(pEventChannel)
			{
				// Subscribe to the scene node owners transform channel
				SubscribeTo(pEventChannel);

				// Subscribe the 'scene' node owner to this node so we can sen audio change messages
				pOwner->SubscribeTo(this);
			}
			*/
		}
	}




	Bool TransformChanged = FALSE;

	if ( Message.ImportData("Translate", m_Transform.GetTranslate() ) == SyncTrue )
	{
		m_Transform.SetTranslateValid();
		TransformChanged = TRUE;
	}

	if ( Message.ImportData("Scale", m_Transform.GetScale() ) == SyncTrue )
	{
		m_Transform.SetScaleValid();
		TransformChanged = TRUE;
	}

	if ( Message.ImportData("Rotation", m_Transform.GetRotation() ) == SyncTrue )
	{
		m_Transform.SetRotationValid();
		TransformChanged = TRUE;
	}

	//	transform has been set
	if ( TransformChanged )
		OnTransformChanged();

	Message.ImportData("LineWidth", m_LineWidth );

	if ( Message.ImportData("MeshRef", m_MeshRef ) == SyncTrue )
	{
		//	start loading the asset in case we havent loaded it already
		TLAsset::LoadAsset( m_MeshRef );

		//	mesh ref changed
		OnMeshRefChanged();
	}

	//	get render flags to set
	TPtrArray<TBinaryTree> FlagChildren;
	if ( Message.GetChildren("RFSet", FlagChildren ) )
	{
		u32 RenderFlagIndex = 0;
		for ( u32 f=0;	f<FlagChildren.GetSize();	f++ )
		{
			FlagChildren[f]->ResetReadPos();
			if ( FlagChildren[f]->Read( RenderFlagIndex ) )
				GetRenderFlags().Set( (RenderFlags::Flags)RenderFlagIndex );
		}
		FlagChildren.Empty();
	}

	//	get render flags to clear
	if ( Message.GetChildren("RFClear", FlagChildren ) )
	{
		u32 RenderFlagIndex = 0;
		for ( u32 f=0;	f<FlagChildren.GetSize();	f++ )
		{
			FlagChildren[f]->ResetReadPos();
			if ( FlagChildren[f]->Read( RenderFlagIndex ) )
				GetRenderFlags().Clear( (RenderFlags::Flags)RenderFlagIndex );
		}
		FlagChildren.Empty();
	}

	//	import colour
	if ( Message.ImportData("Colour", m_Colour ) )
		OnColourChanged();

	//	do inherited init
	TLGraph::TGraphNode<TLRender::TRenderNode>::Initialise( Message );
}



//---------------------------------------------------------
//	clean-up any TPtrs back to us so we will be deallocated
//---------------------------------------------------------
void TLRender::TRenderNode::Shutdown()
{
	//	these contain TPtr's back to us, so we need to clear them
	m_RenderZoneNodes.Empty();

	//	inherited cleanup
	TLGraph::TGraphNode<TLRender::TRenderNode>::Shutdown();
}


//---------------------------------------------------------
//	
//---------------------------------------------------------
void TLRender::TRenderNode::ProcessMessage(TLMessaging::TMessage& Message)
{
	//	gr: only apply the change if it comes from our owner scene node
	if ( Message.GetMessageRef() == "OnTransform" && Message.GetSenderRef() == GetOwnerSceneNodeRef() && GetOwnerSceneNodeRef().IsValid() )
	{
		Bool TransformChanged = FALSE;

		if ( Message.ImportData("Translate", m_Transform.GetTranslate() ) == SyncTrue )
		{
			m_Transform.SetTranslateValid();
			TransformChanged = TRUE;
		}

		if ( Message.ImportData("Scale", m_Transform.GetScale() ) == SyncTrue )
		{
			m_Transform.SetScaleValid();
			TransformChanged = TRUE;
		}

		if ( Message.ImportData("Rotation", m_Transform.GetRotation() ) == SyncTrue )
		{
			m_Transform.SetRotationValid();
			TransformChanged = TRUE;
		}

		//	transform has been set
		if ( TransformChanged )
			OnTransformChanged();

		return;
	}

	//	do inherited init
	TLGraph::TGraphNode<TLRender::TRenderNode>::ProcessMessage( Message );
}


//---------------------------------------------------------
//	set new world transform
//---------------------------------------------------------
void TLRender::TRenderNode::SetWorldTransform(const TLMaths::TTransform& SceneTransform)
{
	//	if old transform is still valid, dont do anything
	if ( m_WorldTransformValid == SyncTrue )
	{
		//	SceneTransform should match our m_WorldTransform
		return;
	}

	//	downgrade the valid status of our world shapes/datums (to "old") if they were valid...
	SetWorldTransformOld();

	//	store new world transform
	m_WorldTransform = SceneTransform;
	m_WorldTransformValid = SyncTrue;

}


//---------------------------------------------------------
//	downgrade all world shape/transform states from valid to old
//---------------------------------------------------------
void TLRender::TRenderNode::SetWorldTransformOld()
{
	if ( m_WorldTransformValid == SyncTrue )
		m_WorldTransformValid = SyncWait;

	if ( m_WorldPosValid == SyncTrue )
		m_WorldPosValid = SyncWait;

	if ( m_WorldBoundsBoxValid == SyncTrue )
		m_WorldBoundsBoxValid = SyncWait;

	if ( m_WorldBoundsSphereValid == SyncTrue )
		m_WorldBoundsSphereValid = SyncWait;

}

