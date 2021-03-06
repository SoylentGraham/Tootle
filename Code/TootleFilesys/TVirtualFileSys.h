/*------------------------------------------------------
	
	Debug file system - this is for creating assets
	at runtime. Cubes, spheres etc

-------------------------------------------------------*/
#pragma once
#include "TLFileSys.h"

namespace TLFileSys
{
	class TVirtualFileSys;	
};



//------------------------------------------------------------
//	debug file system - this file system generates files at runtime
//	can just be used as a dummy file system too
//------------------------------------------------------------
class TLFileSys::TVirtualFileSys : public TLFileSys::TFileSys
{
public:
	TVirtualFileSys(TRefRef FileSysRef,TRefRef FileSysTypeRef);

	virtual bool			IsWritable() const					{	return true;	}
	virtual SyncBool		LoadFileList();
	virtual SyncBool		LoadFile(TPtr<TFile>& pFile);
	virtual SyncBool		WriteFile(TPtr<TFile>& pFile);		//	add this file into the file system if it's not there
	virtual TPtr<TFile>		CreateNewFile(const TString& Filename);	//	create a new empty file into file system if possible - if the filesys is read-only we cannot add external files and this fails

protected:
	bool					m_CreatedDebugFiles;				//	create the debug files/assets only once
};

