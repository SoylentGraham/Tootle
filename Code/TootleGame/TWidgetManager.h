

#pragma once

#include <TootleCore/TManager.h>
#include <TootleInput/TUser.h>

namespace TLGui
{
	class TWidgetManager;

	extern TPtr<TWidgetManager>	g_pWidgetManager;
}


class TLGui::TWidgetManager : public TLCore::TManager
{
private:
	
	class TActionRefData
	{
	public:
		TRef	m_ClickActionRef;
		TRef	m_MoveActionRef;
	};

public:
	TWidgetManager(TRefRef ManagerRef) :
		TLCore::TManager(ManagerRef)
	{
	}
	
	FORCEINLINE Bool	IsClickActionRef(TRefRef ClickActionRef);
	FORCEINLINE Bool	IsMoveActionRef(TRefRef MoveActionRef);
	FORCEINLINE Bool	IsActionPair(TRefRef ClickActionRef, TRefRef MoveActionRef);

	FORCEINLINE TRef	GetClickActionFromMoveAction(TRefRef MoveActionRef);
	FORCEINLINE TRef	GetMoveActionFromClickAction(TRefRef ClickActionRef);

protected:
	virtual SyncBool Shutdown();

	
	virtual void	OnEventChannelAdded(TRefRef refPublisherID, TRefRef refChannelID);
	virtual void	ProcessMessage(TLMessaging::TMessage& Message);	

	void			OnInputDeviceAdded(TRefRef refDeviceID, TRefRef refDeviceType);
	void			OnInputDeviceRemoved(TRefRef refDeviceID, TRefRef refDeviceType);

	// Action mapping
#ifdef TL_TARGET_IPOD
	void			MapDeviceActions_TouchPad(TRefRef refDeviceID, TPtr<TLUser::TUser> pUser);
#else
	void			MapDeviceActions_Mouse(TRefRef refDeviceID, TPtr<TLUser::TUser> pUser);
#endif

	void			MapDeviceActions_Keyboard(TRefRef refDeviceID, TPtr<TLUser::TUser> pUser);
	
private:
	
	TArray<TActionRefData>	m_WidgetActionRefs;
};





FORCEINLINE Bool TLGui::TWidgetManager::IsClickActionRef(TRefRef ClickActionRef)
{
	for(u32 uIndex = 0; uIndex < m_WidgetActionRefs.GetSize(); uIndex++)
	{
		if(m_WidgetActionRefs.ElementAt(uIndex).m_ClickActionRef == ClickActionRef)
			return TRUE;
	}
	return FALSE;
}

FORCEINLINE Bool TLGui::TWidgetManager::IsMoveActionRef(TRefRef MoveActionRef)
{
	for(u32 uIndex = 0; uIndex < m_WidgetActionRefs.GetSize(); uIndex++)
	{
		if(m_WidgetActionRefs.ElementAt(uIndex).m_MoveActionRef == MoveActionRef)
			return TRUE;
	}
	return FALSE;
}

FORCEINLINE Bool TLGui::TWidgetManager::IsActionPair(TRefRef ClickActionRef, TRefRef MoveActionRef)
{
	for(u32 uIndex = 0; uIndex < m_WidgetActionRefs.GetSize(); uIndex++)
	{
		const TActionRefData& ActionPair = m_WidgetActionRefs.ElementAt(uIndex);
		if( ActionPair.m_ClickActionRef == ClickActionRef)
		{
			if(ActionPair.m_MoveActionRef == MoveActionRef)
				return TRUE;

			// Actions will be unique so if we find the click ref 
			// but move is different then we should not find any further click with the same ref
			// therefore we can return
			return FALSE;
		}
	}
	return FALSE;
}


FORCEINLINE TRef TLGui::TWidgetManager::GetClickActionFromMoveAction(TRefRef MoveActionRef)
{
	for(u32 uIndex = 0; uIndex < m_WidgetActionRefs.GetSize(); uIndex++)
	{
		const TActionRefData& ActionPair = m_WidgetActionRefs.ElementAt(uIndex);
	
		if(ActionPair.m_MoveActionRef == MoveActionRef)
			return ActionPair.m_ClickActionRef;
	}
	
	return TRef_Invalid;
}


FORCEINLINE TRef TLGui::TWidgetManager::GetMoveActionFromClickAction(TRefRef ClickActionRef)
{
	for(u32 uIndex = 0; uIndex < m_WidgetActionRefs.GetSize(); uIndex++)
	{
		const TActionRefData& ActionPair = m_WidgetActionRefs.ElementAt(uIndex);
	
		if(ActionPair.m_ClickActionRef == ClickActionRef)
			return ActionPair.m_MoveActionRef;
	}
	
	return TRef_Invalid;
}


