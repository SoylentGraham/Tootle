#include <TootleCore/TLMaths.h>	//	gr: I don't know why but this needs to be included before "TFileFreetype.h" or "rand" isn't found
#include <TootleCore/TEventChannel.h>

#include "TFileSys.h"
#include "TLFileSys.h"


namespace TLFileSys
{
	extern TPtr<TLFileSys::TFileFactory>	g_pFileFactory;
}




//----------------------------------------------------------
//	constructor
//----------------------------------------------------------
TLFileSys::TFileSys::TFileSys(TRefRef FileSysRef,TRefRef FileSysTypeRef) :
	m_FileSysRef		( FileSysRef ),
	m_FileSysTypeRef	( FileSysTypeRef ),
	m_UpdatingFileList	( false )
{
}


//----------------------------------------------------------
//	check this file belongs to this file system, if not break
//----------------------------------------------------------
Bool TLFileSys::TFileSys::CheckIsFileFromThisFileSys(TPtr<TFile>& pFile)
{
	if ( !pFile )
	{
		TLDebug_Break("File expected");
		return FALSE;
	}

	//	check matching file sys
	if ( pFile->GetFileSysRef() != GetFileSysRef() )
	{
		TLDebug_Break("Expected file to be in this file sys");
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------
//	create new file into the file list - returns existing file if it already exists in our file sys
//----------------------------------------------------------
TPtr<TLFileSys::TFile> TLFileSys::TFileSys::CreateFileInstance(const TString& Filename)
{
	//	see if this file already exists
	//	gr: note: NOT a reference! don't want to overwrite the NULL TPtr
	TPtr<TFile> pFile = GetFile( Filename );

	//	already created/exists just return current one
	if ( pFile )
	{
		return pFile;
	}

	//	create new file object
	pFile = TLFileSys::g_pFileFactory->CreateFileInstance( Filename, GetFileSysRef() );

	//	failed to create/init
	if ( !pFile )
		return pFile;

	//	add to our list
	if ( m_Files.Exists( pFile ) )
	{
		TLDebug_Break("Shouldn't find this new file in our list");
	}
	else
	{
		m_Files.Add( pFile );
	}

	return pFile;
}


//----------------------------------------------------------
//	remove file - NULL's ptr too
//----------------------------------------------------------
Bool TLFileSys::TFileSys::RemoveFileInstance(TPtr<TFile> pFile)
{
	if ( !pFile )
	{
		TLDebug_Break("File expected");
		return FALSE;
	}

	//	do removal from factory
	if ( TLFileSys::g_pFileFactory )
	{
		TLFileSys::g_pFileFactory->RemoveFileInstance( pFile );
	}

	//	remove from OUR list
	if ( !m_Files.Remove( pFile ) )
	{
		TLDebug_Break("Should have removed instance... suggests file is NOT stored in this file sys...");
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------
//	update file list if it's out of date.
//	returns FALSE if no changes, WAIT if possible changes,
//	TRUE if there were any changes
//----------------------------------------------------------
SyncBool TLFileSys::TFileSys::UpdateFileList()
{
	//	gr: just to stop recursing -although we're not getting stuck in a loop-
	//		we're doing excessive work.
	//		routine updateFileList, a file changes, triggers a reload of the asset
	//		which triggers another UpdateFileList  but the timestamp is out of date
	//		(over 10 seconds, so this probably only goes wrong when debugging...)
	//		and causes duplicated work.
	//	when we become threaded, in theory the file system[s] will be locked from message
	//	handling until it's all done anyway so we won't have this trouble...
	if ( m_UpdatingFileList )
	{
		//	no changes
		return SyncWait;
	}

	Bool ReloadFilelist = FALSE;

	//	if timestamp is valid compare with current time
	if ( m_LastFileListUpdate.IsValid() )
	{
		TLTime::TTimestamp TimeNow = TLTime::GetTimeNow();
		if ( m_LastFileListUpdate.GetSecondsDiff( TimeNow ) > (s32)GetFilelistTimeoutSecs() )
		{
			ReloadFilelist = TRUE;
		}
	}
	else
	{
		//	timestamp is invalid, get filelist
		ReloadFilelist = TRUE;
	}

	//	file list doesnt need reloading, return no changes
	if ( !ReloadFilelist )
		return SyncFalse;
	
	//	mark as updating file list
	m_UpdatingFileList = true;

	//	reload file list, if failed return no change
	SyncBool LoadResult = LoadFileList();

	//	update timestamp
	m_LastFileListUpdate.SetTimestampNow();

	if ( LoadResult == SyncTrue )
	{
	#ifdef _DEBUG
		TTempString Debug_String("New file list for file sys: ");
		this->GetFileSysRef().GetString( Debug_String );
		Debug_String.Appendf(". %d files: \n", GetFileList().GetSize() );
		for ( u32 f=0;	f<GetFileList().GetSize();	f++ )
		{
			TLFileSys::TFile& File = *(GetFileList().ElementAt(f));
			File.Debug_GetString( Debug_String );
			TLDebug_Print( Debug_String );
			Debug_String.Empty();
		}
	#endif
	}

	//	no longer updating
	m_UpdatingFileList = false;

	return LoadResult;
}


//----------------------------------------------------------
//	update timestamp and flush missing files. returns true if any files were removed or changed
//----------------------------------------------------------
Bool TLFileSys::TFileSys::FinaliseFileList()
{
	Bool Changed = false;

	//	update time stamp of file list
	m_LastFileListUpdate.SetTimestampNow();

	//	flush missing/null files
	for ( s32 f=GetFileList().GetSize()-1;	f>=0;	f-- )
	{
		TPtr<TFile>& pFile = GetFileList().ElementAt(f);

		//	if flagged missing then remove instance (we still flag it in case soemthing has a TPtr to it still)
		if ( pFile->GetFlags()( TFile::Lost ) )
		{
			RemoveFileInstance( pFile );
			Changed = true;
			continue;
		}

		//	for any files that have changed, send out notification
		if ( pFile->IsOutOfDate() )
		{
			//	get manager to notify that file has changed
			TLFileSys::g_pFactory->OnFileChanged( pFile->GetFileAndTypeRef(), this->GetFileSysRef() );	

			Changed = true;
		}
	}

	return Changed;
}


