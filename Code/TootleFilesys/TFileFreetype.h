/*------------------------------------------------------
	
	binary font type which converts to a mesh asset -
	processed by freetype

-------------------------------------------------------*/
#pragma once

#include "TFile.h"

namespace TLFileSys
{
	class TFileFreetype;
};

namespace TLAsset
{
	class TFont;
	class TMesh;
}



//	freetype needs memcmp and wcscpy... though I'm sure it 
//	already includes string.h, we'll just have to do it 
//	ourselves.
#if defined(TL_TARGET_PC)
	#include <string.h>
#endif // PC

namespace TLFreetype
{
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#include FT_GLYPH_H
	#include FT_OUTLINE_H
}

//---------------------------------------------------------
//	binary font type which converts to a mesh asset - processed by freetype
//---------------------------------------------------------
class TLFileSys::TFileFreetype : public TLFileSys::TFile
{
public:
	TFileFreetype(TRefRef FileRef,TRefRef FileTypeRef);
	~TFileFreetype();

	virtual SyncBool	ExportAsset(TPtr<TLAsset::TAsset>& pAsset,TRefRef ExportAssetType);
	virtual void		GetSupportedExportAssetTypes(TArray<TRef>& SupportedTypes) const	{	SupportedTypes.Add( TRef_Static4(F,o,n,t) );	}

protected:
	Bool						InitExport(TPtr<TLAsset::TAsset>& pAsset);
	SyncBool					UpdateExport(TPtr<TLAsset::TAsset>& pAsset);
	void						ShutdownExport(Bool DeleteFont);

	Bool						VectoriseGlyph(TPtr<TLAsset::TMesh>& pMesh,const TLFreetype::FT_Outline& Outline);	//	returns FALSE if nothing generated from outline

	float						GetPointScale();		//	amount to scale points down to to be in relation to our engine units (about 1.f is a standard character height)

protected:
	TLFreetype::FT_Library		m_pLibrary;
	TLFreetype::FT_Face			m_pFace;
	u32							m_CharHeight;
	u32							m_NextGlyphIndex;
	u32							m_NextGlyphCharacter;
};

