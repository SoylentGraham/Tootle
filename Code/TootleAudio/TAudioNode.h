#pragma once

#include <TootleCore/TLGraph.h>
#include <TootleCore/TFlags.h>
#include <TootleCore/TTransform.h>

#include <TootleAsset/TAudio.h>


/*
 
	The TAudioNode is the main object controling audio in game.
 
	An audio node will call the platform speific routines for generating the source audio which has a 1:1 mapping with the node, when the 
	audio nodes audio asset is set.  Once it is mapped to an audio source using the audio asset it will begin to play and control of the audio can
	be specified using the control routines on the node.  Upon completion the node will be removed automatically which will remove the source audio object as well.
 
	The audio asset will remain which essentially keeps track of the original audio data - 'buffer' in OpenAL.
 
	The audio system will utilise the subscription handling to be able to be aware of events such as the 'buffer' being unloaded or the 'source' ending.
	
*/


namespace TLAudio
{
	class TAudioNode;
	class TAudiograph;

	class TAudioProperties;
};


class TLAudio::TAudioProperties
{
public:
	TAudioProperties() :
		m_fVolume(0.0f),
		m_fFrequencyMult(1.0f),
		m_fPitch(1.0f),
		m_fMinRange(1.0f),
		m_fMaxRange(99999.0f),
		m_fRateOfDecay(1.0f),
		m_bRelative(FALSE),
		m_bStreaming(FALSE),
		m_bLooping(FALSE)
	{
	}
	
	float m_fVolume;
	float m_fFrequencyMult;		// Frequency multiplier
	float m_fPitch;
	float m_fMinRange;			// Range within which the audio is full volume
	float m_fMaxRange;			// Range outside which the audio will be silent and potentially culled
	float m_fRateOfDecay;		// Rate of decay of the audio volume past the min range			
	
	Bool m_bRelative;			// Relative flag
	Bool m_bStreaming;			// Streaming flag
	Bool m_bLooping;			// Looping flag
};


class TLAudio::TAudioNode : public TLGraph::TGraphNode<TAudioNode>, public TLMaths::TTransform
{
	friend class TLAudio::TAudiograph;

	enum AudioFlags
	{
		AutoRelease = 0,			// Auto release flag which will allow this node to automatically remove itself when the associated audio has stopped
		Release,					// Release node - triggers request to remove node from graph
	};

public:
	TAudioNode(TRefRef NodeRef,TRefRef TypeRef);

	virtual void			Initialise(TLMessaging::TMessage& Message);
	virtual void 			Update(float fTimestep);					
	virtual void			Shutdown();									
	
	FORCEINLINE Bool		IsEnabled() const				{	return TRUE;	}	//	for the graph templating - audio nodes aren't explicitly disabled... (yet)

	FORCEINLINE Bool		operator==(const TLAudio::TAudioNode& Node) const		{	return this == (&Node);	}
	FORCEINLINE Bool			operator<(TRefRef NodeRef) const			{	return GetNodeRef() < NodeRef;	}
	FORCEINLINE Bool			operator==(TRefRef NodeRef) const			{	return GetNodeRef() == NodeRef;	}
	FORCEINLINE Bool		operator<(const TSorter<TAudioNode*,TRef>& That) const	{	return GetNodeRef() < That.This()->GetNodeRef();	}
	FORCEINLINE Bool		operator<(const TSorter<TAudioNode,TRef>& That) const	{	return GetNodeRef() < That->GetNodeRef();	}

protected:
	void				GetAudioAsset(TPtr<TLAsset::TAudio>& pAudio);						//	returns the audio asset from the asset library with the asset reference

	Bool				Play();
	void				Pause();
	void				Stop();
	void				Reset();
	
	// Audio properties
	void				SetVolume(float fVolume, const Bool& bForced = FALSE);			// Set the volume of this instance
	FORCEINLINE float	GetVolume()					const		{ return m_AudioProperties.m_fVolume; }
	
	void				SetFrequencyMult(float fFrequencyMult);		// Set the frequency multiplier of this instance
	FORCEINLINE float	GetFrequencyMult()			const		{ return m_AudioProperties.m_fFrequencyMult; }

	void				SetPitch(float fPitch);
	FORCEINLINE float	GetPitch()					const		{ return m_AudioProperties.m_fPitch; }

	void				SetMinRange(float fDistance);
	FORCEINLINE float	GetMinRange()				const		{ return m_AudioProperties.m_fMinRange; }

	void				SetMaxRange(float fDistance);
	FORCEINLINE float	GetMaxRange()				const		{ return m_AudioProperties.m_fMaxRange; }


	void				SetRateOfDecay(float fRateOfDecay);
	FORCEINLINE float	GetRateOfDecay()			const		{ return m_AudioProperties.m_fRateOfDecay; }


	void				SetLooping(const Bool& bLooping);
	FORCEINLINE Bool	GetIsLooping()				const		{ return m_AudioProperties.m_bLooping; }

	void				SetStreaming(const Bool& bStreaming);
	FORCEINLINE Bool	GetIsStreaming()			const		{ return m_AudioProperties.m_bStreaming; }

	void				SetRelative(const Bool& bRelative);
	FORCEINLINE Bool	GetIsRelative()			const		{ return m_AudioProperties.m_bRelative; }


	// Audio asset access
	FORCEINLINE TRefRef	GetAudioAssetRef()			const		{	return m_AudioAssetRef;	}
	Bool				SetAudioAssetRef(TRefRef AssetRef);

	FORCEINLINE TRefRef	GetOwnerSceneNodeRef()		const		{	return m_OwnerSceneNode;	}

	void				UpdatePreviousPos()	{ m_vPreviousPos = GetTranslate(); }

	virtual void		ProcessMessage(TLMessaging::TMessage& Message);

	virtual float		GetGlobalVolume();

private:
	
	Bool				CreateSource();						// Generates the source audio data using the audio asset specified
	void				RemoveSource();						// Removes the source audio data

	Bool				CreateBuffer();						// TEMP: Buffer creation
	
	Bool				IsSourceActive();					// Checs the low level audio system to see if a source is active with the node ID

private:
	TAudioProperties	m_AudioProperties;		// Audio properties
	float3				m_vPreviousPos;			// Previous Audio node position
	
	TRef				m_AudioAssetRef;		// Audio asset to use

	TRef				m_OwnerSceneNode;			//	"Owner" scene node - if this is set then we automaticcly process some stuff

	TFlags<AudioFlags>	m_AudioFlags;
};


FORCEINLINE bool operator==(const TLAudio::TAudioNode* pNode,const TRef& Ref)	{	return pNode ? (*pNode == Ref) : false;	}
