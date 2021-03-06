
#pragma once

#include "TPublisher.h"

class TLMessaging::TSubscriber
{
	friend class TLMessaging::TPublisher;
public:
	virtual ~TSubscriber()			{	Shutdown();	}	//	gr: moved this out of being protected so the arrays can delete them, not sure what this exposes as there were no comments as to why it was protected...
	
	FORCEINLINE Bool	SubscribeTo(TPublisher* pPublisher)				{	return pPublisher ? pPublisher->Subscribe(this) : false;	}
	FORCEINLINE Bool	UnsubscribeFrom(TPublisher* pPublisher)			{	return pPublisher ? pPublisher->Unsubscribe(this) : false;	}
	FORCEINLINE Bool	SubscribeTo(TPublisher& Publisher)				{	return Publisher.Subscribe(this);	}
	FORCEINLINE Bool	UnsubscribeFrom(TPublisher& Publisher)			{	return Publisher.Unsubscribe(this);	}

	virtual TRefRef		GetSubscriberRef() const=0;

	FORCEINLINE bool	operator==(const TSubscriber*& pSubscriber) const	{	return this == pSubscriber;	}
	
protected:
	FORCEINLINE void	Shutdown()										{	UnsubscribeAll();	}
	
	virtual void		ProcessMessage(TLMessaging::TMessage& Message) = 0;

private:
	FORCEINLINE Bool	AddPublisher(TPublisher* pPublisher)		{	return m_Publishers.AddUnique(pPublisher) != -1;	}
	FORCEINLINE Bool	RemovePublisher(TPublisher* pPublisher)		{	return m_Publishers.Remove(pPublisher);	}
	FORCEINLINE void	UnsubscribeAll();							// Unsubscribe from all publishers

private:
	TPointerArray<TLMessaging::TPublisher,false>		m_Publishers;			// List of publishers
};

//	explicitly set pointer as data type (need to find a way to do this generically!)
TLCore_DeclareIsDataType(TLMessaging::TSubscriber*);



FORCEINLINE void TLMessaging::TSubscriber::UnsubscribeAll()
{
	// Unsubscribe from all publishers
	for(u32 uIndex = 0; uIndex < m_Publishers.GetSize(); uIndex++)
	{
		TPublisher* pPublisher = m_Publishers.ElementAt(uIndex);
		pPublisher->RemoveSubscriber(this);
	}

	// Empty the list
	m_Publishers.Empty();
}

