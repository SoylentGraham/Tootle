#include "TFile.h"
#include <TootleCore/TLMemory.h>
#include "TFileAsset.h"
#include "TLFileSys.h"
#include <TootleAsset/TAsset.h>




TLFileSys::TFile::TFile(TRefRef InstanceRef,TRefRef TypeRef) :
	m_FileSize			( -1 ),
	m_InstanceRef		( InstanceRef ),
	m_FileAndTypeRef	( TRef(), TypeRef ),
	m_IsLoaded			( SyncFalse )
{
}


//-----------------------------------------------------------
//	initialise the file
//-----------------------------------------------------------
SyncBool TLFileSys::TFile::Init(TRefRef FileRef,TRefRef FileSysRef,const TString& Filename)
{
	//	the file ref should be invalid at this point
	if ( m_FileAndTypeRef.GetRef().IsValid() )
	{
		TLDebug_Break("Expected file's FileRef to be invalid, as it is NOT assigned at construction");
	}

	//	set params
	m_FileAndTypeRef.SetRef( FileRef );
	m_FileSysRef = FileSysRef;
	m_Filename = Filename;

	return SyncTrue;
}


//-----------------------------------------------------------
//	update timestamp
//-----------------------------------------------------------
void TLFileSys::TFile::SetTimestamp(const TLTime::TTimestamp& NewTimestamp)	
{
	//	if the old timestamp was invalid then just set new timestamp
	if ( !m_Timestamp.IsValid() )
	{
		m_Timestamp = NewTimestamp;
		return;
	}

	//	timestamp hasnt changed... do nothing
	if ( m_Timestamp == NewTimestamp )
		return;

	//	check in case timestamp is older
	//	gr: check seconds, some file systems dont record millisecond timestamps (only virtual ones do!)
	s32 SecondsDiff = m_Timestamp.GetSecondsDiff( NewTimestamp );
	//	timestamp is older??
	if ( SecondsDiff < 0 )
	{
		s32 MilliDiff = m_Timestamp.GetMilliSecondsDiff( NewTimestamp );
		if ( !TLDebug_Break( TString("File's timestamp is OLDER than what it was before? (%d seconds, %d milliseconds diff)", SecondsDiff, MilliDiff ) ) )
			return;
	}

	//	if the file is laoded, it's now out of date
	if ( IsLoaded() != SyncFalse )
		GetFlags().Set( TFile::OutOfDate );

	//	set new timestamp
	m_Timestamp = NewTimestamp;	
}


//-----------------------------------------------------------
//	update timestamp
//-----------------------------------------------------------
void TLFileSys::TFile::SetTimestampNow()
{
	//	update timestamp
	m_Timestamp.SetTimestampNow();

	//	if the file is laoded, it's now out of date
	if ( IsLoaded() != SyncFalse )
		GetFlags().Set( TFile::OutOfDate );
}


//-----------------------------------------------------------
//	turn this file into an asset file, when we create it, put it into this file system
//-----------------------------------------------------------
SyncBool TLFileSys::TFile::Export(TPtr<TFileAsset>& pAssetFile)
{
	if ( !pAssetFile )
	{
		TLDebug_Break("Asset file expected");
		return SyncFalse;
	}

	//	does this file convert to an asset? if so genericlly create assetfile from asset that we create
	Bool DoExportAsset = TRUE;
	
	//	do we need to continue loading an existing asset?
	if ( m_pExportAsset )
	{
		if ( m_pExportAsset->GetLoadingState() == TLAsset::LoadingState_Loading )
			DoExportAsset = TRUE;
		//	else asset doesnt need to continue being loaded
	}

	if ( DoExportAsset )
	{
		Bool Supported = FALSE;
		SyncBool ExportAssetResult = ExportAsset( m_pExportAsset, Supported );

		//	is supported so see how it went... and convert
		if ( Supported )
		{
			//	update state of existing asset
			if ( m_pExportAsset )
			{
				if ( ExportAssetResult == SyncFalse )
					m_pExportAsset->SetLoadingState( TLAsset::LoadingState_Failed );
				else if ( ExportAssetResult == SyncWait )
					m_pExportAsset->SetLoadingState( TLAsset::LoadingState_Loading );
				else if ( ExportAssetResult == SyncTrue )
					m_pExportAsset->SetLoadingState( TLAsset::LoadingState_Loaded );
			}

			//	supported but still processing
			if ( ExportAssetResult == SyncWait )
				return SyncWait;

			//	failed to export to asset
			if ( ExportAssetResult == SyncFalse )
			{
				m_pExportAsset = NULL;
				return SyncFalse;
			}
		}
		else
		{
			if ( m_pExportAsset )
			{
				if ( !TLDebug_Break("ExportAsset() unsupported... but generated asset...") )
				{
					m_pExportAsset = NULL;
					return SyncFalse;
				}
			}

			m_pExportAsset = NULL;

			#ifdef _DEBUG
			TTempString Debug_String("TFile ");
			Debug_String.Append( GetFilename() );
			Debug_String.Append(" (");
			GetTypeRef().GetString( Debug_String );
			Debug_String.Append(") does not support exporting asset - marked as unknown type");
			TLDebug_Break( Debug_String );
			#endif
			SetUnknownType();
			return SyncFalse;
		}
	}

	//	convert asset to asset file
	if ( m_pExportAsset )
	{
		//	loading state should be loaded
		if ( !m_pExportAsset->IsLoaded() )
		{
			TLDebug_Break("Asset should be loaded at this point");
			return SyncWait;
		}

		//	export asset to asset file
		SyncBool ExportResult = m_pExportAsset->Export( pAssetFile );
		if ( ExportResult == SyncWait )
			return SyncWait;

		//	failed, cleanup
		if ( ExportResult == SyncFalse )
		{
			m_pExportAsset = NULL;
			return SyncFalse;
		}
	}
	else
	{
		//	gr: from now on, we do not turn files we don't recognise into .asset files
		//	instead we mark them as unknown file types and fail to convert.
		//	if we still want to get the pure data out of a file [I THINK] you just need to
		//	get the TFileFactory::CreateObject to return an "asset" type of file.
		//	[gr: if that doesn't work, then make up some binary type and use the code below]
		TLDebug_Break("This should have already been caught - TFile type export must be supported, but failed to generate asset?");
		return SyncFalse;
		/*
		//	masquerade as a generic binary file
		pAssetFile->GetHeader().m_TootFileRef = TLFileSys::g_TootFileRef;
		pAssetFile->GetHeader().m_AssetType = "Asset";

		//	base code just sticks all our binary data into the root of the asset file 
		TBinaryTree& BinaryTree = pAssetFile->GetData();
		BinaryTree.Write( this->GetData() );

		//	set root of binary data to be called "data"
		BinaryTree.SetDataRef("Data");

		//	just set data info in header to what it is
		pAssetFile->GetHeader().m_DataLength = this->GetSize();
		pAssetFile->GetHeader().m_DataCheckSum = this->GetChecksum();

		//	data is not compressed
		pAssetFile->GetHeader().m_Flags.Clear( TFileAsset::Compressed );
		*/
	}

	//	this file no longer needs to be imported, but the binary file itself is out of date
	//	(the binary part of the asset file isnt THIS, it's been created from scratch)
	pAssetFile->SetNeedsImport( FALSE );
	pAssetFile->SetNeedsExport( TRUE );

	return SyncTrue;
}


//-----------------------------------------------------------
//	get a pointer to the file sys this file is owned by (GetFileSysRef)
//-----------------------------------------------------------
TPtr<TLFileSys::TFileSys> TLFileSys::TFile::GetFileSys() const
{
	return TLFileSys::GetFileSys( GetFileSysRef() );
}


//-----------------------------------------------------------
//	copy file data and attributes (timestamp, flags)
//-----------------------------------------------------------
Bool TLFileSys::TFile::Copy(TPtr<TFile>& pFile,Bool CopyFilename)
{
	if ( !pFile )
	{
		TLDebug_Break("File expected");
		return FALSE;
	}

	//	copy data
	GetData().Copy( pFile->GetData() );

	//	copy attributes
	m_IsLoaded = pFile->IsLoaded();
	m_Flags = pFile->GetFlags();
	m_Timestamp = pFile->GetTimestamp();

	//	copy refs?

	//	copy filename
	if ( CopyFilename )
	{
		m_Filename = pFile->GetFilename();
	}

	return TRUE;
}


