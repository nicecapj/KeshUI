// Copyright 2014-2015 Matt Chapman. All Rights Reserved.

#include "KeshUI/KeshUI.h"
#include "KeshUI/KUIInterface.h"
#include "KeshUI/KUIInterfaceWidgetChildManager.h"
#include "KeshUI/KUIInterfaceContainer.h"
#include "KeshUI/KUIInterfaceWidget.h"
#define _XTGMATH
#include <functional>


UKUIInterfaceContainer::UKUIInterfaceContainer( const class FObjectInitializer& oObjectInitializer )
	: Super(oObjectInitializer)
{
	arChildren.SetNum( 0 );
	this->v2Size = FVector2D::ZeroVector;
	bValidLayout = false;
	bFocused = false;
	arChildManagers.SetNum( 0 );
	iTickRequests = 0;
	iMouseInputRequests = 0;
	iKeyInputRequests = 0;
}


const FVector2D& UKUIInterfaceContainer::GetSize() const
{
	return v2Size;
}


void UKUIInterfaceContainer::SetSizeStruct( const FVector2D& v2Size )
{
	SetSize( v2Size.X, v2Size.Y );
}


void UKUIInterfaceContainer::SetSize( float fWidth, float fHeight )
{
	if ( v2Size.X == fWidth && v2Size.Y == fHeight )
		return;

	v2Size.X = fWidth;
	v2Size.Y = fHeight;

	Super::SetSize( fWidth, fHeight );

	InvalidateRenderCache();
	InvalidateLayout();
}


bool UKUIInterfaceContainer::CanTick() const
{
	return false;
}


void UKUIInterfaceContainer::AddChild( UKUIInterfaceElement* oChild )
{
	// Can't add a null pointer.
	if ( oChild == NULL )
	{
		KUIErrorDebugUO( "Trying to add a null child" );
		return;
	}

	// Check if we're in the child array already.
	for ( int32 i = 0; i < arChildren.Num(); ++i )
	{
		if ( arChildren[ i ] != oChild )
			continue;

		KUIErrorUO( "Trying to add a child that is already a child" );
		return;
	}

	// If this child already has a container, remove it from it.
	if ( oChild->GetContainer() != NULL )
		oChild->GetContainer()->RemoveChild( oChild );

	// Add the child to our array.
	arChildren.Add( oChild );
	SortChildren();

	oChild->SetContainer( this );

	if ( oChild->IsA<UKUIInterfaceContainer>() )
	{
		UKUIInterfaceContainer* const ctContainer = Cast<UKUIInterfaceContainer>( oChild );

		AddTickRequests( ctContainer->GetTickRequests() + ( ctContainer->CanTick() ? 1 : 0 ) );
		AddMouseInputRequests( ctContainer->GetMouseInputRequests() + ( ctContainer->CanReceieveMouseEvents() ? 1 : 0 ) );
		AddKeyInputRequests( ctContainer->GetKeyInputRequests() + ( ctContainer->CanReceieveKeyEvents() ? 1 : 0 ) );
	}

	KUISendEvent( FKUIInterfaceContainerElementEvent, EKUIInterfaceContainerEventList::E_ChildAdded, oChild );

	InvalidateRenderCache();
}


bool UKUIInterfaceContainer::RemoveChild( UKUIInterfaceElement* oChild )
{
	// Can't remove a null pointer
	if ( oChild == NULL )
	{
		KUIErrorDebugUO( "Trying to remove a null child" );
		return false;
	}

	// Check if we're actually in the children array.
	int32 iIndex = INDEX_NONE;

	for ( int32 i = 0; i < arChildren.Num(); ++i )
	{
		if ( arChildren[ i ] != oChild )
			continue;

		iIndex = i;
		break;
	}

	// It wasn't
	if ( iIndex == INDEX_NONE )
	{
		KUIErrorDebugUO( "Trying to remove a child not in the child array" );
		return false;
	}

	// Sanity check, this should never be the case at this stage.
	if ( oChild->GetContainer() != this )
	{
		KUIErrorUO( "Trying to remove a child whose container is not this element" );
		return false;
	}

	// Swap out the highest index with this child, to maintain contiguity.
	int32 iHighestIndex = INDEX_NONE;

	for ( int32 i = arChildren.Num() - 1; i > iIndex; --i )
	{
		if ( arChildren[ i ] == NULL )
			continue;

		iHighestIndex = i;
		break;
	}

	// We found a highest child, so swap.  Otherwise, we were the highest child and no sort is necessary.
	if ( iHighestIndex != INDEX_NONE )
	{
		arChildren[ iIndex ] = arChildren[ iHighestIndex ];

		// Re-sort the array for z-index.	
		arChildren.SetNum( iHighestIndex );
		SortChildren();
	}

	// Reduce the size of the array by 1.
	else
		arChildren.SetNum( iIndex );

	oChild->SetContainer( NULL );

	// Run the child event to set up containers.
	if ( oChild->IsA<UKUIInterfaceContainer>() )
	{
		UKUIInterfaceContainer* const ctContainer = Cast<UKUIInterfaceContainer>( oChild );

		RemoveTickRequests( ctContainer->GetTickRequests() + ( ctContainer->CanTick() ? 1 : 0 ) );
		RemoveMouseInputRequests( ctContainer->GetMouseInputRequests() + ( ctContainer->CanReceieveMouseEvents() ? 1 : 0 ) );
		RemoveKeyInputRequests( ctContainer->GetKeyInputRequests() + ( ctContainer->CanReceieveKeyEvents() ? 1 : 0 ) );
	}

	KUISendEvent( FKUIInterfaceContainerElementEvent, EKUIInterfaceContainerEventList::E_ChildRemoved, oChild );
	InvalidateRenderCache();
	return true;
}


UKUIInterfaceElement* UKUIInterfaceContainer::GetChildAtIndex( int32 iIndex ) const
{
	if ( !arChildren.IsValidIndex( iIndex ) )
		return NULL;

	return arChildren[ iIndex ];
}


TArray<UKUIInterfaceElement*>::TIterator UKUIInterfaceContainer::GetChildIterator()
{
	return arChildren.CreateIterator();
}


void UKUIInterfaceContainer::SortChildren()
{
	if ( bDebug )
		bDebug = true;

	if ( arChildren.Num() == 0 )
		return;

	TArray<UKUIInterfaceElement*> arChildrenTemp;
	arChildrenTemp.SetNum( arChildren.Num() );

	for ( int32 i = 0; i < arChildren.Num(); ++i )
		arChildrenTemp[ i ] = arChildren[ i ];
	
	uint16 minZIndex = KUI_ZINDEX_NONE;
	uint16 lastZIndex = KUI_ZINDEX_NONE;
	int32 iCurrentIndex = 0;

	while ( true )
	{
		minZIndex = KUI_ZINDEX_NONE;

		for ( int32 i = 0; i < arChildrenTemp.Num(); ++i )
		{
			if ( arChildrenTemp[ i ] == NULL )
				continue;

			if ( lastZIndex != KUI_ZINDEX_NONE && arChildrenTemp[ i ]->GetZIndex() <= lastZIndex )
				continue;

			if ( minZIndex == KUI_ZINDEX_NONE || arChildrenTemp[ i ]->GetZIndex() < minZIndex )
				minZIndex = arChildrenTemp[ i ]->GetZIndex();
		}

		if ( minZIndex == KUI_ZINDEX_NONE )
			break;

		for ( int32 i = 0; i < arChildrenTemp.Num(); ++i )
		{
			if ( arChildrenTemp[ i ] == NULL )
				continue;

			if ( arChildrenTemp[ i ]->GetZIndex() != minZIndex )
				continue;

			arChildren[ iCurrentIndex ] = arChildrenTemp[ i ];
			++iCurrentIndex;
		}

		lastZIndex = minZIndex;
	}

	arChildren.SetNum( iCurrentIndex );	
}


bool UKUIInterfaceContainer::CanReceieveMouseEvents() const
{
	return false;
}


bool UKUIInterfaceContainer::CanReceieveKeyEvents() const
{
	return false;
}


void UKUIInterfaceContainer::SetVisible( bool bVisible )
{
	if ( this->bVisible == bVisible )
		return;

	Super::SetVisible( bVisible );

	KUIBroadcastEvent( FKUIInterfaceContainerVisibilityEvent, EKUIInterfaceContainerEventList::E_VisibilityChange, bVisible );
}


void UKUIInterfaceContainer::Render( AKUIInterface* aHud, UCanvas* oCanvas, const FVector2D& v2Origin, UKUIInterfaceElement* oRenderCacheObject )
{
	if ( !HasValidLayout() )
	{
		DoLayout();
		KUISendEvent( FKUIInterfaceEvent, EKUIInterfaceContainerEventList::E_LayoutComplete );
	}

	Super::Render( aHud, oCanvas, v2Origin, oRenderCacheObject );

	if ( IsRenderCaching() && oRenderCacheObject != this )
		return;

	const FVector2D v2RenderLocation = GetRenderLocation();

	for ( int32 i = 0; i < arChildren.Num(); ++i )
	{
		if ( arChildren[ i ] == NULL )
			continue;

		if ( !arChildren[ i ]->IsVisible() )
			continue;

		arChildren[ i ]->Render( aHud, oCanvas, v2Origin + v2RenderLocation, oRenderCacheObject );	
	}
}


void UKUIInterfaceContainer::OnChildSizeChange( const FKUIInterfaceContainerElementEvent& stEventInfo )
{
	// Not needed any more... but not a bad idea having this method here.
	//InvalidateLayout();
}


void UKUIInterfaceContainer::OnChildLocationChange( const FKUIInterfaceContainerElementEvent& stEventInfo )
{
	// Not needed any more.
	//InvalidateLayout();
}


bool UKUIInterfaceContainer::HasValidLayout() const
{
	return bValidLayout;
}


void UKUIInterfaceContainer::InvalidateLayout()
{
	bValidLayout = false;

	InvalidateAlignLocation();

	KUISendEvent( FKUIInterfaceEvent, EKUIInterfaceContainerEventList::E_LayoutInvalidated );
}


// Default class uses alignment and docking to do layout.
void UKUIInterfaceContainer::DoLayout()
{
	// Invalidate all the locations.
	for ( int32 i = 0; i < arChildren.Num(); ++i )
	{
		if ( arChildren[ i ] == NULL )
			continue;

		if ( IsChildsLayoutManaged( arChildren[ i ] ) )
			continue;

		arChildren[ i ]->InvalidateAlignLocation();
	}

	TArray<UKUIInterfaceElement*> arAlignStack;

	for ( int32 i = 0; i < arChildren.Num(); ++i )
	{
		if ( arChildren[ i ] == NULL )
			continue;

		if ( arChildren[ i ]->HasValidAlignLocation() )
			continue;

		if ( IsChildsLayoutManaged( arChildren[ i ] ) )
			continue;

		arChildren[ i ]->CalculateAlignLocation( arAlignStack );
	}

	bValidLayout = true;
}


void UKUIInterfaceContainer::OnTick( const FKUIInterfaceContainerTickEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnMouseMove( const FKUIInterfaceContainerMouseLocationEvent& stEventInfo )
{

}


bool UKUIInterfaceContainer::OnMouseButtonDown( const FKUIInterfaceContainerMouseButtonEvent& stEventInfo )
{
	return stEventInfo.bHandled;
}
	

bool UKUIInterfaceContainer::OnMouseButtonUp( const FKUIInterfaceContainerMouseButtonEvent& stEventInfo )
{
	return stEventInfo.bHandled;
}


bool UKUIInterfaceContainer::OnKeyDown( const FKUIInterfaceContainerKeyEvent& stEventInfo )
{
	return stEventInfo.bHandled;
}


bool UKUIInterfaceContainer::OnKeyUp( const FKUIInterfaceContainerKeyEvent& stEventInfo )
{
	return stEventInfo.bHandled;
}


bool UKUIInterfaceContainer::OnKeyRepeat( const FKUIInterfaceContainerKeyEvent& stEventInfo )
{
	return stEventInfo.bHandled;
}


bool UKUIInterfaceContainer::OnKeyChar( const FKUIInterfaceContainerCharEvent& stEventInfo )
{
	return stEventInfo.bHandled;
}


void UKUIInterfaceContainer::OnScreenResolutionChange( const FKUIInterfaceContainerScreenResolutionEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnMatchStart( const FKUIInterfaceEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnMatchEnd( const FKUIInterfaceEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnMatchPaused( const FKUIInterfaceContainerPlayerEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnMatchUnpaused( const FKUIInterfaceContainerPlayerEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnPlayerDeath( const FKUIInterfaceContainerPlayerEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnVisibilityChange( const FKUIInterfaceContainerVisibilityEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnFocus( const FKUIInterfaceEvent& stEventInfo )
{
	bFocused = true;
}


void UKUIInterfaceContainer::OnBlur( const FKUIInterfaceEvent& stEventInfo )
{
	bFocused = false;
}


void UKUIInterfaceContainer::OnFocusChange( const FKUIInterfaceContainerElementEvent& stEventInfo )
{

}


int16 UKUIInterfaceContainer::GetTickRequests() const
{
	return iTickRequests;
}


void UKUIInterfaceContainer::AddTickRequests( int16 iCount )
{
	if ( iCount == 0 )
		return;

	iTickRequests += iCount;

	if ( GetContainer() != NULL )
		GetContainer()->AddTickRequests( iCount );
}


void UKUIInterfaceContainer::RemoveTickRequests( int16 iCount )
{
	if ( iCount == 0 )
		return;

	iTickRequests -= iCount;

	if ( GetContainer() != NULL )
		GetContainer()->RemoveTickRequests( iCount );
}


int16 UKUIInterfaceContainer::GetMouseInputRequests() const
{
	return iMouseInputRequests;
}


void UKUIInterfaceContainer::AddMouseInputRequests( int16 iCount )
{
	if ( iCount == 0 )
		return;

	iMouseInputRequests += iCount;

	if ( GetContainer() != NULL )
		GetContainer()->AddMouseInputRequests( iCount );
}


void UKUIInterfaceContainer::RemoveMouseInputRequests( int16 iCount )
{
	if ( iCount == 0 )
		return;

	iMouseInputRequests -= iCount;

	if ( GetContainer() != NULL )
		GetContainer()->RemoveMouseInputRequests( iCount );
}


int16 UKUIInterfaceContainer::GetKeyInputRequests() const
{
	return iKeyInputRequests;
}


void UKUIInterfaceContainer::AddKeyInputRequests( int16 iCount )
{
	if ( iCount == 0 )
		return;

	iKeyInputRequests += iCount;

	if ( GetContainer() != NULL )
		GetContainer()->AddKeyInputRequests( iCount );
}


void UKUIInterfaceContainer::RemoveKeyInputRequests( int16 iCount )
{
	if ( iCount == 0 )
		return;

	iKeyInputRequests -= iCount;

	if ( GetContainer() != NULL )
		GetContainer()->RemoveKeyInputRequests( iCount );
}


void UKUIInterfaceContainer::AddChildManager( UKUIInterfaceWidgetChildManager* oChildManager )
{
	if ( arChildManagers.Contains( oChildManager ) )
		return;

	arChildManagers.Add( oChildManager );
	
	if ( !IsTemplate() )
		oChildManager->Update();
}


void UKUIInterfaceContainer::RemoveChildManager( UKUIInterfaceWidgetChildManager* oChildManager )
{
	arChildManagers.Remove( oChildManager );
}
		

void UKUIInterfaceContainer::UpdateChildManagers()
{
	for ( int32 i = 0; i < arChildManagers.Num(); ++i )
	{
		if ( arChildManagers[ i ] == NULL )
			continue;

		arChildManagers[ i ]->Update();
	}

	KUISendEvent( FKUIInterfaceEvent, EKUIInterfaceContainerEventList::E_ChildManagersUpdated );
}


bool UKUIInterfaceContainer::IsChildsLayoutManaged( UKUIInterfaceElement* oChild ) const
{
	return false;
}


void UKUIInterfaceContainer::OnMatchStateChange( const FKUIInterfaceContainerMatchStateEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnLayoutInvalidated( const FKUIInterfaceEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnLayoutComplete( const FKUIInterfaceEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnChildManagersUpdated( const FKUIInterfaceEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnChildAdded( const FKUIInterfaceContainerElementEvent& stEventInfo )
{

}


void UKUIInterfaceContainer::OnChildRemoved( const FKUIInterfaceContainerElementEvent& stEventInfo )
{

}


bool UKUIInterfaceContainer::RespondsToEvent( uint8 iEventID ) const
{
	if ( Super::RespondsToEvent( iEventID ) )
		return true;

	switch ( iEventID )
	{
		case EKUIInterfaceContainerEventList::E_Tick:
			if ( CanTick() )
				return true;

			if ( iTickRequests > 0 )
				return true;

			return false;

		case EKUIInterfaceContainerEventList::E_MouseMove:
		case EKUIInterfaceContainerEventList::E_MouseButtonDown:
		case EKUIInterfaceContainerEventList::E_MouseButtonUp:
			if ( !IsVisible() )
				return false;

			if ( CanReceieveMouseEvents() )
				return true;

			if ( iMouseInputRequests > 0 )
				return true;

			return false;

		case EKUIInterfaceContainerEventList::E_KeyDown:
		case EKUIInterfaceContainerEventList::E_KeyUp:
		case EKUIInterfaceContainerEventList::E_KeyRepeat:
			if ( !IsVisible() )
				return false;

			if ( CanReceieveKeyEvents() )
				return true;

			if ( iKeyInputRequests > 0 )
				return true;

			return false;

		default:
			return ( iEventID >= KUI_CONTAINER_EVENT_FIRST && iEventID <= KUI_CONTAINER_EVENT_LAST );
	}
}


void UKUIInterfaceContainer::SendEvent( FKUIInterfaceEvent& stEventInfo )
{
	if ( IsTemplate() )
		return;

	Super::SendEvent( stEventInfo );

	static TArray<std::function<void( UKUIInterfaceContainer*, FKUIInterfaceEvent& )>> arDispatchers;

	if ( arDispatchers.Num() == 0 )
	{
		arDispatchers.SetNum( KUI_CONTAINER_EVENT_LAST - KUI_CONTAINER_EVENT_FIRST + 1 );

// Can't use UStruct pointers... so this.
#pragma warning( disable : 4946 )

		arDispatchers[ EKUIInterfaceContainerEventList::E_Tick - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnTick( *reinterpret_cast<FKUIInterfaceContainerTickEvent*>( &stEventInfo ) );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MouseMove - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnMouseMove( *reinterpret_cast<FKUIInterfaceContainerMouseLocationEvent*>( &stEventInfo ) );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MouseButtonDown - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			FKUIInterfaceContainerMouseButtonEvent* stMouseEventInfo = reinterpret_cast<FKUIInterfaceContainerMouseButtonEvent*>( &stEventInfo );
			stMouseEventInfo->bHandled = oElement->OnMouseButtonDown( *stMouseEventInfo );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MouseButtonUp - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			FKUIInterfaceContainerMouseButtonEvent* stMouseEventInfo = reinterpret_cast<FKUIInterfaceContainerMouseButtonEvent*>( &stEventInfo );
			stMouseEventInfo->bHandled = oElement->OnMouseButtonUp( *stMouseEventInfo );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_KeyDown - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			FKUIInterfaceContainerKeyEvent* stKeyEventInfo = reinterpret_cast<FKUIInterfaceContainerKeyEvent*>( &stEventInfo );
			stKeyEventInfo->bHandled = oElement->OnKeyDown( *stKeyEventInfo );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_KeyUp - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			FKUIInterfaceContainerKeyEvent* stKeyEventInfo = reinterpret_cast<FKUIInterfaceContainerKeyEvent*>( &stEventInfo );
			stKeyEventInfo->bHandled = oElement->OnKeyUp( *stKeyEventInfo );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_KeyRepeat - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			FKUIInterfaceContainerKeyEvent* stKeyEventInfo = reinterpret_cast<FKUIInterfaceContainerKeyEvent*>( &stEventInfo );
			stKeyEventInfo->bHandled = oElement->OnKeyRepeat( *stKeyEventInfo );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_KeyChar - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			FKUIInterfaceContainerCharEvent* stCharEventInfo = reinterpret_cast<FKUIInterfaceContainerCharEvent*>( &stEventInfo );
			stCharEventInfo->bHandled = oElement->OnKeyChar( *stCharEventInfo );
			// No BP event
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_ScreenResolutionChange - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnScreenResolutionChange( *reinterpret_cast<FKUIInterfaceContainerScreenResolutionEvent*>( &stEventInfo ) );
			oElement->OnScreenResolutionChangeBP( *reinterpret_cast<FKUIInterfaceContainerScreenResolutionEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MatchStart - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnMatchStart( *reinterpret_cast<FKUIInterfaceEvent*>( &stEventInfo ) );
			oElement->OnMatchStartBP( *reinterpret_cast<FKUIInterfaceEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MatchPaused - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnMatchPaused( *reinterpret_cast<FKUIInterfaceContainerPlayerEvent*>( &stEventInfo ) );
			oElement->OnMatchPausedBP( *reinterpret_cast<FKUIInterfaceContainerPlayerEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MatchUnpaused - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnMatchUnpaused( *reinterpret_cast<FKUIInterfaceContainerPlayerEvent*>( &stEventInfo ) );
			oElement->OnMatchUnpausedBP( *reinterpret_cast<FKUIInterfaceContainerPlayerEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MatchEnd - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnMatchEnd( *reinterpret_cast<FKUIInterfaceEvent*>( &stEventInfo ) );
			oElement->OnMatchEndBP( *reinterpret_cast<FKUIInterfaceEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_PlayerDeath - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnPlayerDeath( *reinterpret_cast<FKUIInterfaceContainerPlayerEvent*>( &stEventInfo ) );
			oElement->OnPlayerDeathBP( *reinterpret_cast<FKUIInterfaceContainerPlayerEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_VisibilityChange - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnVisibilityChange( *reinterpret_cast<FKUIInterfaceContainerVisibilityEvent*>( &stEventInfo ) );
			oElement->OnVisibilityChangeBP( *reinterpret_cast<FKUIInterfaceContainerVisibilityEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_Focus - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnFocus( stEventInfo );
			oElement->OnFocusBP( stEventInfo );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_Blur - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnBlur( stEventInfo );
			oElement->OnBlurBP( stEventInfo );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_FocusChange - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnFocusChange( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
			oElement->OnFocusChangeBP( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_ChildSizeChange - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnChildSizeChange( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
			oElement->OnChildSizeChangeBP( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_ChildLocationChange - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnChildLocationChange( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
			oElement->OnChildLocationChangeBP( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_MatchStateChange - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnMatchStateChange( *reinterpret_cast<FKUIInterfaceContainerMatchStateEvent*>( &stEventInfo ) );
			oElement->OnMatchStateChangeBP( *reinterpret_cast<FKUIInterfaceContainerMatchStateEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_LayoutInvalidated - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnLayoutInvalidated( stEventInfo );
			oElement->OnLayoutInvalidatedBP( stEventInfo );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_LayoutComplete - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnLayoutComplete( stEventInfo );
			oElement->OnLayoutCompleteBP( stEventInfo );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_ChildManagersUpdated - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnChildManagersUpdated( stEventInfo );
			oElement->OnChildManagersUpdatedBP( stEventInfo );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_ChildAdded - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnChildAdded( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
			oElement->OnChildAddedBP( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
		};

		arDispatchers[ EKUIInterfaceContainerEventList::E_ChildRemoved - KUI_CONTAINER_EVENT_FIRST ] =
			[] ( UKUIInterfaceContainer* oElement, FKUIInterfaceEvent& stEventInfo )
		{
			oElement->OnChildRemoved( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
			oElement->OnChildRemovedBP( *reinterpret_cast<FKUIInterfaceContainerElementEvent*>( &stEventInfo ) );
		};

#pragma warning( default : 4946 )

	}

	if ( stEventInfo.iEventID >= KUI_CONTAINER_EVENT_FIRST && stEventInfo.iEventID <= KUI_CONTAINER_EVENT_LAST )
		arDispatchers[ stEventInfo.iEventID - KUI_CONTAINER_EVENT_FIRST ]( this, stEventInfo );
}


void UKUIInterfaceContainer::BroadcastEvent( FKUIInterfaceEvent& stEventInfo, bool bTopDown )
{
	//if ( bDebug && stEventInfo.iEventID != EKUIInterfaceContainerEventList::E_Tick && stEventInfo.iEventID != EKUIInterfaceContainerEventList::E_MouseMove )
		//bDebug = true;

	bool bResponds = RespondsToEvent( stEventInfo.iEventID );

	if ( !bResponds )
	{
		switch ( stEventInfo.iEventID )
		{
			case EKUIInterfaceContainerEventList::E_Tick:
			case EKUIInterfaceContainerEventList::E_MouseMove:
			case EKUIInterfaceContainerEventList::E_MouseButtonDown:
			case EKUIInterfaceContainerEventList::E_MouseButtonUp:
			case EKUIInterfaceContainerEventList::E_KeyDown:
			case EKUIInterfaceContainerEventList::E_KeyUp:
			case EKUIInterfaceContainerEventList::E_KeyRepeat:
			case EKUIInterfaceContainerEventList::E_KeyChar:
				return;
		}
	}

	if ( !bTopDown )
	{
		if ( bResponds )
			SendEvent( stEventInfo );

		for ( int32 i = 0; i < arChildren.Num(); ++i )
		{
			UKUIInterfaceElement* ctChild = Cast<UKUIInterfaceElement>( arChildren[ i ] );

			if ( ctChild == NULL )
				continue;

			UKUIInterfaceContainer* ctChildContainer = Cast<UKUIInterfaceContainer>( arChildren[ i ] );

			if ( ctChildContainer == NULL )
				ctChild->SendEvent( stEventInfo );

			else
				ctChildContainer->BroadcastEvent( stEventInfo, bTopDown );
		}
	}

	else
	{
		for ( int32 i = arChildren.Num() - 1; i >= 0; --i )
		{
			UKUIInterfaceElement* ctChild = Cast<UKUIInterfaceElement>( arChildren[ i ] );

			if ( ctChild == NULL )
				continue;

			UKUIInterfaceContainer* ctChildContainer = Cast<UKUIInterfaceContainer>( arChildren[ i ] );

			if ( ctChildContainer == NULL )
				ctChild->SendEvent( stEventInfo );

			else
				ctChildContainer->BroadcastEvent( stEventInfo, bTopDown );
		}

		if ( bResponds )
			SendEvent( stEventInfo );
	}
}
