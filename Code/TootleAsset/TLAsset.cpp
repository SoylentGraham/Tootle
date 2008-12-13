#include "TLAsset.h"
#include "TMesh.h"
#include "TFont.h"
#include "TMenu.h"
#include "TAudio.h"
#include "TLoadTask.h"
#include <TootleCore/TPtr.h>
#include <TootleCore/TEventChannel.h>
#include <TootleFileSys/TFileAsset.h>

namespace TLAsset
{
	TPtr<TLAsset::TAssetFactory>	g_pFactory;
	TPtrArray<TLoadTask>			g_LoadTasks;
	TPtr<TAsset>					g_pNullAsset;

};



//----------------------------------------------------------
//	asset sort
//----------------------------------------------------------
TLArray::SortResult	TLAsset::AssetSort(const TPtr<TLAsset::TAsset>& a,const TPtr<TLAsset::TAsset>& b,const void* pTestRef)
{
	const TRef& aRef = a->GetAssetRef();
	const TRef& bRef = pTestRef ? *(const TRef*)pTestRef : b->GetAssetRef();
	
	//	== turns into 0 (is greater) or 1(equals)
	return aRef < bRef ? TLArray::IsLess : (TLArray::SortResult)(aRef==bRef);	
}


//----------------------------------------------------------
//	return a pointer to an asset
//----------------------------------------------------------
TPtr<TLAsset::TAsset>& TLAsset::CreateAsset(TRefRef AssetRef,TRefRef AssetType)
{
	if ( !g_pFactory )
	{
		TLDebug_Break("Asset factory expected");
		return g_pNullAsset;
	}

	TPtr<TAsset>& pNewAsset = g_pFactory->GetInstance( AssetRef, TRUE, AssetType );
	if ( !pNewAsset )
	{
		TTempString DebugString("Failed to create asset... ");
		AssetRef.GetString( DebugString );
		DebugString.Append(" (");
		AssetType.GetString( DebugString );
		DebugString.Append(")");
		TLDebug_Print( DebugString );
		return g_pNullAsset;
	}
	else
	{
		TTempString DebugString("Created asset: ");
		AssetRef.GetString( DebugString );
		DebugString.Append(" (");
		AssetType.GetString( DebugString );
		DebugString.Append(")");
		TLDebug_Print( DebugString );
	}

	//	ensure it's the right type
	if ( pNewAsset->GetAssetType() != AssetType )
	{
		TTempString DebugString("Created/found asset ");
		pNewAsset->GetAssetRef().GetString( DebugString );
		DebugString.Append(" but is type ");
		pNewAsset->GetAssetType().GetString( DebugString );
		DebugString.Append(". Expected type ");
		AssetType.GetString( DebugString );
		TLDebug_Print( DebugString );
		return g_pNullAsset;
	}

	return pNewAsset;
}


//----------------------------------------------------------
//	return a pointer to an asset
//----------------------------------------------------------
TPtr<TLAsset::TAsset>& TLAsset::GetAsset(TRefRef AssetRef,Bool LoadedOnly)
{
	if ( !g_pFactory )
	{
		TLDebug_Break("Asset factory expected");
		return g_pNullAsset;
	}
	
	TPtr<TLAsset::TAsset>& pAsset = g_pFactory->GetInstance( AssetRef );
	
	if ( LoadedOnly )
	{
		if ( pAsset && !pAsset->IsLoaded() )
			return g_pNullAsset;
	}

	if ( !pAsset && AssetRef.IsValid() )
	{
		TTempString DebugString("failed to find asset ");
		AssetRef.GetString( DebugString );
		TLDebug_Print( DebugString );
	}

	
	return pAsset;
}


//----------------------------------------------------------
//	delete an asset
//----------------------------------------------------------
void TLAsset::DeleteAsset(TRefRef AssetRef)
{
	//	find the existing asset
	//XXXX
	TPtr<TAsset> pAsset = GetAsset(AssetRef);

	//	unknown asset, nothing to do
	if ( !pAsset )
		return;

	//	mark asset as unavailible
	pAsset->SetLoadingState( SyncFalse );

	TTempString DebugString("Deleting asset from factory... ");
	AssetRef.GetString( DebugString );
	TLDebug_Print( DebugString );

	//	delete from factory
	if ( !g_pFactory->RemoveInstance( AssetRef ) )
	{
		TTempString DebugString("Deleting asset from factory... ");
		AssetRef.GetString( DebugString );
		DebugString.Append(" not found");
		TLDebug_Print( DebugString );
	}
}

//----------------------------------------------------------
//	load asset from a file system
//----------------------------------------------------------
TPtr<TLAsset::TAsset>& TLAsset::LoadAsset(const TRef& AssetRef, Bool bBlocking)
{
	if ( !g_pFactory )
	{
		TLDebug_Break("Asset factory expected");
		return g_pNullAsset;
	}

	//	check it doesnt already exist
	{
		TPtr<TAsset>& pAsset = GetAsset( AssetRef );
		if ( pAsset )
		{
			if ( pAsset->IsLoaded() )
				return pAsset;
		}
	}

	//	look for an existing loading task
	TPtr<TLAsset::TLoadTask> pLoadTask = TLAsset::GetLoadTask( AssetRef );

	//	create a new load task if we dont have one already
	if ( !pLoadTask )
	{
		pLoadTask = new TLoadTask( AssetRef );
	}

	//	missing task (failed to alloc?)
	if ( !pLoadTask )
		return g_pNullAsset;

	//	do first update, if it fails then we can abort early and fail immedietly
	SyncBool FirstUpdateResult = pLoadTask->Update( bBlocking );

	//	get loading/loaded asset
	TPtr<TAsset>& pLoadingAsset = pLoadTask->GetAsset();

	//	verify the correct result
	if ( FirstUpdateResult != SyncFalse )
	{
		//	asset expected
		if ( !pLoadingAsset )
		{
			if ( !TLDebug_Break("Asset expected") )
				FirstUpdateResult = SyncFalse;
		}
	}

	//	failed
	if ( FirstUpdateResult == SyncFalse )
	{
		pLoadTask = NULL;
		return g_pNullAsset;
	}

	//	finished already! dont need to add the task to the queue
	if ( FirstUpdateResult == SyncTrue )
	{
		pLoadTask = NULL;
		return pLoadingAsset;
	}

	//	if we got here with a block load, then it failed to block load...
	if ( bBlocking )
	{
		TLDebug_Break("Block load failed... asynchornously loading...");
	}

	//	load in progress, add task to list for asynchronous load
	g_LoadTasks.AddUnique( pLoadTask );
		
	return pLoadingAsset;
}
	

//----------------------------------------------------------
//	instance an asset
//----------------------------------------------------------
TLAsset::TAsset* TLAsset::TAssetFactory::CreateObject(TRefRef InstanceRef,TRefRef TypeRef)
{
	if ( TypeRef == "Audio" )
		return new TLAsset::TAudio( InstanceRef );

	if ( TypeRef == "Mesh" )
		return new TLAsset::TMesh( InstanceRef );

	if ( TypeRef == "Font" )
		return new TLAsset::TFont( InstanceRef );
	
	if ( TypeRef == "Menu" )
		return new TLAsset::TMenu( InstanceRef );
	
	if ( TypeRef == "Temp" )
		return new TLAsset::TTempAsset( InstanceRef );

	return NULL;
}


void TLAsset::TAssetFactory::OnEventChannelAdded(TRefRef refPublisherID, TRefRef refChannelID)
{
	if(refPublisherID == "CORE")
	{
		// Subscribe to the update messages
		if(refChannelID == TLCore::UpdateRef)
			TLMessaging::g_pEventChannelManager->SubscribeTo(this, refPublisherID, refChannelID); 
	}
	
	// Super event channel routine
	TManager::OnEventChannelAdded(refPublisherID, refChannelID);
}



SyncBool TLAsset::TAssetFactory::Update(float fTimeStep)
{
	//	update manager
	if ( TManager::Update( fTimeStep ) == SyncFalse )
		return SyncFalse;

	//	update load tasks, FIFO
	for ( u32 t=0;	t<g_LoadTasks.GetSize();	t++ )
	{
		TPtr<TLoadTask>& pTask = g_LoadTasks[t];
		if ( !pTask )
			continue;

		//	update task
		SyncBool UpdateResult = pTask->Update( FALSE );

		//	all complete!
		if ( UpdateResult == SyncTrue )
			pTask = NULL;
	}

	//	remove null(completed) tasks
	g_LoadTasks.RemoveNull();

	return SyncTrue;
}



//------------------------------------------------------------
//	get the load task for this asset
//------------------------------------------------------------
TPtr<TLAsset::TLoadTask> TLAsset::GetLoadTask(TRefRef AssetRef)
{
	//	
	return g_LoadTasks.FindPtr( AssetRef );
}

