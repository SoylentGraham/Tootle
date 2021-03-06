#pragma once

#include <TootleCore/TLGraph.h>
#include "TRenderNode.h"



namespace TLRender
{
	class TRendergraph;
	class TRenderNodeFactory;

	extern TPtr<TRendergraph> g_pRendergraph;
};


namespace TLGraph
{
	//	stores a message destined for a node
	class TNodeMessage
	{
	public:
		TNodeMessage(TRefRef NodeRef,TLMessaging::TMessage& Message);
	
		FORCEINLINE TLMessaging::TMessage&	GetMessage()		{	return m_Message;	}
		FORCEINLINE TRefRef					GetNodeRef() const	{	return m_NodeRef;	}

	protected:
		TRef					m_NodeRef;
		TLMessaging::TMessage	m_Message;
	};
};


//----------------------------------------------------------
//	TRendergraph class
//----------------------------------------------------------
class TLRender::TRendergraph : public TLGraph::TGraph<TLRender::TRenderNode>
{
public:
	TRendergraph() :
		TLGraph::TGraph<TLRender::TRenderNode>	( "Render" )
	{
	}

	virtual Bool				SendMessageToNode(TRefRef NodeRef,TLMessaging::TMessage& Message);	//	send message to node

protected:
	virtual SyncBool			Initialise();
	virtual SyncBool			Update(float fTimeStep);		//	alternative graph update
	virtual SyncBool			Shutdown();

protected:
	TPtrArray<TLGraph::TNodeMessage>	m_NodeMessages;	//	queued up messages going to nodes
};

TLGraph_DeclareGraph(TLRender::TRenderNode);



//----------------------------------------------------------
//	Generic render node factory
//----------------------------------------------------------
class TLRender::TRenderNodeFactory : public TNodeFactory<TRenderNode>
{
public:
	virtual TRenderNode*	CreateObject(TRefRef InstanceRef,TRefRef TypeRef);
};

