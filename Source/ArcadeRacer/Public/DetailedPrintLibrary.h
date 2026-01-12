#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DetailedPrintLibrary.generated.h"

UCLASS()
class ARCADERACER_API UDetailedPrintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Igual ao Print String, mas adiciona prefixo com o nome da Blueprint/Classe que originou a chamada. */
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject", CallableWithoutWorldContext, AdvancedDisplay="2", DevelopmentOnly), Category="Utilities|Debug")
	static void DetailedPrintString(
		const UObject* WorldContextObject,
		const FString& InString = TEXT("Hello"),
		bool bPrintToScreen = true,
		bool bPrintToLog = true,
		FLinearColor TextColor = FLinearColor(0.0f, 0.66f, 1.0f),
		float Duration = 2.0f,
		FName Key = NAME_None
	);

private:
	static FString GetBestCallerName(const UObject* WorldContextObject);
};
