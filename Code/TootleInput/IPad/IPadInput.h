/*
 *  IPadInput.h
 *  TootleInput
 *
 *  Created by Duane Bradbury on 14/09/2009.
 *  Copyright 2009 Tootle. All rights reserved.
 *
 */

#pragma once

#include <TootleCore/TLCore.h>

#include "../TLInput.h"

namespace TLInput
{
	namespace Platform
	{
		SyncBool		Init();
		SyncBool		Update();
		SyncBool		Shutdown();
		
		Bool			UpdateDevice(TInputDevice& Device);
		
		SyncBool		EnumerateDevices();
		void			RemoveAllDevices();
		
		
		SyncBool		CreateVirtualDevice(TRefRef InstanceRef, TRefRef DeviceTypeRef);
		SyncBool		RemoveVirtualDevice(TRefRef InstanceRef);

		// TEMP test routine
		void TestVibrateDevice();
				
		namespace IPod
		{
			const u32 MAX_CURSOR_POSITIONS = 4;
			extern TFixedArray<int2,MAX_CURSOR_POSITIONS>	g_CursorPositions;
			extern Bool										g_bVirtualKeyboardActive;	//	gr: replace this with a function to see if the textview controller exists or not

			// Base touch data - used for the persistent TouchObject class and as temp data passed form the
			// ipod touch system to the engine input system
			class TTouchData
			{
			public:
				enum TouchPhase
				{
					Begin = 0,
					Move,
					End,
					Cancel,
				};
				
				TTouchData()	{}
				TTouchData(TRefRef TouchDataRef) :
					TouchRef(TouchDataRef)
				{
				}
				
				
				inline Bool			operator==(const TRef& TouchDataRef)				const	{	return TouchRef == TouchDataRef;	}
				
			public:
				TRef						TouchRef;				// ID of the touch data
				int2						uCurrentPos;			// Current position of the touch event
				int2						uPreviousPos;			// Previous position of the touch event
				float						fTimestamp;				// Timestamp
				TouchPhase					uPhase;					// Currrent touch phase 
				u8							uTapCount;				// Number of taps for a particular touch event
			};
			
			// Engine side persistent touch object class
			class TTouchObject : public TTouchData
			{
			public:				
				TTouchObject()	{}
				TTouchObject(TRefRef TouchDataRef) :
					TTouchData(TouchDataRef)
				{
				}
				
				
				inline TTouchObject&			operator=(const TTouchData& TouchData)					
				{	
					uCurrentPos		= TouchData.uCurrentPos;			// Current position of the touch event
					uPreviousPos	= TouchData.uPreviousPos;			// Previous position of the touch event
					fTimestamp		= TouchData.fTimestamp;				// Timestamp
					uPhase			= TouchData.uPhase;					// Currrent touch phase 
					uTapCount		= TouchData.uTapCount;				// Number of taps for a particular touch event
					
					uStartPosition	= TouchData.uCurrentPos;			// Starting position of the touch object
					
					return *this;
				}

				
			public:
				int2						uStartPosition;			// Starting position of the touch object
			};
						
			void ProcessTouchBegin(const TTouchData& touchData);
			void ProcessTouchMoved(const TTouchData& touchData);
			void ProcessTouchEnd(const TTouchData& touchData);

			void ProcessAcceleration(const float3& vAccelerationData);
			
			void ProcessVirtualKey(TRefRef KeyRef);

			Bool CreateDevice(TRefRef InstanceRef, TRefRef DeviceTypeRef, Bool bVirtual);
			Bool InitialiseDevice(TPtr<TInputDevice> pDevice, TRefRef DeviceTypeRef, Bool bVirtual);
		
			
			FORCEINLINE void		SetCursorPosition(u8 Index,const int2& Pos)	{	g_CursorPositions[Index] = Pos;	}
			FORCEINLINE const int2&	GetCursorPosition(u8 Index)					{	return g_CursorPositions[Index];	}
		}
	};
};


TLCore_DeclareIsDataType( TLInput::Platform::IPod::TTouchData );
