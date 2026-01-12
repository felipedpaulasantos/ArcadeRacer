#include "ArcadeRacer/Public/DetailedPrintLibrary.h"

#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

static FString GetClassPrettyName(const UClass* Cls)
{
	return IsValid(Cls) ? Cls->GetName() : FString(TEXT("Unknown"));
}

FString UDetailedPrintLibrary::GetBestCallerName(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return TEXT("None");
	}

	// Se for um componente, preferir o dono (Actor)
	if (const UActorComponent* AsComp = Cast<UActorComponent>(WorldContextObject))
	{
		if (const AActor* Owner = AsComp->GetOwner())
		{
			return GetClassPrettyName(Owner->GetClass());
		}
		return GetClassPrettyName(AsComp->GetClass());
	}

	// Se for um Actor, usar a classe dele (Blueprint gerada ou C++)
	if (const AActor* AsActor = Cast<AActor>(WorldContextObject))
	{
		return GetClassPrettyName(AsActor->GetClass());
	}

	// Tentar subir na cadeia de Outer para achar algo mais “Blueprint-like” do que um objeto genérico
	for (const UObject* It = WorldContextObject; IsValid(It); It = It->GetOuter())
	{
		// Se encontramos um Actor no Outer, geralmente é um bom “callsite”
		if (const AActor* OuterActor = Cast<AActor>(It))
		{
			return GetClassPrettyName(OuterActor->GetClass());
		}
	}

	// Fallback: classe do próprio objeto de contexto
	return GetClassPrettyName(WorldContextObject->GetClass());
}

void UDetailedPrintLibrary::DetailedPrintString(
	const UObject* WorldContextObject,
	const FString& InString,
	bool bPrintToScreen,
	bool bPrintToLog,
	FLinearColor TextColor,
	float Duration,
	FName Key
)
{
	const FString Caller = GetBestCallerName(WorldContextObject);
	const FString Final = FString::Printf(TEXT("[%s] %s"), *Caller, *InString);

	UKismetSystemLibrary::PrintString(
		WorldContextObject,
		Final,
		bPrintToScreen,
		bPrintToLog,
		TextColor,
		Duration,
		Key
	);
}
