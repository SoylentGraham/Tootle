#include "TScreen.h"
#include "TRenderTarget.h"
#include "TRenderNodeText.h"
#include "TRenderGraph.h"
#include <TootleOpenglRasteriser/OpenglRasteriser.h>



//---------------------------------------------------------
//	
//---------------------------------------------------------
TLRender::TScreen::TScreen(TRefRef ScreenRef,const Type2<u16>& Size,TScreenShape ScreenShape) :
	m_HasShutdown	( false ),
	m_Ref			( ScreenRef ),
	m_InitialSize	( Size ),
	m_ScreenShape	( ScreenShape )
{
	//	gr: disabled for now, core manager limits frame rate instead of using hardware sync
	//m_Flags.Set( Flag_SyncFrameRate );
}


//---------------------------------------------------------
//	
//---------------------------------------------------------
TLRender::TScreen::~TScreen()
{
	//	if we haven't had a shutdown invoke an emergency shutdown
	if ( !m_HasShutdown )
		Shutdown();
}


//----------------------------------------------------------
//	create window
//----------------------------------------------------------
SyncBool TLRender::TScreen::Init()
{
	//	already initialised with window
	if ( m_pWindow )
		return SyncTrue;
	
	//	allocate window
	m_pWindow = TLGui::CreateGuiWindow( GetRef() );
	if ( !m_pWindow )
		return SyncFalse;
	
	//	set size
	m_pWindow->SetSize( m_InitialSize );
	
	//	center the window
	int4 WindowDimensions;
	WindowDimensions.zw() = m_pWindow->GetSize();
	GetCenteredSize( WindowDimensions );
	m_pWindow->SetPosition( WindowDimensions.xy() );

	//	create opengl canvas
	m_pCanvas = TLGui::CreateOpenglCanvas( *m_pWindow, GetRef() );
	if ( !m_pCanvas )
		return SyncFalse;
	
	//	create rasteriser to go with the canvas
	m_pCanvas->MakeCurrent();
	m_pRasteriser = new TLRaster::OpenglRasteriser();
	if ( !m_pRasteriser || !m_pRasteriser->Initialise() )
	{
		m_pRasteriser = NULL;
		m_pCanvas = NULL;
		return SyncFalse;
	}
	
	//	make the window visible
	m_pWindow->Show();
	
	// Subscirbe to the window
	SubscribeTo(m_pWindow);
	
	return SyncTrue;
}


//---------------------------------------------------------
//	do screen update
//---------------------------------------------------------
SyncBool TLRender::TScreen::Update()
{
	//	lost window?
	if ( !m_pWindow )
		return SyncFalse;
	
	//	continue
	return SyncTrue;
}



//---------------------------------------------------------
//	clean up
//---------------------------------------------------------
SyncBool TLRender::TScreen::Shutdown()
{
	//	already done a successfull shutdown
	if ( m_HasShutdown )
		return SyncTrue;

	SyncBool ShutdownResult = SyncTrue;

	//	clean up render targets
	if ( m_RenderTargets.GetSize() )
	{
		for ( s32 r=m_RenderTargets.GetSize()-1;	r>=0;	r-- )
		{
			/*
			SyncBool Result = m_RenderTargets[r]->Shutdown();
			
			//	error
			if ( Result == SyncFalse )
				return SyncFalse;

			//	wait
			if ( Result == SyncWait )
			{
				ShutdownResult = SyncWait;
				continue;
			}
			*/

			//	shutdown okay, release
			m_RenderTargets[r] = NULL;
			m_RenderTargets.RemoveAt( r );
		}
	}

	//	mark as shutdown if everything has succeeded
	if ( ShutdownResult == SyncTrue )
		m_HasShutdown = TRUE;

	m_pCanvas = NULL;
	m_pWindow = NULL;
	
	return ShutdownResult;
}

	

//---------------------------------------------------------
//	render the render targets
//---------------------------------------------------------
void TLRender::TScreen::Draw()
{
	//	get the rasteriser
	if ( !m_pRasteriser )
		return;
	TLRaster::TRasteriser& Rasteriser = *m_pRasteriser;

	//	render each render target in z order
	m_RenderTargets.Sort();
	for ( u32 r=0;	r<m_RenderTargets.GetSize();	r++ )
	{
		//	get render target
		TRenderTarget* pRenderTarget = m_RenderTargets[r];
		if ( !pRenderTarget )
			continue;

		//	check is enabled
		TRenderTarget& RenderTarget = *pRenderTarget;
		if ( !RenderTarget.IsEnabled() )
			continue;

		//	get the size... fails if it's too small to be of any use
		Type4<s32> RenderTargetSize;
		if ( !GetRenderTargetSize( RenderTargetSize, RenderTarget ) )
			continue;
	
		//	draw the render target's data into the rasteriser (this traverses the render tree)
		RenderTarget.Draw( Rasteriser );
	
		//	rasterise the rasterer
		Rasteriser.Rasterise( RenderTarget, *this );
	}	
	
	//	flip buffers - gr: this should be in the rasteriser?
	m_pCanvas->EndRender();
}


TPtr<TLRender::TRenderTarget> TLRender::TScreen::CreateRenderTarget(TRefRef TargetRef)
{
	// Check to make sure a render target with the specified name doesn;t already exist
	if ( m_RenderTargets.Exists( TargetRef ) )
		return NULL;

	// Create a new render target and add it to the list
	TPtr<TLRender::TRenderTarget> pRenderTarget = new TRenderTarget(TargetRef);

	if(!pRenderTarget)
		return NULL;

	//	add render target to list
	m_RenderTargets.Add( pRenderTarget );

	//	resort render targets by z
	OnRenderTargetZChanged( *pRenderTarget );

	return pRenderTarget;
}


//---------------------------------------------------------
//	fetch render target
//---------------------------------------------------------
TPtr<TLRender::TRenderTarget>& TLRender::TScreen::GetRenderTarget(const TRef& TargetRef)
{
	return m_RenderTargets.FindPtr( TargetRef );
}


//---------------------------------------------------------
//	shutdown a render target
//---------------------------------------------------------
SyncBool TLRender::TScreen::DeleteRenderTarget(const TRef& TargetRef)
{
	//	find the active render target index 
	s32 Index = m_RenderTargets.FindIndex(TargetRef);

	//	doesnt exist
	if ( Index == -1 )
	{
		//	if it's in the shutdown list, return wait
		if ( m_ShutdownRenderTargets.Exists( TargetRef ) )
			return SyncWait;

		//	non-existant target ref
		return SyncFalse;
	}

	//	grab pointer to the render target
	TPtr<TRenderTarget> pRenderTarget = m_RenderTargets[Index];

	//	remove from render target list
	m_RenderTargets.RemoveAt( (u32)Index );

	pRenderTarget = NULL;
	/*
	//	shutdown render target
	SyncBool Result = pRenderTarget->Shutdown();

	//	instant shutdown, so destroy
	if ( Result != SyncWait )
	{
		pRenderTarget = NULL;
		return Result;
	}

	//	is shutting down, move to shutdown list
	m_ShutdownRenderTargets.Add( pRenderTarget );
	*/

	return SyncWait;
}



Bool TLRender::TScreen::GetRenderTargetSize(Type4<s32>& Size,TRefRef TargetRef) 
{	
	const TRenderTarget* pRenderTarget = GetRenderTarget( TargetRef );	
	return pRenderTarget ? GetRenderTargetSize( Size, *pRenderTarget ) : FALSE;	
}

//---------------------------------------------------------
//	get the dimensions of a render target
//---------------------------------------------------------
Bool TLRender::TScreen::GetRenderTargetSize(Type4<s32>& Size,const TRenderTarget& RenderTarget)
{
	Type4<s32> RenderTargetMaxSize = GetRenderTargetMaxSize();
	RenderTarget.GetSize( Size, RenderTargetMaxSize );

	return true;
}




//---------------------------------------------------------
//	Get a render target-relative cursor position from a screen pos - fails if outside render target box
//---------------------------------------------------------
Bool TLRender::TScreen::GetRenderTargetPosFromScreenPos(const TRenderTarget& RenderTarget,Type2<s32>& RenderTargetPos,Type4<s32>& RenderTargetSize,const Type2<s32>& ScreenPos)
{
	//	check the point is inside the screen viewport
	Type4<s32> ViewportMaxSize = GetViewportMaxSize();

	if ( !ViewportMaxSize.GetIsInside( ScreenPos ) )
		return FALSE;

	//	convert screen(viewport) pos to render target pos by rotating it inside the viewport
	RenderTargetPos = ScreenPos;

	Type4<s32> MaxRenderTargetSize = GetRenderTargetMaxSize();

	//	rotate screen pos to be in "render target" space
	if ( GetScreenShape() == TLRender::ScreenShape_WideLeft )
	{
		//	rotate RIGHT
		RenderTargetPos.Left() = MaxRenderTargetSize.Right()  - ScreenPos.Top();
	//	RenderTargetPos.Left() = ViewportMaxSize.Right()  - ScreenPos.Top();
		RenderTargetPos.Top() = ScreenPos.Left();
	}
	else if ( GetScreenShape() == TLRender::ScreenShape_WideRight )
	{
		//	rotate LEFT
		RenderTargetPos.Left() = ScreenPos.Top();
	//	RenderTargetPos.Top() = ViewportMaxSize.Bottom() - ScreenPos.Left();
		RenderTargetPos.Top() = MaxRenderTargetSize.Bottom() - ScreenPos.Left();
	}

	//	make relative to render target
	RenderTarget.GetSize( RenderTargetSize, MaxRenderTargetSize );

	RenderTargetPos.Left() -= RenderTargetSize.Left();
	RenderTargetPos.Top() -= RenderTargetSize.Top();

	//	outside render target, fail
	if ( !RenderTargetSize.GetIsInside( RenderTargetPos ) )
		return FALSE;

	return TRUE;
}


//---------------------------------------------------------
//	Get a screen pos from a render target-relative cursor position - fails if outside render target box
//---------------------------------------------------------
Bool TLRender::TScreen::GetScreenPosFromRenderTargetPos(Type2<s32>& ScreenPos, const TRenderTarget& RenderTarget,const Type2<s32>& RenderTargetPos, Type4<s32>& RenderTargetSize)	//	Get a screen pos render target-relative cursor position- fails if outside render target box
{
	//	outside render target, fail
	if ( !RenderTargetSize.GetIsInside( RenderTargetPos ) )
		return FALSE;

	//	convert screen(viewport) pos to render target pos by rotating it inside the viewport
	Type2<s32> RotatedScreenPos = RenderTargetPos;

	Type4<s32> MaxRenderTargetSize = GetRenderTargetMaxSize();

	//	make relative to render target
	RenderTarget.GetSize( RenderTargetSize, MaxRenderTargetSize );

	RotatedScreenPos.Left() += RenderTargetSize.Left();
	RotatedScreenPos.Top() += RenderTargetSize.Top();

//	rotate screen pos to be in "screen" space
	if ( GetScreenShape() == TLRender::ScreenShape_WideLeft )
	{
		//	rotate RIGHT
		RotatedScreenPos.Top() = MaxRenderTargetSize.Right() - RenderTargetPos.Left();
		RotatedScreenPos.Left() = RenderTargetPos.Top();
	}
	else if ( GetScreenShape() == TLRender::ScreenShape_WideRight )
	{
		//	rotate LEFT
		RotatedScreenPos.Top() = RenderTargetPos.Left();
		RotatedScreenPos.Left() = MaxRenderTargetSize.Bottom() - RenderTargetPos.Top();
	}

	// Calculate the rotated render target sizes
	Type4<s32> RotatedRenderTargetSize = RenderTargetSize;

	if ( GetScreenShape() == ScreenShape_WideLeft )
	{
		//	gr: rendertarget is rotated left, so to get viewport, rotate it right again
		//	rotate right
		RotatedRenderTargetSize.x = RenderTargetSize.Top();
		RotatedRenderTargetSize.y = MaxRenderTargetSize.Width() - RenderTargetSize.Right();
		RotatedRenderTargetSize.Width() = RenderTargetSize.Height();
		RotatedRenderTargetSize.Height() = RenderTargetSize.Width();
	}
	else if ( GetScreenShape() == ScreenShape_WideRight )
	{
		//	gr: rendertarget is rotated right, so to get viewport, rotate it left again
		//	rotate left
		RotatedRenderTargetSize.x = MaxRenderTargetSize.Height() - RenderTargetSize.Bottom();
		RotatedRenderTargetSize.y = RenderTargetSize.Left();
		RotatedRenderTargetSize.Width() = RenderTargetSize.Height();
		RotatedRenderTargetSize.Height() = RenderTargetSize.Width();
	}

	// Now flip the Y based on the rotated render target to get a final pos in screen space
	ScreenPos.Left() = RotatedScreenPos.Left();
	ScreenPos.Top() = RotatedRenderTargetSize.Height() - RotatedScreenPos.Top();

	//	check the point is inside the screen viewport
	Type4<s32> ViewportMaxSize = GetViewportMaxSize();

	if ( !ViewportMaxSize.GetIsInside( ScreenPos ) )
		return FALSE;

	return TRUE;
}




//---------------------------------------------------------
//	get a world position from this screen posiiton
//---------------------------------------------------------
Bool TLRender::TScreen::GetWorldRayFromScreenPos(const TRenderTarget& RenderTarget,TLMaths::TLine& WorldRay,const Type2<s32>& ScreenPos)
{
	Type2<s32> RenderTargetPos;
	Type4<s32> RenderTargetSize;
	if ( !GetRenderTargetPosFromScreenPos( RenderTarget, RenderTargetPos, RenderTargetSize, ScreenPos ) )
		return FALSE;

	//	let render target do it's own conversions what with fancy cameras n that
	return RenderTarget.GetWorldRay( WorldRay, RenderTargetPos, RenderTargetSize, GetScreenShape() );
}


//---------------------------------------------------------
//	
//---------------------------------------------------------
Bool TLRender::TScreen::GetWorldPosFromScreenPos(const TRenderTarget& RenderTarget,float3& WorldPos,float WorldDepth,const Type2<s32>& ScreenPos)
{
	Type2<s32> RenderTargetPos;
	Type4<s32> RenderTargetSize;
	if ( !GetRenderTargetPosFromScreenPos( RenderTarget, RenderTargetPos, RenderTargetSize, ScreenPos ) )
		return FALSE;

	//	let render target do it's own conversions what with fancy cameras n that
	return RenderTarget.GetWorldPos( WorldPos, WorldDepth, RenderTargetPos, RenderTargetSize, GetScreenShape() );
}



Bool TLRender::TScreen::GetScreenPosFromWorldPos(const TRenderTarget& RenderTarget, const float3& WorldPos, Type2<s32>& ScreenPos)
{
	Type4<s32> MaxRenderTargetSize = GetRenderTargetMaxSize();
	Type4<s32> RenderTargetSize;
	RenderTarget.GetSize(RenderTargetSize, MaxRenderTargetSize);

	//	let render target do it's own conversions what with fancy cameras n that
	Type2<s32> RenderTargetPos;
	if ( !RenderTarget.GetRenderTargetPos( RenderTargetPos, WorldPos, RenderTargetSize, GetScreenShape()) )
		return FALSE;


	return GetScreenPosFromRenderTargetPos( ScreenPos, RenderTarget, RenderTargetPos, RenderTargetSize);
}




//---------------------------------------------------------
//	get the render target max size (in "render target space") - this is the viewport size, but rotated
//---------------------------------------------------------
Type4<s32> TLRender::TScreen::GetRenderTargetMaxSize()
{
	Type4<s32> MaxSize = GetViewportMaxSize();

	//	rotate render target so it's in "render target" space
	if ( GetScreenShape() == TLRender::ScreenShape_WideLeft )
	{
		Type4<s32> ViewportMaxSize = MaxSize;

		//	rotate left
	//	topleft = bottomleft
	//	topright = topleft
	//	bottomright = topright
	//	bottomleft = bottomright
		MaxSize.x = ViewportMaxSize.Height() - ViewportMaxSize.Bottom();
		MaxSize.y = ViewportMaxSize.Left();
		MaxSize.Width() = ViewportMaxSize.Height();
		MaxSize.Height() = ViewportMaxSize.Width();
	}
	else if ( GetScreenShape() == TLRender::ScreenShape_WideRight )
	{
		Type4<s32> ViewportMaxSize = MaxSize;

		//	rotate right
		MaxSize.x = ViewportMaxSize.Top();
		MaxSize.y = ViewportMaxSize.Width() - ViewportMaxSize.Right();
		MaxSize.Width() = ViewportMaxSize.Height();
		MaxSize.Height() = ViewportMaxSize.Width();
	}

	return MaxSize;
}


//---------------------------------------------------------
//	
//---------------------------------------------------------
void TLRender::TScreen::CreateDebugRenderTarget(TRefRef FontRef)
{
	if ( !FontRef.IsValid() )
		return;

	TPtr<TLRender::TRenderTarget> pRenderTarget = CreateRenderTarget("Debug");
	if ( !pRenderTarget )
		return;

	pRenderTarget->SetScreenZ(99);
	pRenderTarget->SetClearColour( TColour(0.f, 1.f, 0.f, 0.f ) );
	
	TPtr<TLRender::TCamera> pCamera = new TLRender::TOrthoCamera;
	pCamera->SetPosition( float3( 0, 0, -10.f ) );
	pRenderTarget->SetCamera( pCamera );


	//	store render target
	m_DebugRenderTarget = pRenderTarget->GetRef();

	//	init root node
	TRef RootRenderNode;
	{
		float DebugScale = GetScreenShape() == TLRender::ScreenShape_Portrait ? 5.f : 3.0f;

		TLMessaging::TMessage InitMessage(TLCore::InitialiseRef);
		InitMessage.ExportData("Scale", float3( DebugScale, DebugScale, 1.f ) );
		InitMessage.ExportData("Colour", TColour( 1.f, 1.f, 1.f, 0.8f ) );
		RootRenderNode = TLRender::g_pRendergraph->CreateNode( "root", TRef(), TRef(), &InitMessage );
		pRenderTarget->SetRootRenderNode( RootRenderNode );
	}

	//	add fps text
	{
		TLMessaging::TMessage InitMessage(TLCore::InitialiseRef);
		InitMessage.ExportData("FontRef", FontRef );
		TRef FpsRenderNode = TLRender::g_pRendergraph->CreateNode( "dbgfps", "txtext", RootRenderNode, &InitMessage );
		m_DebugRenderText.Add( "fps", FpsRenderNode );
	}

}

	
//---------------------------------------------------------
//	return text render node for this debug text
//---------------------------------------------------------
TLRender::TRenderNodeText* TLRender::TScreen::Debug_GetRenderNodeText(TRefRef DebugTextRef)
{
	TRef* ppRenderNodeRef = m_DebugRenderText.Find( DebugTextRef );
	if ( !ppRenderNodeRef )
		return NULL;

	TLRender::TRenderNode* pRenderNode = TLRender::g_pRendergraph->FindNode( *ppRenderNodeRef );
	if ( !pRenderNode )
		return NULL;

	return static_cast<TLRender::TRenderNodeText*>(pRenderNode);
}


//---------------------------------------------------------
//	z has changed on render target - resorts render targets
//---------------------------------------------------------
void TLRender::TScreen::OnRenderTargetZChanged(const TRenderTarget& RenderTarget)
{
	m_RenderTargets.Sort();
}



//----------------------------------------------------------
//	get the desktop dimensions
//----------------------------------------------------------
void TLRender::TScreen::GetDesktopSize(Type4<s32>& DesktopSize) const
{
	TLGui::Platform::GetDesktopSize( DesktopSize );
}


//----------------------------------------------------------
//	take a screen size and center it on the desktop
//----------------------------------------------------------
void TLRender::TScreen::GetCenteredSize(Type4<s32>& Size) const
{
	Type4<s32> DesktopSize;
	GetDesktopSize( DesktopSize );
	
	s32 DesktopCenterX = DesktopSize.x + (DesktopSize.Width() / 2);
	s32 DesktopCenterY = DesktopSize.y + (DesktopSize.Height() / 2);
	
	Size.x = DesktopCenterX - (Size.Width() / 2);
	Size.y = DesktopCenterY - (Size.Height() / 2);
}


