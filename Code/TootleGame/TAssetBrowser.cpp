#include "TAssetBrowser.h"

#include <TootleRender/TScreen.h>
#include <TootleRender/TScreenManager.h>
#include <TootleRender/TCamera.h>
#include <TootleAsset/TLAsset.h>
#include <TootleInput/TLInput.h>
#include <TootleCore/TCoreManager.h>

//----------------------------------------------
//	this is the menu renderer AND menu controller in one class
//----------------------------------------------
TLGame::TAssetBrowser::TAssetBrowser(TRefRef ScreenRef,TRefRef FontRef) :
	m_FontRef			( FontRef ),
	m_ScreenRef			( ScreenRef )
{
}


//----------------------------------------------
//	async initialise routine
//----------------------------------------------
SyncBool TLGame::TAssetBrowser::Initialise()
{	
	//	load font
	if ( TLAsset::LoadAsset( m_FontRef, "Font", TRUE ) == SyncFalse )
		return SyncFalse;
		
	//	screen ref is invalid? - resort to a valid ref but we shouldnt have an invalid one
	if ( !m_ScreenRef.IsValid() )
	{
		if ( !TLDebug_Break("Screen for asset browser is invalid") )
			return SyncFalse;

		//	force a valid ref
		m_ScreenRef.Increment();
	}

	//	get/create screen
	TPtr<TLRender::TScreen> pScreen = TLRender::g_pScreenManager->GetInstance( m_ScreenRef, TRUE );
	if ( !pScreen )
		return SyncFalse;
	if ( pScreen->Init() != SyncTrue )
		return SyncFalse;
	m_ScreenRef = pScreen->GetRef();

	//	create render target & camera
	TPtr<TLRender::TRenderTarget> pRenderTarget = pScreen->CreateRenderTarget("AssetBrowser");
	if ( !pRenderTarget )
		return SyncFalse;
	m_RenderTargetRef = pRenderTarget->GetRef();

	//	setup render target
	pRenderTarget->SetClearColour( TColour( 0.0f, 0.0f, 0.0f, 0.7f ) );
	
	//	create camera
	TPtr<TLRender::TCamera> pCamera = new TLRender::TOrthoCamera;
	pRenderTarget->SetCamera( pCamera );
	pCamera->SetPosition( float3( 0.f, 0.f, -10.f ) );
	pCamera->SetLookAt( float3( 0.f, 0.f, 0.f ) );

	//	create root render node to render from
	m_pMenuRenderNode = new TLRender::TRenderNode("AssetRoot");
	if ( !m_pMenuRenderNode )
		return SyncFalse;
	pRenderTarget->SetRootRenderNode( *m_pMenuRenderNode );

	//	init menu
	if ( !OpenMenu("Root") )
		return SyncFalse;

	//	subscribe to input system
	this->SubscribeTo( TLInput::g_pInputSystem );
	this->SubscribeTo( TLCore::g_pCoreManager );

	//	everything setup, menu open
	return SyncTrue;
}



//----------------------------------------------
//	cleanup all the stuff we created
//----------------------------------------------
void TLGame::TAssetBrowser::Shutdown()
{
	//	delete render target
	TPtr<TLRender::TScreen> pScreen = TLRender::g_pScreenManager->GetInstance( m_ScreenRef );
	if ( pScreen && m_RenderTargetRef.IsValid() )
	{
		pScreen->DeleteRenderTarget( m_RenderTargetRef );
		m_RenderTargetRef.SetInvalid();
	}
	
	//	delete our root render node
	TLDebug_Break("todo");
}


//----------------------------------------------
//	find our render target
//----------------------------------------------
TPtr<TLRender::TRenderTarget> TLGame::TAssetBrowser::GetRenderTarget()
{
	TPtr<TLRender::TScreen> pScreen = TLRender::g_pScreenManager->GetInstance( m_ScreenRef );
	if ( !pScreen )
		return NULL;

	return pScreen->GetRenderTarget( m_RenderTargetRef );
}


//----------------------------------------------
//	
//----------------------------------------------
TPtr<TLAsset::TMenu> TLGame::TAssetBrowser::CreateMenu(TRefRef MenuRef)
{
	//	"root" menu is the list of assets
	if ( MenuRef == "Root" )
	{
		TPtr<TLAsset::TMenu> pNewMenuAsset = new TLAsset::TMenu( MenuRef );
		TPtr<TLMenu::TMenuItem> pItem;
		
		//	add a "refresh" menu command
		pItem = pNewMenuAsset->AddMenuItem("Refresh");
		pItem->SetString("Refresh");
		pItem->SetCommand("Refresh");
/*
		//	add a menu item for all the assets we have
		for ( u32 i=0;	i<TLAsset::g_pManager->GetSize();	i++ )
		{
			TPtr<TLAsset::TAsset>& pAsset = TLAsset::g_pManager->ElementAt(i);
			TRefRef AssetRef = pAsset->GetAssetRef();
			pItem = pNewMenuAsset->AddMenuItem( AssetRef );
			TTempString AssetName;
			AssetRef.GetString( AssetName );
			pItem->SetText( AssetName );
			pItem->SetNextMenu( AssetRef );
		}
*/
		return pNewMenuAsset;
	}
	
	TLDebug_Break("gr: need to rework how this works, need to duplicate existing menu asset i think");
	/*
	//	assume any other menu ref is for an asset
	TLAsset::TMenu* pAsset = TLAsset::GetAsset( MenuRef );
	if ( pAsset )
	{
		//	create asset-specific menu
		TPtr<TLAsset::TMenu> pNewMenuAsset = new TLAsset::TMenu("Asset");
		TPtr<TLMenu::TMenu> pNewMenu = new TLMenu::TMenu( pNewMenuAsset );
	
		//	set the asset ref in the menu
		TPtr<TBinaryTree>& pMenuData = pNewMenuAsset->GetData("Asset",TRUE);
		pMenuData->Write( MenuRef );

		TPtr<TLMenu::TMenuItem> pItem;

		//	todo: add reload/unload/clone menu options to manipulate this asset

		//	add back menu option
		pItem = pNewMenuAsset->AddMenuItem("Back");
		pItem->SetText("Back");
		pItem->SetMenuCommand("Close");

		return pNewMenu;
	}
	*/

	return TMenuController::CreateMenu( MenuRef );
}

//----------------------------------------------
//	execute custom menu command
//----------------------------------------------
Bool TLGame::TAssetBrowser::ExecuteCommand(TRefRef MenuCommand)
{
	return TMenuController::ExecuteCommand( MenuCommand );
}


//----------------------------------------------
//	handle messages to and from the menu system, from input system etc
//----------------------------------------------
void TLGame::TAssetBrowser::ProcessMessage(TLMessaging::TMessage& Message)
{
	TRefRef MessageRef = Message.GetMessageRef();

	//	update animations
	if ( MessageRef == "Update" )
	{
		UpdateAssetMenuAnim();
		return;
	}

	TPtr<TLMenu::TMenu>& pCurrentMenu = GetCurrentMenu();

	//	menu message
	if ( MessageRef == "Close" && pCurrentMenu )
	{
		if ( pCurrentMenu->GetMenuRef() == "Asset" )
			CloseAssetRenderMenu();
		else if ( pCurrentMenu->GetMenuRef() == "Root" )
			CloseRootRenderMenu();
		return;
	}
	
	//	menu message
	if ( MessageRef == "Open" && pCurrentMenu )
	{
		if ( pCurrentMenu->GetMenuRef() == "Asset" )
			CreateAssetRenderMenu();
		else if ( pCurrentMenu->GetMenuRef() == "Root" )
			CreateRootRenderMenu();
		return;
	}
	
	//	do default message handling
	TLMenu::TMenuController::ProcessMessage( Message );
}


//----------------------------------------------
//----------------------------------------------
void TLGame::TAssetBrowser::CreateAssetRenderMenu()
{
}

//----------------------------------------------
//----------------------------------------------
void TLGame::TAssetBrowser::CloseAssetRenderMenu()
{
}

//----------------------------------------------
//----------------------------------------------
void TLGame::TAssetBrowser::CreateRootRenderMenu()
{
}

//----------------------------------------------
//----------------------------------------------
void TLGame::TAssetBrowser::CloseRootRenderMenu()
{
}

//----------------------------------------------
//----------------------------------------------
void TLGame::TAssetBrowser::UpdateAssetMenuAnim()
{
}

