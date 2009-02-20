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
	m_FileSysTypeRef	( FileSysTypeRef )
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
TPtr<TLFileSys::TFile> TLFileSys::TFileSys::CreateFileInstance(const TString& Filename,TRef TypeRef)
{
	//	generate a file ref from the file name (and type if provided)
	TLFileSys::TFileRef FileRef = GetFileRef( Filename, TypeRef );

	//	see if this file already exists
	TPtr<TFile> pFile = GetFile( FileRef );

	/*	gr: now, two files with the same name(first 5 chars) AND extension, 2nd one disapears
	//	as this is a new file, we want to make sure we're not overwriting the fileref of
	//	another file. 
	//	this means LongFileName1 and LongFileName2 can both exist and generate different refs
	if ( !pFile )
	{
		//	see if there's a file with this file ref
		TPtr<TFile> pDupeFile = GetFile( FileRef, FALSE );
		while ( pDupeFile )
		{
			FileRef.Increment();
			pDupeFile = GetFile( FileRef, FALSE );
		}
	}
	*/

	//	already created/exists just return current one - if it's a different file, that's tough luck, maybe files need renaming
	if ( pFile )
	{
#ifdef _DEBUG
		if ( pFile->GetFilename() != Filename )
		{
			TLDebug_Warning("Found two files with the same FileRef.TypeRef, but different filenames. Consider renaming your files!");
		}
#endif
		return pFile;
	}

	//	create new file object
	pFile = TLFileSys::g_pFileFactory->CreateFileInstance( FileRef, GetFileSysRef(), Filename );

	//	failed to create/init
	if ( !pFile )
		return pFile;

	//	add to our list
	if ( m_Files.Exists( pFile ) )
	{
		TLDebug_Break("Shouldn't find this new file in our list");
	}
	m_Files.Add( pFile );

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
	
	//	reload file list, if failed return no change
	SyncBool LoadResult = LoadFileList();

	if ( LoadResult == SyncFalse )
		return SyncFalse;

	return SyncWait;
}


//----------------------------------------------------------
//	update timestamp and flush missing files
//----------------------------------------------------------
void TLFileSys::TFileSys::FinaliseFileList()
{
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
			continue;
		}
	}
}


//----------------------------------------------------------
//	generate file ref and type ref from filename
//----------------------------------------------------------
TLFileSys::TFileRef TLFileSys::TFileSys::GetFileRef(const TString& Filename,TRef TypeRef)
{	
	//	extract a file type from the extension of the filename if it's not been provided
	TArray<TString> FilenameParts;
	TRef FileRef;
	
	//	no .'s in the filename, so use invalid file type and generate filename in the normal way
	if ( !Filename.Split('.',FilenameParts) )
	{
		FileRef.Set( Filename );
		//	gr: dont invalidate, just leave it as provided
		//TypeRef.SetInvalid();	
	}
	else 
	{
		//	need a type ref...
		if ( !TypeRef.IsValid() && FilenameParts.GetSize() > 1 )
		{
			TypeRef.Set( FilenameParts.ElementLastConst() );
			FilenameParts.RemoveLast();
		}

		//	get filename from first part only - world.scheme.asset becomes just "world"
		FileRef.Set( FilenameParts[0] );
	}

	return TFileRef( FileRef, TypeRef );
}



