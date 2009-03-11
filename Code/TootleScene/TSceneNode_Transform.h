
#pragma once

#include <TootleCore/TLMaths.h>

#include "TSceneNode.h"

namespace TLScene
{
		class TSceneNode_Transform;
};

/*
	TSceneNode_Transform class
*/
class TLScene::TSceneNode_Transform : public TLScene::TSceneNode
{
public:
	TSceneNode_Transform(TRefRef NodeRef,TRefRef TypeRef) :
	  TSceneNode(NodeRef,TypeRef)
	{
	}

	virtual void				Initialise(TLMessaging::TMessage& Message);
	virtual Bool				HasTransform()	{ return TRUE; }

	FORCEINLINE float3			GetPosition() const												{	float3 Pos( 0,0,0 );	GetPosition( Pos );	return Pos;	}
	FORCEINLINE void			GetPosition(float3& Pos) const									{	GetTransform().TransformVector( Pos );	}	//	you can use this to get a relative offset from this node by initialising Pos to the [local] offset you want

	FORCEINLINE	const float3&	GetTranslate() const											{	return m_Transform.GetTranslate();	}
	FORCEINLINE void			SetTranslate(const float3& vPos)								{	m_Transform.SetTranslate(vPos);	OnTranslationChanged();	}

	FORCEINLINE	const TLMaths::TQuaternion&		GetRotation() const								{	return m_Transform.GetRotation();	}
	FORCEINLINE void							SetRotation(const TLMaths::TQuaternion& qRot)	{	m_Transform.SetRotation(qRot);	OnRotationChanged();	}

	FORCEINLINE	const float3&	GetScale() const												{	return m_Transform.GetScale();	}
	FORCEINLINE void			SetScale(const float3& vScale)									{	m_Transform.SetScale(vScale);	OnScaleChanged();	}

	const TLMaths::TTransform&	GetTransform() const								{	return m_Transform;	}
	void						SetTransform(const TLMaths::TTransform& Transform)	{	m_Transform = Transform; OnTransformChanged(TRUE, TRUE, TRUE); }	

	// Distance checks
	virtual float				GetDistanceTo(const TLMaths::TLine& Line);			//	gr: note, this returns SQUARED distance! bad function naming!

protected:
	virtual void				ProcessMessage(TLMessaging::TMessage& Message);

	virtual void				Translate(float3 vTranslation);
	//virtual void				Rotate(float3 vRotation);
	//virtual void				Scale(float3 vScale);
	TLMaths::TTransform&		GetTransform() 							{	return m_Transform;	}

	FORCEINLINE void			OnTranslationChanged()		{ OnTransformChanged(TRUE, FALSE, FALSE); }
	FORCEINLINE void			OnRotationChanged()			{ OnTransformChanged(FALSE, TRUE, FALSE); }
	FORCEINLINE void			OnScaleChanged()			{ OnTransformChanged(FALSE, FALSE, TRUE); }

	void						OnTransformChanged(Bool bTranslation, Bool bRotation, Bool bScale);

private:
	TLMaths::TTransform			m_Transform;
};

