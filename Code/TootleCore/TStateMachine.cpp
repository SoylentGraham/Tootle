#include "TStateMachine.h"




TRef TStateMachine::GetCurrentModeRef() const		
{	
	return m_pCurrentMode ? m_pCurrentMode->GetModeRef() : TRef();	
}


//-----------------------------------------------------------
//	update current mode
//-----------------------------------------------------------
void TStateMachine::Update()
{
	//	not in any mode
	if ( !m_pCurrentMode )
		return;

	//	do update
	TRef NextModeRef = m_pCurrentMode->Update();

	//	mode change requested
	if ( NextModeRef.IsValid() )
	{
		SetMode( NextModeRef );
	}
}


//-----------------------------------------------------------
//	change current mode
//-----------------------------------------------------------
Bool TStateMachine::SetMode(TRefRef NextModeRef)
{
	TPtr<TStateMode> pNextMode = GetMode(NextModeRef);
	TRef CurrentModeRef = GetCurrentModeRef();

	//	throw an error if we tried to change to a valid mode and we couldnt find it
	if ( !pNextMode && NextModeRef.IsValid() )
	{
		TTempString NextModeString;
		NextModeRef.GetString( NextModeString );
		TTempString CurrentModeString;
		CurrentModeRef.GetString( CurrentModeString );
		TLDebug_Break( TString("Attempted to change to non-existant mode %s from mode %s", NextModeString.GetData(), CurrentModeString.GetData() ) );
	}

	//	begin new mode (if failed we'll go onto no mode)
	if ( pNextMode )
	{
		if ( !pNextMode->OnBegin( CurrentModeRef ) )
			pNextMode = NULL;
	}

	//	end current mode
	if ( m_pCurrentMode )
	{
		m_pCurrentMode->OnEnd( pNextMode ? pNextMode->GetModeRef() : TRef() );
	}

	//	set current mode to the new mode
	m_pCurrentMode = pNextMode;

	//	log mode change
	m_Debug_ModeLog.Add( GetCurrentModeRef() );

	return m_pCurrentMode.IsValid();
}

	
//-----------------------------------------------------------
//	add mode to list of modes - returns new mode's ref. Invalid if not created
//-----------------------------------------------------------
TRef TStateMachine::AddMode(TRefRef ModeRef,TPtr<TStateMode>& pMode)
{
	if ( !pMode )
	{
		TLDebug_Break("New mode expected");
		return TRef();
	}

	//	set ref on mode
	if ( !pMode->Init( ModeRef, this ) )
		return TRef();

	//	see if we've already got a mode with this ref
	TPtr<TStateMode> pExistingMode = GetMode( ModeRef );
	if ( pExistingMode )
	{
		TTempString ModeRefString;
		ModeRef.GetString( ModeRefString );
		TLDebug_Break( TString("Mode already exists with ref %s", ModeRefString.GetData() ) );
		return TRef();
	}

	//	add mode to list
	s32 ModeIndex = m_Modes.Add( pMode );

	//	failed
	if ( ModeIndex == -1 )
		return TRef();

	//	first mode, set as the current mode
	if ( ModeIndex == 0 )
		SetMode( pMode->GetModeRef() );

	return pMode->GetModeRef();
}


//-----------------------------------------------------------
//	debugprint the mode log
//-----------------------------------------------------------
void TStateMachine::Debug_ModeLog(const TString& StateMachineName) const
{
	TTempString DebugString;
	DebugString.Append( StateMachineName );
	DebugString.Appendf(" mode history (%d changes)", m_Debug_ModeLog.GetSize() );
	TLDebug_Print(DebugString);

	for ( u32 i=0;	i<m_Debug_ModeLog.GetSize();	i++ )
	{
		DebugString.Empty();
		TTempString ModeRefString;
		m_Debug_ModeLog[i].GetString( ModeRefString );

		DebugString.Appendf("%2d: %s", i, ModeRefString.GetData() );

		//	if last mode append ("current") to it
		if ( i == m_Debug_ModeLog.GetLastIndex() )
			DebugString.Append(" (current mode)");

		TLDebug_Print(DebugString);
	}
}



//-----------------------------------------------------------
//	initialise mode and check params are valid
//-----------------------------------------------------------
Bool TStateMode::Init(TRefRef ModeRef,TStateMachine* pStateMachine)			
{
	//	check params
	if ( !pStateMachine )
	{
		TLDebug_Break("State machine expected");
		return FALSE;
	}

	if ( !ModeRef.IsValid() )
	{
		TLDebug_Break("Mode ref invalid");
		return FALSE;
	}

	//	set vars
	m_ModeRef = ModeRef;
	m_pStateMachine = pStateMachine;

	return TRUE;
}
