// Copyright 2014-2015 Matt Chapman. All Rights Reserved.


#include "KeshUI.h"
#include "Widget/KUISelectWidget.h"


UKUISelectWidget::UKUISelectWidget( const class FObjectInitializer& oObjectInitializer )
: Super( oObjectInitializer )
{
	iSelectedValue = KUI_SELECT_WIDGET_NO_VALUE;
	arValues.SetNum( 0 );
	dgSelectValueChangeDelegate.BindUObject( this, &UKUISelectWidget::OnSliderValueChange );
}


bool UKUISelectWidget::HasValue() const
{
	return arValues.IsValidIndex( iSelectedValue );
}


FText UKUISelectWidget::GetValue() const
{
	if ( arValues.IsValidIndex( iSelectedValue ) )
		return arValues[ iSelectedValue ];

	return FText::GetEmpty();
}


uint8 UKUISelectWidget::GetSelectedIndex() const
{
	return iSelectedValue;
}


bool UKUISelectWidget::SetValueByIndex( uint8 iIndex )
{
	if ( iIndex == KUI_SELECT_WIDGET_NO_VALUE || arValues.IsValidIndex( iIndex ) )
	{
		iSelectedValue = iIndex;
		KUISendEvent( FKUIInterfaceEvent, EKUIInterfaceWidgetEventList::E_ValueChange );
		return true;
	}

	else if ( !arValues.IsValidIndex( iIndex ) )
		KUIErrorUO( "Invalid index: %d", iIndex );

	return false;
}


bool UKUISelectWidget::SetValueByValue( const FText& txValue )
{
	return SetValueByIndex( GetIndexByValue( txValue ) );
}


uint8 UKUISelectWidget::AddValue( const FText& txValue )
{
	if ( arValues.Num() == KUI_SELECT_WIDGET_MAX_VALUE )
		return KUI_SELECT_WIDGET_NO_VALUE;

	arValues.Add( txValue );

	return arValues.Num() - 1;
}


bool UKUISelectWidget::RemoveValueByIndex( uint8 iIndex )
{
	if ( !arValues.IsValidIndex( iIndex ) )
		return false;

	for ( uint8 i = iIndex; i < ( arValues.Num() - 1 ); ++i )
		arValues[ i ] = arValues[ i + 1 ];

	arValues.SetNum( arValues.Num() - 1 );

	if ( iSelectedValue == arValues.Num() )
		SetValueByIndex( KUI_SELECT_WIDGET_NO_VALUE );

	return true;
}


bool UKUISelectWidget::RemoveValueByValue( const FText& txValue )
{
	return RemoveValueByIndex( GetIndexByValue( txValue ) );
}


uint8 UKUISelectWidget::GetIndexOf( const FText& txValue ) const
{
	for ( uint8 i = 0; i < arValues.Num(); ++i )
		if ( arValues[ i ].EqualTo( txValue ) )
			return i;

	return KUI_SELECT_WIDGET_NO_VALUE;
}


bool UKUISelectWidget::NextValue()
{
	if ( iSelectedValue == KUI_SELECT_WIDGET_NO_VALUE )
		return SetValueByIndex( 0 );

	return SetValueByIndex( iSelectedValue + 1 );
}


bool UKUISelectWidget::PreviousValue()
{
	return SetValueByIndex( iSelectedValue - 1 );
}


uint8 UKUISelectWidget::GetIndexByValue( const FText& txValue ) const
{
	for ( uint8 i = 0; i < arValues.Num(); ++i )
		if ( arValues[ i ].EqualTo( txValue ) )
			return i;

	return KUI_SELECT_WIDGET_NO_VALUE;
}


void UKUISelectWidget::SetValueChangeDelegate( UObject* oObject, FKUISelectValueChangePrototype fnValueChangeCallback )
{
	if ( oObject == NULL || fnValueChangeCallback == NULL )
		dgSelectValueChangeDelegate.Unbind();

	else
		dgSelectValueChangeDelegate.BindUObject( oObject, fnValueChangeCallback );
}


void UKUISelectWidget::OnSliderValueChange( UKUISelectWidget* cmSelect )
{
	KUILogDebugUO( "Changed Value!: %d %s", iSelectedValue, *( GetValue().ToString() ) );
}


bool UKUISelectWidget::RespondsToEvent( uint8 iEventID ) const
{
	if ( Super::RespondsToEvent( iEventID ) )
		return true;

	return ( iEventID == EKUIInterfaceWidgetEventList::E_ValueChange );
}


void UKUISelectWidget::SendEvent( FKUIInterfaceEvent& stEventInfo )
{
	if ( IsTemplate() )
		return;

	Super::SendEvent( stEventInfo );

	if ( stEventInfo.iEventID == EKUIInterfaceWidgetEventList::E_ValueChange )
	{
		dgSelectValueChangeDelegate.ExecuteIfBound( this );
		OnSelectValueStateChangeBP( this );
	}
}
