/*------------------------------------------------------
	
	Base filesystem type & factory

-------------------------------------------------------*/
#pragma once
#include <TootleCore/TLTypes.h>
#include <TootleCore/TClassFactory.h>
#include <TootleCore/TLTime.h>
#include <TootleCore/TManager.h>
#include "TFile.h"
#include "TFileSys.h"


namespace TLFileSys
{
	class TFileSysFactory;		//	file system class factory
	class TFileFactory;			//	TFile class factory
	class TFileGroup;			//	groups files with the same ref (same filename) together so we can sort out which to load/convert/etc

	TPtr<TLFileSys::TFileSys>&	GetFileSys(TRefRef FileSysRef);		//	return a file system
	void						GetFileSys(TPtrArray<TLFileSys::TFileSys>& FileSysList,TRefRef FileSysRef,TRefRef FileSysTypeRef);	//	return all matching file systems to these refs/types
	TPtrArray<TLFileSys::TFileSys>&	GetFileSystems();					//	return al the file systems
	void						GetFileList(TArray<TRef>& FileList);	//	get a list of all files in all the file systems (gets refs out of the groups)
	TPtr<TFile>&				GetLatestFile(TTypedRefRef FileRef);	//	given a file & type - get the latest version of that file from the file groups
	TPtr<TFileGroup>&			GetFileGroup(TRefRef FileRef);		//	get the group of files for a file ref
	Bool						UpdateFileLists();					//	update the file lists on the file systems, returns TRUE if any file sys' has change
	
	TPtr<TFile>					CreateFileInFileSys(const TString& Filename,TPtrArray<TLFileSys::TFileSys>& FileSysList);
	TPtr<TFile>					CreateAssetFileInFileSys(const TTypedRef& AssetAndTypeRef,TPtrArray<TLFileSys::TFileSys>& FileSysList);	//	wrapper to create a file for a .asset file (to ensure consistent filenames)
	TPtr<TFile>&				GetLatestFile(TPtrArray<TLFileSys::TFile>& Files,TRef FileType=TRef(),TRefRef ExportAssetType=TRef());	//	from a list of files, return the one with the most recent timestamp

	//	create a local file system
	SyncBool					CreateLocalFileSys(TRef& FileSysRef,const TString& Directory,Bool IsWritable);	//	async create a local filesystem for the specified path. FileSysRef is set to the new file system's ref for the purposes of asynchronousness so keep using it when async calling this func

	Bool						GetParentDir(TString& Directory);					//	directory manipulation - turns filename to dir, then chops off a dir at a time
	TTypedRef					GetFileAndTypeRef(const TString& Filename);			//	generate file ref with explicit type

	extern TPtr<TFileSysFactory>	g_pFactory;			//	extern'd as it's a manager, the file factory is not exposed
	

	// System specific directory helper routines
	Bool GetAssetDirectory(TString& AssetDir);
	Bool GetAssetSubDirectory(TString& AssetDir, const TString& Subdirectory);
	Bool GetUserDirectory(TString& UserDir);
	
	namespace Platform
	{
		Bool GetAssetDirectory(TString& AssetDir);
		Bool GetUserDirectory(TString& UserDir);
	}
	
};




//------------------------------------------------------------
//	class factory for file systems
//------------------------------------------------------------
class TLFileSys::TFileSysFactory : public TLCore::TManager, public TClassFactory<TLFileSys::TFileSys>
{
	friend class TLFileSys::TFileSys;
public:
	TFileSysFactory(TRefRef ManagerRef);
	~TFileSysFactory();

	Bool							UpdateFileLists();					//	update file lists of the file systems, return TRUE if any changed

protected:
	virtual SyncBool				Initialise();
	virtual SyncBool				Shutdown();
	virtual SyncBool				Update(float fTimeStep);

	virtual TLFileSys::TFileSys*	CreateObject(TRefRef InstanceRef,TRefRef TypeRef);
	
	virtual void					OnEventChannelAdded(TRefRef refPublisherID, TRefRef refChannelID);

	void							OnFileChanged(TTypedRefRef FileRef,TRefRef FileSysRef);	//	notify subscribers that this file has changed
};


//------------------------------------------------------------
//	class factory for file systems
//------------------------------------------------------------
class TLFileSys::TFileFactory : public TClassFactory<TLFileSys::TFile>
{
public:
	TFileFactory()					{}

	FORCEINLINE TPtr<TFileGroup>&				GetFileGroup(TRefRef FileRef)		{	return m_FileGroups.FindPtr( FileRef );	}
	FORCEINLINE const TPtrArray<TFileGroup>&	GetFileGroups() const				{	return m_FileGroups;	}

	TPtr<TLFileSys::TFile>&			CreateFileInstance(const TString& Filename,TRefRef FileSysRef);	//	create instance, init, add to group
	Bool							RemoveFileInstance(TPtr<TLFileSys::TFile>& pFile);				//	delete instance, remove from group

protected:
	virtual TLFileSys::TFile*		CreateObject(TRefRef InstanceRef,TRefRef TypeRef);

private:
	void							OnFileAdded(TPtr<TFile>& pFile);	//	file was found in a file system, put it into it's group
	void							OnFileRemoved(TPtr<TFile>& pFile);	//	file removed from a file system, remove from it's group

protected:
	TPtrArray<TFileGroup>			m_FileGroups;
};



//------------------------------------------------------------
//	Group of files with the same FileRef (filename) but with different extensions (Types)
//	we use this to fetch the most recent file, so even if there's a world.scheme.asset, 
//	we will fetch world.scheme if it's newer and it'll output a newer .asset
//------------------------------------------------------------
class TLFileSys::TFileGroup
{
public:
	TFileGroup(TRefRef FileRef);

	FORCEINLINE TRefRef	GetFileRef() const								{	return m_FileRef;	}	//	get file ref for the group

	void				Add(TPtr<TFile>& pFile);						//	add file to group
	void				Remove(TPtr<TFile>& pFile);						//	remove file from group
	TPtr<TFile>&		GetNewestFile(TRefRef FileType=TRef(),TRefRef ExportAssetType=TRef());	//	get file with newest timestamp. FileType filters file types, ExportAssetType filters only files that can export this asset type
	FORCEINLINE Bool	IsEmpty() const									{	return (m_Files.GetSize() == 0);	}
	TPtrArray<TFile>&	GetFiles()										{	return m_Files;	}

	FORCEINLINE Bool	operator==(TRefRef FileRef) const				{	return GetFileRef() == FileRef;	}
	FORCEINLINE Bool	operator==(const TFileGroup& FileGroup) const	{	return GetFileRef() == FileGroup.GetFileRef();	}

protected:
	TRef				m_FileRef;		//	file ref of group
	TPtrArray<TFile>	m_Files;		//	list of files in this group
};



//-----------------------------------------------------------
//	return al the file systems
//-----------------------------------------------------------
FORCEINLINE TPtrArray<TLFileSys::TFileSys>& TLFileSys::GetFileSystems()
{
	return g_pFactory->GetInstanceArray();
}
