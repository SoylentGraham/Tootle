/*------------------------------------------------------
	Core include header

-------------------------------------------------------*/
#pragma once

#include "TLTypes.h"
#include "TRef.h"

// Forward declarations
class TBinaryTree;

namespace TLCore
{
	//	useful functions
	u32				PointerToInteger(void* pAddress);	//	convert a pointer to an integer
	void*			IntegerToPointer(u32 Integer);		//	convert an integer to a pointer
	
	// DB - Global TRef's that get used often.  Performance optimisation
	const TRef InitialiseRef	= TRef_Static(I,n,i,t,i);	//"Initialise";
	const TRef UpdateRef		= TRef_Static(U,p,d,a,t);	//"Update";
	const TRef RenderRef		= TRef_Static(R,e,n,d,e);	//"Render";
	const TRef ShutdownRef		= TRef_Static(S,h,u,t,d);	//"Shutdown";
	const TRef TimeStepRef		= TRef_Static(T,i,m,e,s);	//"Timestep";
	const TRef TimeStepModRef	= TRef_Static(T,S,M,o,d);	//"TSMod";
	const TRef QuitRef			= TRef_Static4(Q,u,i,t);	//"Quit";

	const TRef SetPropertyRef	= TRef_Static(S,e,t,P,r);	// "SetProperty"
	const TRef GetPropertyRef	= TRef_Static(G,e,t,P,r);	// "GetProperty"
	const TRef PropertyRef		= TRef_Static(P,r,o,p,e);	// "Property"

	const TRef ManagerRef		= TRef_Static(M,a,n,a,g);	// "Manager"

	namespace Platform
	{
		SyncBool			Init();					//	platform init
		SyncBool			Update();				//	platform update
		SyncBool			Shutdown();				//	platform shutdown
		void				Sleep(u32 Millisecs);	//	platform thread/process sleep

		void				DoQuit();			// Notification of app quit
		const TString&		GetAppExe();		//	get the application exe (full path)
		
		void				QueryHardwareInformation(TBinaryTree& Data);
		void				QueryLanguageInformation(TBinaryTree& Data);

		void				OpenWebURL(TString& urlstr);
	}

};
