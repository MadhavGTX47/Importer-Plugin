// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MyFactory.generated.h"

/**
 * 
 */
UCLASS()
class BUCKY_API UMyFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:

	//UMyFactory::UMyFactory(const FObjectInitializer& ObjectInitializer);

	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename,
		const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
};
