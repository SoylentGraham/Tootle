#include "MacRenderTarget.h"
#include "MacRender.h"
#include "MacOpenglExt.h"
#include "../TRenderNode.h"
#include "../TScreen.h"
#include <TootleAsset/TLAsset.h>
#include <TootleAsset/TMesh.h>
#include <TootleAsset/TShader.h>



//#define FORCE_RENDERNODE_CLEAR		//	even if our clear colour is opaque, clear with a render node regardless


TLRender::Platform::RenderTarget::RenderTarget(const TRef& Ref) :
	TRenderTarget	( Ref )
{
}


//-------------------------------------------------------
//
//-------------------------------------------------------
Bool TLRender::Platform::RenderTarget::BeginDraw(const Type4<s32>& MaxSize,const TScreen& Screen)			
{
	//	do base checks
	if ( !TRenderTarget::BeginDraw(MaxSize, Screen) )
		return FALSE;
	
	Type4<s32> ViewportSize;
	if ( !GetViewportSize( ViewportSize, MaxSize ) )
		return FALSE;

	//	setup viewport and sissor outside the viewport
	glViewport( ViewportSize.Left(), ViewportSize.Top(), ViewportSize.Width(), ViewportSize.Height() );
	glScissor( ViewportSize.Left(), ViewportSize.Top(), ViewportSize.Width(), ViewportSize.Height() );
	Opengl::Debug_CheckForError();		

	//	calculate new view sizes etc for this viewport
	TPtr<TLRender::TCamera>& pCamera = GetCamera();
	pCamera->SetViewport( ViewportSize, Screen.GetScreenShape() );

	//	do projection vs orthographic setup
	if ( GetCamera()->IsOrtho() )
	{
		if ( !BeginOrthoDraw( pCamera.GetObject<TLRender::TOrthoCamera>(), Screen.GetScreenShape() ) )
			return FALSE;
	}
	else
	{
		if ( !BeginProjectDraw( pCamera.GetObject<TLRender::TProjectCamera>(), Screen.GetScreenShape() ) )
			return FALSE;
	}

	//	enable/disable antialiasing
	if ( GetFlag( Flag_AntiAlias ) )
		glEnable( GL_MULTISAMPLE_ARB );
	else
		glDisable( GL_MULTISAMPLE_ARB );

	//	clear render target (viewport has been set)
	GLbitfield ClearMask = 0x0;
	if ( GetFlag( Flag_ClearColour ) )
	{
#ifndef FORCE_RENDERNODE_CLEAR
		//	if the clear colour has an alpha, we dont use the opengl clear as it doesnt support alpha
		if ( !m_ClearColour.IsTransparent() )
			ClearMask |= GL_COLOR_BUFFER_BIT;
#endif
	}

	if ( GetFlag( Flag_ClearDepth ) )	
	{
		ClearMask |= GL_DEPTH_BUFFER_BIT;
	}

	if ( GetFlag( Flag_ClearStencil ) )	
	{
		ClearMask |= GL_STENCIL_BUFFER_BIT;
	}

	//	set the clear colour
	if ( ( ClearMask & GL_COLOR_BUFFER_BIT ) != 0x0 )
		glClearColor( m_ClearColour.GetRed(),  m_ClearColour.GetGreen(),  m_ClearColour.GetBlue(),  m_ClearColour.GetAlpha() );

	//	set the clear depth
	float ClearDepth = GetCamera()->GetFarZ();
	TLMaths::Limit( ClearDepth, 0.f, 1.f );
	glClearDepth( ClearDepth );
	glDepthFunc( GL_LEQUAL );

	//	clear
	glClear( ClearMask );

	Opengl::Debug_CheckForError();		

	return TRUE;	
}



//-------------------------------------------------------
//	setup projection mode
//-------------------------------------------------------
Bool TLRender::Platform::RenderTarget::BeginProjectDraw(TLRender::TProjectCamera* pCamera,TLRender::TScreenShape ScreenShape)
{
	//	get the camera
	//	init projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//	get view box
	const TLMaths::TBox2D& ScreenViewBox = pCamera->GetScreenViewBox();
	
	//	set projection matrix - 
	//	gr: note, Bottom and Top are the WRONG way around to invert opengl's upside coordinate system and makes things simpiler in our own code
	glFrustum( ScreenViewBox.GetLeft(), ScreenViewBox.GetRight(), ScreenViewBox.GetTop(), ScreenViewBox.GetBottom(), pCamera->GetNearZ(), pCamera->GetFarZ() );

	//	rotate the view matrix so that UP is properly relative to the new screen
	//	gr: another "thing what is backwards" - as is the -/+ of the shape rotation....
	float ProjectionRotationDeg = -pCamera->GetCameraRoll().GetDegrees();

	if ( ScreenShape == TLRender::ScreenShape_WideRight )
		ProjectionRotationDeg -= 90.f;
	else if ( ScreenShape == TLRender::ScreenShape_WideLeft )
		ProjectionRotationDeg += 90.f;

	//	roll around z
	Opengl::SceneRotate( TLMaths::TAngle(ProjectionRotationDeg), float3( 0.f, 0.f, 1.f ) );

	//	update projection matrix
	//	gr: todo; calc the projection matrix in the camera (like we used to) then we don't need to
	//		re-calculate it again for the frustum code, nor would we need this get-flaotv code
	TLMaths::TMatrix& ProjectionMatrix = pCamera->GetProjectionMatrix(TRUE);
	glGetFloatv( GL_PROJECTION_MATRIX, ProjectionMatrix );

	//	setup camera
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//	setup camera
	m_CameraTransform.SetInvalid();
	m_CameraTransform.SetTranslate( pCamera->GetPosition() * -1.f );

	//	apply look at matrix (rotate)
	const TLMaths::TMatrix& LookAtMatrix = pCamera->GetCameraLookAtMatrix();
	m_CameraTransform.SetMatrix( LookAtMatrix );

	Opengl::SceneTransform( m_CameraTransform );

	//	update the modelview matrix on the camera
	TLMaths::TMatrix& ModelViewMatrix = pCamera->GetModelViewMatrix(TRUE);
	glGetFloatv( GL_MODELVIEW_MATRIX, ModelViewMatrix );

	//	gr: redundant now, but using temporarily for testing
	BeginSceneReset();

	return TRUE;
}


//-------------------------------------------------------
//	clean up projection scene
//-------------------------------------------------------
void TLRender::Platform::RenderTarget::EndProjectDraw()
{
	EndScene();
}


//-------------------------------------------------------
//	setup projection mode
//-------------------------------------------------------
Bool TLRender::Platform::RenderTarget::BeginOrthoDraw(TLRender::TOrthoCamera* pCamera,TLRender::TScreenShape ScreenShape)
{
	//	setup ortho projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//	get ortho dimensions box
	const TLMaths::TBox2D& OrthoBox = pCamera->GetOrthoBox();
	
	//	rotate the view matrix so that UP is properly relative to the new screen
	//	gr: another "thing what is backwards" - as is the -/+ of the shape rotation....
	float ProjectionRotationDeg = -pCamera->GetCameraRoll().GetDegrees();

	if ( ScreenShape == TLRender::ScreenShape_WideRight )
		ProjectionRotationDeg -= 90.f;
	else if ( ScreenShape == TLRender::ScreenShape_WideLeft )
		ProjectionRotationDeg += 90.f;

	//	roll around z
	Opengl::SceneRotate( TLMaths::TAngle(ProjectionRotationDeg), float3( 0.f, 0.f, 1.f ) );

	//	set the world coordinates
	glOrtho( OrthoBox.GetLeft(), OrthoBox.GetRight(), OrthoBox.GetBottom(), OrthoBox.GetTop(), GetCamera()->GetNearZ(), GetCamera()->GetFarZ() );

	Opengl::Debug_CheckForError();		

	//	update projection matrix
	TLMaths::TMatrix& ProjectionMatrix = pCamera->GetProjectionMatrix(TRUE);
	glGetFloatv( GL_PROJECTION_MATRIX, ProjectionMatrix );
	
	Bool UseClearRenderNode = (m_ClearColour.GetAlpha() > 0.f && m_ClearColour.IsTransparent());
	
#ifdef FORCE_RENDERNODE_CLEAR
	UseClearRenderNode = TRUE;
#endif
	/*
	//	clear the screen manually if we need to apply alpha
	if ( UseClearRenderNode )
	{
		if ( !m_pRenderNodeClear )
		{
			m_pRenderNodeClear = new TRenderNodeClear("Clear","Clear");
		}
		m_pRenderNodeClear->SetSize( OrthoSize, -1.f, m_ClearColour );
	}
	else
	{
		//	remove the clear object
		m_pRenderNodeClear = NULL;
	}
	*/

	//	setup camera
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//	translate
	m_CameraTransform.SetInvalid();

	m_CameraTransform.SetTranslate( pCamera->GetPosition() );

	//	apply look at matrix (rotate)
//	const TLMaths::TMatrix& LookAtMatrix = pCamera->GetCameraLookAtMatrix();
//	m_CameraTransform.SetMatrix( LookAtMatrix );
	Opengl::SceneTransform( m_CameraTransform );

	//	update the modelview matrix on the camera
	TLMaths::TMatrix& ModelViewMatrix = pCamera->GetModelViewMatrix(TRUE);
	glGetFloatv( GL_MODELVIEW_MATRIX, ModelViewMatrix );

	//	gr: redundant now, but using temporarily for testing
	BeginSceneReset();

	return TRUE;
}


//-------------------------------------------------------
//	clean up ortho scene
//-------------------------------------------------------
void TLRender::Platform::RenderTarget::EndOrthoDraw()
{
	EndScene();
}



//-----------------------------------------------------------
//	save off current scene 
//	gr: no more push attribs, it's not supported on opengl ES so handle it ourselves
//-----------------------------------------------------------
void TLRender::Platform::RenderTarget::BeginScene()
{
	//	save the scene
	glPushMatrix();
	Opengl::Debug_CheckForError();		

	//	keep track of how many scenes we've started to keep the pushing and popping in sync
	m_Debug_SceneCount++;

}


//-----------------------------------------------------------
//	save off current scene and reset camera 
//-----------------------------------------------------------
void TLRender::Platform::RenderTarget::BeginSceneReset(Bool ApplyCamera)
{
	BeginScene();
	
	//	reset opengl scene
	glLoadIdentity();
	
	//	and reset to camera pos
	if ( ApplyCamera )
	{
		Opengl::SceneTransform( m_CameraTransform );
	}
}


//-----------------------------------------------------------
//	restore previous scene
//-----------------------------------------------------------
void TLRender::Platform::RenderTarget::EndScene()
{
	if ( m_Debug_SceneCount <= 0 )
	{
		if ( TLDebug_Break("Ending more scenes than we've begun") )
			return;
		
		//	we have elected to carry on so reset the scenecount to 1 and it'll be set to 0 again
		m_Debug_SceneCount = 1;
	}
	
	if ( m_Debug_SceneCount > 0 )
	{
		glPopMatrix();
		Opengl::Debug_CheckForError();		
		
		m_Debug_SceneCount--;
	}
}
