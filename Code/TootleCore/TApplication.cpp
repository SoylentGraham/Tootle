/*
 *  TApplication.cpp
 *  TootleCore
 *
 *  Created by Duane Bradbury on 06/02/2009.
 *  Copyright 2009 Tootle. All rights reserved.
 *
 */

#include "TApplication.h"

#include <TootleCore/TCoreManager.h>
#include <TootleRender/TScreenManager.h>
#include <TootleFileSys/TLFileSys.h>


#include <TootleInput/TLInput.h>


// Time for the bootup sequence to occur over
#ifdef TL_TARGET_PC
	#define BOOTUP_TIME_MIN 2.0f
#else
	#define BOOTUP_TIME_MIN 0.2f	//	gr: reduced so just gets out of splash screen asap
#endif


using namespace TLCore;


//-----------------------------------------------------------
//	Application initialisation
//-----------------------------------------------------------
SyncBool TApplication::Initialise()
{
	Bool Waiting = FALSE;

	//	create file systems
	if ( !m_LocalFileSysRef.IsValid() )
	{
		#if defined(TL_TARGET_PC)
			TTempString AssetDir = "Assets\\";
		#elif defined(TL_TARGET_MAC)
			TTempString AssetDir = "Assets\\";
		#elif defined(TL_TARGET_IPOD)
			TTempString AppName = GetName();
			TTempString AssetDir = AppName;	
			AssetDir.Append(".app/");
		#endif

		TLFileSys::CreateLocalFileSys( m_LocalFileSysRef, AssetDir, FALSE );
	}

	//	continue init of file system
	if ( m_LocalFileSysRef.IsValid() )
	{
		SyncBool Result = SyncFalse;

		TPtr<TLFileSys::TFileSys>& pFileSys = TLFileSys::GetFileSys( m_LocalFileSysRef );
		if ( pFileSys )
			Result = pFileSys->Init();

		if ( Result == SyncFalse )
		{
			//	gr: fail if asset dir cannot be opened
			TLDebug_Break("failed to create local file sys");
			m_LocalFileSysRef.SetInvalid();
			return SyncFalse;
		}
			
		Waiting |= (Result == SyncWait);
	}



	//	create docs file sys
	if ( !m_UserFileSysRef.IsValid() )
	{
		#if defined(TL_TARGET_PC)
			TTempString AssetDir = "Assets\\User\\";
		#elif defined(TL_TARGET_MAC)
			TTempString AssetDir = "Assets\\User\\";
		#elif defined(TL_TARGET_IPOD)
			TTempString AssetDir = "Documents/";	//	gr: I'm pretty sure the documents dir is above the app dir
		#endif

		TLFileSys::CreateLocalFileSys( m_UserFileSysRef, AssetDir, TRUE );
	}	
	
	//	continue init of file system
	if ( m_UserFileSysRef.IsValid() )
	{
		SyncBool Result = SyncFalse;

		TPtr<TLFileSys::TFileSys>& pFileSys = TLFileSys::GetFileSys( m_UserFileSysRef );
		if ( pFileSys )
			Result = pFileSys->Init();

		if ( Result == SyncFalse )
		{
			//	gr: let system continue if this dir cannot be used
			TLDebug_Print("failed to create local user file sys");
			m_UserFileSysRef.SetInvalid();
		}
			
		Waiting |= (Result == SyncWait);
	}


	//	subscribe to screen manager to get screen-deleted messages
	SubscribeTo( TLRender::g_pScreenManager );
	
	//	subscribe core to app so we can send quit message
	TLCore::g_pCoreManager->SubscribeTo( this );
	
	//	create screen
	//	gr: todo: turn this into some factory? or an alias for platform screen?
	TPtr<TLRender::TScreen>& pScreen = TLRender::g_pScreenManager->GetInstance(TRef("Screen"), TRUE, GetDefaultScreenType() );
	if(!pScreen)
	{
		TLDebug_Print("Failed to create main screen object");
		return SyncFalse;
	}
	
	// Initialise the screen object
	SyncBool Result = pScreen->Init();
	if ( Result != SyncTrue )
	{
		TLDebug_Print("Error: Failed to initialise screen");
		return Result;
	}
	
	// Add the application modes
	AddModes();

	return TManager::Initialise();
}


void TApplication::AddModes()
{
	AddMode<TApplicationState_Bootup>("Bootup");
	AddMode<TApplicationState_FrontEnd>("FrontEnd");
	AddMode<TApplicationState_EnterGame>("EnterGame");
	AddMode<TApplicationState_Game>("Game");
	AddMode<TApplicationState_Pause>("Pause");
	AddMode<TApplicationState_ExitGame>("ExitGame");	
}



//-----------------------------------------------------------
//	Application update
//-----------------------------------------------------------
SyncBool TApplication::Update(float fTimeStep)
{
	//	request to change app mode
	if ( m_NewAppMode.IsValid() )
	{
		TStateMachine::SetMode( m_NewAppMode );
		m_NewAppMode.SetInvalid();
	}

	// Udpate the state machine
	TStateMachine::Update(fTimeStep);
		
	return TManager::Update(fTimeStep);
}

//-----------------------------------------------------------
//	Application shutdown
//-----------------------------------------------------------
SyncBool TApplication::Shutdown()
{
	//	clean up states
	TStateMachine::Shutdown();

	//	clean up game if it hasnt been done
	DestroyGame();

	return TManager::Shutdown();
}


//-----------------------------------------------------------
//	process messages
//-----------------------------------------------------------
void TApplication::ProcessMessage(TLMessaging::TMessage& Message)
{
	if ( Message.GetMessageRef() == "ScreenChanged" ) 
	{
		TRef State;

		if(Message.ImportData("State", State))
		{
			//	screen was deleted
			if ( State == "Deleted" )
			{
				//	if there are no screens left, close app
				if ( TLRender::g_pScreenManager->GetSize() == 0 )
				{
					TLMessaging::TMessage Message(TLCore::QuitRef);
					PublishMessage( Message );
				}
			}
		}
	}

	TManager::ProcessMessage(Message);
}


void TApplication::OnEventChannelAdded(TRefRef refPublisherID, TRefRef refChannelID)
{

	TManager::OnEventChannelAdded(refPublisherID, refChannelID);
}



TPtr<TLGame::TGame> TApplication::CreateGameObject()
{
	TLDebug_Break("Overload this and create your game");
	return NULL;
}


//-----------------------------------------
//	create and setup the game object on the app
//-----------------------------------------
Bool TApplication::CreateGame()
{
	//	existing game shouldn't really exist
	if ( m_pGame )
	{
		TLDebug_Break("Creating new game whilst other already exists...");
		DestroyGame();
	}

	//	create new instance
	m_pGame = CreateGameObject();
	if ( !m_pGame )
		return FALSE;
	
	//	register as manager so things can find it by using the manager lookup functionality...
	//	overkill for a game?
	TLCore::g_pCoreManager->RegisterManager( m_pGame );

	// Subscribe the game object to the application
	m_pGame->SubscribeTo(this);
	
	// Subscribe to the core manager for update messages
	m_pGame->SubscribeTo(TLCore::g_pCoreManager);

	//	gr: just initialise, we dont wanna send this message to everyone
	if ( m_pGame->Initialise() != SyncTrue )
	{
		DestroyGame();
		return FALSE;
	}

	return TRUE;
}


//-----------------------------------------
//	
//-----------------------------------------
void TApplication::DestroyGame()
{
	if ( !m_pGame )
		return;

	//	shutdown
	m_pGame->Shutdown();

	//	unregister
	TLCore::g_pCoreManager->UnregisterManager( m_pGame->GetManagerRef() );

	//	free			
	m_pGame = NULL;
}


//--------------------------------------------------
//	notify subscribers when option changes - and do any specific option stuff
//--------------------------------------------------
void TApplication::OnOptionChanged(TRefRef OptionRef)
{
	TLMessaging::TMessage Message("OptChanged");
	Message.Write( OptionRef );
	
	PublishMessage( Message );
}









// Application States


// Bootup state

Bool TApplication::TApplicationState_Bootup::OnBegin(TRefRef PreviousMode)
{
	TApplication* pApp = GetStateMachine<TApplication>();
	// TODO:

	//	gr: if this fails (e.g. no logo asset) skip onto next mode
	Bool bSuccess = CreateIntroScreen();
	
	if(!bSuccess)
		return FALSE;
	
	// TODO:
	// Setup language
	// Setup Audio - language specific
	// Load Text - language specific
	// Setup global settings

	// Obtain list of files to load at startup
	pApp->GetPreloadFiles(m_PreloadFiles);

	if(m_PreloadFiles.GetSize() > 0)
		PreloadFiles();
	
	return TStateMode::OnBegin(PreviousMode);
}


TApplication::TApplicationState_Bootup::TApplicationState_Bootup() :
	m_pTimelineInstance	( NULL ),
	m_SkipBootup		( FALSE )
{
}

Bool TApplication::TApplicationState_Bootup::CreateIntroScreen()
{
	m_SkipBootup = FALSE;

	TPtr<TLRender::TScreen>& pScreen = TLRender::g_pScreenManager->GetDefaultScreen();	
	if(!pScreen)
	{
		TLDebug_Break("Error: Failed to get screen");
		return FALSE;
	}
		
	//	create background graphic
	TLAsset::TMesh* pBgAsset = TLAsset::GetAsset<TLAsset::TMesh>("logo");
	if ( pBgAsset )
	{
		TLMessaging::TMessage InitMessage(TLCore::InitialiseRef);
		InitMessage.ExportData("MeshRef", pBgAsset->GetAssetRef() );
		InitMessage.ExportData(TRef_Static(T,r,a,n,s), float3( 0.f, 0.f, -50.f ) );
		InitMessage.ExportData("LineWidth", 3.f );

		m_LogoRenderNode = TLRender::g_pRendergraph->CreateNode("logo", TRef(), TRef(), &InitMessage );
	}
	else
	{
		//TLDebug_Break("Error: Failed to load logo asset");
		m_SkipBootup = TRUE;

		//	gr: this will just go into a "no mode" mode.
		//return FALSE;
	}

	//	create a render target if we created a render node
	if ( m_LogoRenderNode.IsValid() )
	{
		TPtr<TLRender::TRenderTarget> pRenderTarget = pScreen->CreateRenderTarget( TRef("Intro") );
		if(!pRenderTarget)
		{
			TLDebug_Break("Error: Failed to create logo render target");
			return FALSE;
		}
	
		m_RenderTarget = pRenderTarget->GetRef();
		pRenderTarget->SetClearColour( TColour( 1.f, 1.f, 1.f, 1.f ) );
	
		TPtr<TLRender::TCamera> pCamera = new TLRender::TOrthoCamera;
		pRenderTarget->SetCamera( pCamera );
		pCamera->SetPosition( float3( 0, 0, -10.f ) );
		pRenderTarget->SetRootRenderNode( m_LogoRenderNode );
	}

	// Create timeline
	TPtr<TLAsset::TTimeline> pIntroTimeline = TLAsset::GetAssetPtr<TLAsset::TTimeline>("t_logo");
	if ( pIntroTimeline )
	{
		// Create the timeline instance
		m_pTimelineInstance = new TLAnimation::TTimelineInstance( pIntroTimeline );

		// Bind the timeline instance to the render node and init
		if(m_pTimelineInstance)
		{
			TLMessaging::TMessage Message(TLCore::InitialiseRef);
			Message.ExportData("Time", 0.0f);
			m_pTimelineInstance->Initialise(Message);
		}
	}
	
	// All done
	return TRUE;
}


TRef TApplication::TApplicationState_Bootup::Update(float Timestep)
{
	SyncBool TimelineUpdate = SyncFalse;
	if(m_pTimelineInstance)
		TimelineUpdate = m_pTimelineInstance->Update(Timestep);

	if ( m_SkipBootup || (GetModeTime() > BOOTUP_TIME_MIN) && PreloadFiles() && (TimelineUpdate == SyncFalse) )
	{
		TApplication* pApp = GetStateMachine<TApplication>();
		
		// If we have a front end mode then go to the front end mode, otherwise drop into the enter game mode
		if(pApp->HasMode("FrontEnd"))
			return "FrontEnd";
		else
			return "EnterGame";
	}

	// Wait for preload files to be loaded and the min time
	return TRef();
};


//--------------------------------------------------------
//	load files. returns TRUE when finished (no more to load)
//--------------------------------------------------------
Bool TApplication::TApplicationState_Bootup::PreloadFiles()
{
	for ( s32 i=m_PreloadFiles.GetLastIndex();	i>=0;	i-- )
	{
		SyncBool AssetLoadState = TLAsset::LoadAsset( m_PreloadFiles[i], FALSE );
		
		//	still loading
		if ( AssetLoadState == SyncWait )
			continue;

		//	failed show error
		if ( AssetLoadState == SyncFalse )
		{
			TTempString DebugString("Preload: Asset failed to load ");
			m_PreloadFiles[i].GetString( DebugString );
			TLDebug_Print( DebugString );

			//	remove from preload list
			m_PreloadFiles.RemoveAt( i );
			continue;
		}

		//	loaded, remove from preload list
		if ( AssetLoadState == SyncTrue )
		{
			//	remove from preload list
			m_PreloadFiles.RemoveAt( i );
			continue;
		}

		TLDebug_Break("Unknown state here");
	}

	return (m_PreloadFiles.GetSize() == 0);
}


void TApplication::TApplicationState_Bootup::OnEnd(TRefRef NextMode)
{
	//	delete node
	TLRender::g_pRendergraph->RemoveNode( m_LogoRenderNode );

	//	delete render target
	if ( m_RenderTarget.IsValid() )
	{
		TLRender::g_pScreenManager->DeleteRenderTarget( m_RenderTarget );
		m_RenderTarget.SetInvalid();
	}

	m_pTimelineInstance = NULL;


	//	delete assets we used
	TLAsset::DeleteAsset("logo","Mesh");
	TLAsset::DeleteAsset("t_logo","Timeline");
	
	TLTime::TTimestampMicro BootTime(TRUE);	
	TLCore::g_pCoreManager->StoreTimestamp("TSBootupTime", BootTime);
	
	TLTime::TTimestampMicro StartTime;
	if(TLCore::g_pCoreManager->RetrieveTimestamp("TSStartTime", StartTime))
	{	
		// Calculate time it took to go through the entire bootup sequence
		s32 Secs, MilliSecs, MicroSecs;
		StartTime.GetTimeDiff(BootTime, Secs, MilliSecs, MicroSecs);
	
		TTempString time;
		time.Appendf("%d.%d:%d Seconds", Secs, MilliSecs, MicroSecs);
		TLDebug_Print("App finished boot sequence");
		TLDebug_Print(time.GetData());		
	}
}



// Front End state
Bool TApplication::TApplicationState_FrontEnd::OnBegin(TRefRef PreviousMode)
{
	// TODO:
	// Load front end scheme
	
	return TStateMode::OnBegin(PreviousMode);
}

TRef TApplication::TApplicationState_FrontEnd::Update(float Timestep)
{
	// Essentially wait until the app is signalled for what mode to change into
	return TRef();
};



// Enter Game transitional state
Bool TApplication::TApplicationState_EnterGame::OnBegin(TRefRef PreviousMode)
{
	// TODO:
	// Begin transition

	// Create (gamespecific) TGame object
	TApplication* pApp = GetStateMachine<TApplication>();
	if ( !pApp->CreateGame() )
		return FALSE;

	return TStateMode::OnBegin(PreviousMode);
}

TRef TApplication::TApplicationState_EnterGame::Update(float Timestep)
{	
	// Wait for game files to laod
	return "Game";
};



// Game active state
TRef TApplication::TApplicationState_Game::Update(float Timestep)
{
	return TRef();
};



// Game paused state - may be moved to a state on the TGame object instead
TRef TApplication::TApplicationState_Pause::Update(float Timestep)
{
	// Pause game
	return TRef();
};



// Exit Game transitional state
Bool TApplication::TApplicationState_ExitGame::OnBegin(TRefRef PreviousMode)
{
	// TODO:
	// Begin transition
	// Save states of stuff
	
	return TStateMode::OnBegin(PreviousMode);
}

TRef TApplication::TApplicationState_ExitGame::Update(float Timestep)
{
	// Wait for transition to be ready
	// return TRef();
	
	// Request unload game schemes
		
	return "FrontEnd";
};

void TApplication::TApplicationState_ExitGame::OnEnd(TRefRef NextMode)
{
	// Destroy (game specific) TGame object
	TApplication* pApp = GetStateMachine<TApplication>();
	pApp->DestroyGame();	
	
	return TStateMode::OnEnd(NextMode);
}




