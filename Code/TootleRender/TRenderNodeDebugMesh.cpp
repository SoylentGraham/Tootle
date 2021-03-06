#include "TRenderNodeDebugMesh.h"




TLRender::TRenderNodeDebugMesh::TRenderNodeDebugMesh(TRefRef RenderNodeRef,TRefRef TypeRef) :
	TRenderNode		( RenderNodeRef, TypeRef )
{
}


//---------------------------------------------------------
//	init - create our mesh
//---------------------------------------------------------
void TLRender::TRenderNodeDebugMesh::Initialise(TLMessaging::TMessage& Message)
{
	//	do inherited init
	TRenderNode::Initialise( Message );

	//	make up mesh
	if ( !m_pDebugMesh )
	{
		m_pDebugMesh = new TLAsset::TMesh("Debug");
		m_pDebugMesh->SetLoadingState( TLAsset::LoadingState_Loaded );
	}
/*
	//	default debuggy flags
	GetRenderFlags().Set( TLRender::TRenderNode::RenderFlags::Debug_Wireframe );
	GetRenderFlags().Set( TLRender::TRenderNode::RenderFlags::Debug_ColourWireframe );
	GetRenderFlags().Clear( TLRender::TRenderNode::RenderFlags::DepthRead );
*/
}


