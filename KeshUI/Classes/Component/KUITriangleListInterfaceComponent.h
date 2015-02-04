// Copyright 2014-2015 Matt Chapman. All Rights Reserved.

#pragma once

#include "Component/KUICanvasItemInterfaceComponent.h"
#include "KUIMacros.h"
#include "KUITriangleListInterfaceComponent.generated.h"

/**
 * KeshUI UI Framework (KUI) TriangleList render component.
 */
UCLASS(ClassGroup="KeshUI | Component", Blueprintable, BlueprintType)
class KESHUI_API UKUITriangleListInterfaceComponent : public UKUICanvasItemInterfaceComponent
{
	GENERATED_UCLASS_BODY()
	KUI_CLASS_HEADER( UKUITriangleListInterfaceComponent )

public:

	/* Gets the current number of triangles. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual int16 GetTriangleCount() const;

	/* Gets the array capacity. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual int16 GetArrayCapacity() const;

	/* Sets the array capacity. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual void SetArrayCapacity( int16 iCapacity );

	/* Adds a triangle. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual void AddTriangleStruct( const FCanvasUVTri& stTriangle );

	/* Adds a triangle. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual void AddTriangle( FVector2D v2ScreenPointA, FVector2D v2ScreenPointB, FVector2D v2ScreenPointC, 
		FVector2D v2TexturePointA = FVector2D::ZeroVector, 
		FVector2D v2TexturePointB = FVector2D::ZeroVector, 
		FVector2D v2TexturePointC = FVector2D::ZeroVector,
		FLinearColor lcPointA = FLinearColor::White,
		FLinearColor lcPointB = FLinearColor::White,
		FLinearColor lcPointC = FLinearColor::White );

	/* Gets the texture. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual UTexture* GetTexture() const;

	/* Sets the texture. Sets the screen size to match this if it's zero. */
	UFUNCTION(Category="KeshUI | Component | Triangle List", BlueprintCallable)
	virtual void SetTexture( UTexture* tTexture );

	/* Gets the size of this component. */
	virtual FVector2D GetSize() const override;

	/* Returns true if this can have its location changed. */
	virtual bool IsPositionable() const override;

	/* Returns true if there's enough information to render. */
	virtual bool HasValidComponents() const override;

protected:

	int16 iTriangleCount;
	FVector4 v4Bounds;
	TArray<FCanvasUVTri> arTriangles;

	UPROPERTY() // Ensures reference count
	UTexture* tTexture;

	/* Tries to construct a new FCanvasTriangleListItem! */
	virtual void ConstructNewItem() override;

};
