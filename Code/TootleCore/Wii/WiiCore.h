/*------------------------------------------------------
	Wii Core include header

-------------------------------------------------------*/
#pragma once

#include "../TLTypes.h"


#include <math.h>
#include <stdio.h>
#include <typeinfo>


//	forward declarations
class TString;
class TBinaryTree;


namespace TLDebug
{
	namespace Platform
	{
		void		Print(const TString& String);	//	platform specific debug output
		Bool		Break(const TString& String);	//	return FALSE to stop app, TRUE and will attempt to continue
	}
};


namespace TLTime
{
	class TTimestamp;

	namespace Platform
	{
		SyncBool			Init();				//	time init
	}
}

namespace TLCore
{
	namespace Platform
	{
		SyncBool			Init();				//	platform init
		SyncBool			Update();			//	platform update
		SyncBool			Shutdown();			//	platform shutdown
		void				Sleep(u32 Millisecs);	//	platform thread/process sleep

		void				DoQuit();			// Notification of app quit
		const TString&		GetAppExe();		//	get the application exe (full path)
		
		void				QueryHardwareInformation(TBinaryTree& Data);
		void				QueryLanguageInformation(TBinaryTree& Data);
	}
}





