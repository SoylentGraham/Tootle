/*
 *  TFileAssetScript.h
 *  TootleFileSys
 *
 *  Created by Duane Bradbury on 15/03/2009.
 *  Copyright 2009 Tootle. All rights reserved.
 *
 */

#pragma once

#include "TFile.h"
#include "TFileXml.h"


namespace TLFileSys
{
	class TFileAssetScript;
};

namespace TLAsset
{
	class TAssetScript;
	class TKeyframe;
}

class TLFileSys::TFileAssetScript : public TLFileSys::TFileXml
{
public:
	TFileAssetScript(TRefRef FileRef,TRefRef FileTypeRef);
	
	virtual SyncBool	ExportAsset(TPtr<TLAsset::TAsset>& pAsset,Bool& Supported);			//	import the XML and convert from Collada to mesh
	
protected:

	SyncBool			ImportAssetScript(TPtr<TLAsset::TAssetScript> pAssetScript,TPtr<TXmlTag>& pTag);
	SyncBool			ImportAssetScript_ImportKeyframeTag(TPtr<TLAsset::TAssetScript>& pAssetScript, TPtr<TXmlTag>& pImportTag);
	SyncBool			ImportAssetScript_ImportNodeTag(TPtr<TLAsset::TAssetScript>& pAssetScript, TLAsset::TKeyframe* pKeyframe, TPtr<TXmlTag>& pImportTag);
	SyncBool			ImportAssetScript_ImportCommandTag(TPtr<TLAsset::TAssetScript>& pAssetScript, TLAsset::TKeyframe* pkeyframe, TPtr<TXmlTag>& pImportTag);
};