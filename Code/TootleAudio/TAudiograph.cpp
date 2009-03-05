
#include "TAudiograph.h"
#include "TLAudio.h"
#include <TootleCore/TEventChannel.h>

namespace TLAudio
{
	TPtr<TAudiograph> g_pAudiograph;
};


using namespace TLAudio;

TAudioNode* TAudioNodeFactory::CreateObject(TRefRef InstanceRef,TRefRef TypeRef)
{
	// Create engine side audio nodes
	if(TypeRef == "Audio")
		return new TAudioNode(InstanceRef,TypeRef);
	else if(TypeRef == "Music")
		return new TAudioNode_Music(InstanceRef,TypeRef);
	
	return NULL;
}



TAudiograph::TAudiograph(TRefRef refManagerID) :
	TLGraph::TGraph<TLAudio::TAudioNode>	( refManagerID ),
	m_fMusicVolume(1.0f),
	m_fEffectsVolume(1.0f),
	m_bPause(FALSE)
{
}



SyncBool TAudiograph::Initialise()
{
	if(TLMessaging::g_pEventChannelManager)
	{
		if(InitDevices() != SyncTrue)
			return SyncFalse;

		TLMessaging::g_pEventChannelManager->RegisterEventChannel(this, "AUDIOGRAPH", "START");
		TLMessaging::g_pEventChannelManager->RegisterEventChannel(this, "AUDIOGRAPH", "STOP");
		TLMessaging::g_pEventChannelManager->RegisterEventChannel(this, "AUDIOGRAPH", "PAUSE");
		TLMessaging::g_pEventChannelManager->RegisterEventChannel(this, "AUDIOGRAPH", "VOLUME");

		if(TLGraph::TGraph<TLAudio::TAudioNode>::Initialise() == SyncTrue)
		{	
			//	create generic audio node factory
			TPtr<TClassFactory<TLAudio::TAudioNode,FALSE> > pFactory = new TAudioNodeFactory();

			if(!pFactory)
			{
				TLDebug_Print("Failed to create audio node factory");
				return SyncFalse;
			}

			AddFactory(pFactory);
			
			return SyncTrue;
		}
		
	}

	return SyncWait;
}

SyncBool TAudiograph::InitDevices()
{
	return TLAudio::Platform::Init();
}


SyncBool TAudiograph::Update(float fTimeStep)
{
	TArray<TRef> refArray;

	if(Platform::DetermineFinishedAudio(refArray))
	{
		// Send out notification of each audio item that has come to an end
		for(u32 uIndex = 0; uIndex < refArray.GetSize(); uIndex++)
		{
			TRef AudioRef = refArray.ElementAt(uIndex);

			TLMessaging::TMessage Message("AUDIO");

			Message.AddChannelID("STOP");
			Message.Write(AudioRef);

			PublishMessage(Message);
		}
	}

	return TLGraph::TGraph<TLAudio::TAudioNode>::Update(fTimeStep);
}

SyncBool TAudiograph::Shutdown()
{
	if(Platform::Shutdown() != SyncTrue)
		return SyncFalse;

	return TLGraph::TGraph<TLAudio::TAudioNode>::Shutdown();
}


void TAudiograph::ProcessMessage(TLMessaging::TMessage& Message)
{
	if(Message.GetMessageRef() == "SETVOL")
	{
		float fVolume; 
		if(Message.ImportData("Effects", fVolume))
		{
			// Clamp to within range
			TLMaths::Limit(fVolume, 0.0f, 1.0f);

			m_fEffectsVolume = fVolume;
			OnEffectsVolumeChanged();
		}

		if(Message.ImportData("Music", fVolume))
		{
			// Clamp to within range
			TLMaths::Limit(fVolume, 0.0f, 1.0f);

			m_fMusicVolume = fVolume;
			OnMusicVolumeChanged();
		}

		return;
	}
	else if(Message.GetMessageRef() == "PAUSE")
	{
		m_bPause = TRUE;
		OnPauseStateChanged();
		return;
	}
	else if(Message.GetMessageRef() == "UNPAUSE")
	{
		m_bPause = FALSE;
		OnPauseStateChanged();
		return;
	}


	// Super class process message
	TLGraph::TGraph<TLAudio::TAudioNode>::ProcessMessage(Message);
}


Bool TAudiograph::StartMusic(TRefRef AudioAsset)
{
	// Music alreayd playing
	if(m_MusicRef.IsValid())
	{
		return TRUE;
	}

	// Create a music node
	TLMessaging::TMessage Message(TLCore::InitialiseRef);

	Message.ExportData("Asset", AudioAsset);
	Message.ExportData("Play", TRUE);

	TLAudio::TAudioProperties Props;

	Props.m_bStreaming = TRUE;
	Props.m_fVolume = 1.0f;
	Message.ExportData("Props", Props);

	m_MusicRef = CreateNode("Music", "Music", "Root", &Message); 

	return m_MusicRef.IsValid();
}


void TAudiograph::OnMusicVolumeChanged()
{
	TLMessaging::TMessage Message("AUDIO");

	Message.AddChannelID("VOLUME");
	Message.ExportData("Effects", GetMusicVolume());

	PublishMessage(Message);
}

void TAudiograph::OnEffectsVolumeChanged()
{
	TLMessaging::TMessage Message("AUDIO");

	Message.AddChannelID("VOLUME");
	Message.ExportData("Effects", GetEffectsVolume());

	PublishMessage(Message);
}


void TAudiograph::OnPauseStateChanged()
{
	// Broadcast pause message to all subscribers
	TLMessaging::TMessage Message("AUDIO");

	Message.AddChannelID("PAUSE");
	Message.ExportData("State", m_bPause);

	PublishMessage(Message);
}

	