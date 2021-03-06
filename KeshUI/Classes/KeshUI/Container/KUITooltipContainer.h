// Copyright 2014-2015 Matt Chapman. All Rights Reserved.

#pragma once

#include "KeshUI/Container/KUIRootContainer.h"
#include "KeshUI/KUIMacros.h"
#include "KUITooltipContainer.generated.h"

class AKUIInterface;

/**
 * The container used for cursors.
 */
UCLASS(ClassGroup="KeshUI|Container", BlueprintType, Blueprintable)
class KESHUI_API UKUITooltipContainer : public UKUIRootContainer
{
	GENERATED_BODY()
	KUI_CLASS_HEADER( UKUITooltipContainer )

	UKUITooltipContainer( const class FObjectInitializer& oObjectInitializer );

public:

	/* Overridden to false. */
	virtual bool CanReceieveKeyEvents() const;

};
