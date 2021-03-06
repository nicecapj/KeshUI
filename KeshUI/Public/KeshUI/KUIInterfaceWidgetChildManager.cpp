// Copyright 2014-2015 Matt Chapman. All Rights Reserved.

#include "KeshUI/KeshUI.h"
#include "KeshUI/KUIInterfaceElement.h"
#include "KeshUI/KUIInterfaceWidget.h"
#include "KeshUI/KUIInterfaceWidgetChildManager.h"


UKUIInterfaceWidgetChildManager::UKUIInterfaceWidgetChildManager( const class FObjectInitializer& oObjectInitializer )
	: Super(oObjectInitializer)
{
	KUI_UO_INIT_DEBUG()

	cmWidget = NULL;
	oChild = NULL;
}


UKUIInterfaceWidget* UKUIInterfaceWidgetChildManager::GetWidget() const
{
	return cmWidget.Get();
}


void UKUIInterfaceWidgetChildManager::SetWidget( UKUIInterfaceWidget* cmWidget )
{
	this->cmWidget = cmWidget;
}


UKUIInterfaceElement* UKUIInterfaceWidgetChildManager::GetChild() const
{
	return oChild.Get();
}


void UKUIInterfaceWidgetChildManager::SetChild( UKUIInterfaceElement* oChild )
{
	this->oChild = oChild;
}


void UKUIInterfaceWidgetChildManager::Update()
{

}
