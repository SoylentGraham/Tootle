/*
 *  IPodCamera.h
 *  TootleCamera
 *
 *  Created by Duane Bradbury on 28/01/2010.
 *  Copyright 2010 Tootle. All rights reserved.
 *
 */

#include <TootleCore/TLTypes.h>

namespace TLCamera
{
	namespace Platform
	{
		SyncBool Initialise();
		SyncBool Shutdown();
		
		
		Bool	ConnectToCamera();
		Bool	DisconnectFromCamera();
	}
}