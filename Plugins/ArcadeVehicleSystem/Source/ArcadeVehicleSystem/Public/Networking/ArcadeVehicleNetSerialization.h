/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "ArcadeVehicleNetSerialization.generated.h"

/**
 * 8 bit float compression for networking.
 * 0 decimal places precision.
 */
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FFloat_NetQuantize
{
	GENERATED_USTRUCT_BODY()

	FFloat_NetQuantize();
	FFloat_NetQuantize(const float& InValue);
	
	FFloat_NetQuantize& operator=(const float& rhs)
	{
		Value = rhs;
		return *this;
	}

	bool operator==(const float& rhs) const
	{
		return Value == rhs;
	}

	bool operator!=(const float& rhs) const
	{
		return Value != rhs;
	}

	FFloat_NetQuantize operator*(const float& rhs)
	{
		FFloat_NetQuantize newFloat;
		newFloat.Value = Value * rhs;
		return newFloat;
	}
	
	FFloat_NetQuantize& operator*=(const float& rhs)
	{
		Value += rhs;
		return *this;
	}

	float ToFloat() const
	{
		return Value;
	}
	
	/** Method for serializing the bits of this structure. */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/** Zero static value. */
	static FFloat_NetQuantize ZeroFloat;
	
protected:
	/** Underlying float value. */
	UPROPERTY(BlueprintReadOnly, Category=Networking)
	float Value;
};

/** Serialization functionality for net quantized float. */
template<>
struct TStructOpsTypeTraits<FFloat_NetQuantize> : public TStructOpsTypeTraitsBase2<FFloat_NetQuantize>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

FORCEINLINE FVector2D ClampVector2D(const FVector2D& V, const FVector2D& Min, const FVector2D& Max)
{
	return FVector2D(
		FMath::Clamp(V.X,Min.X,Max.X),
		FMath::Clamp(V.Y,Min.Y,Max.Y)
		);
}

template<int32 ScaleFactor, int32 MaxBitsPerComponent>
bool WritePackedVector2D(FVector2D Value, FArchive& Ar)	// Note Value is intended to not be a reference since we are scaling it before serializing!
{
	check(Ar.IsSaving());

	// Scale vector by quant factor first
	Value *= ScaleFactor;

	// Nan Check
	if( Value.ContainsNaN() )
	{
		logOrEnsureNanError(TEXT("WritePackedVector2D: Value contains NaN, clearing for safety."));
		FVector2D Dummy(0, 0);
		WritePackedVector2D<ScaleFactor, MaxBitsPerComponent>(Dummy, Ar);
		return false;
	}

	// Some platforms have RoundToInt implementations that essentially reduces the allowed inputs to 2^31.
	const FVector2D ClampedValue = ClampVector2D(Value, FVector2D(-1073741824.0f), FVector2D(1073741760.0f));
	bool bClamp = ClampedValue != Value;

	// Do basically FVector2D::SerializeCompressed
	int32 IntX	= FMath::RoundToInt(ClampedValue.X);
	int32 IntY	= FMath::RoundToInt(ClampedValue.Y);
			
	uint32 Bits	= FMath::Clamp<uint32>( FMath::CeilLogTwo( 1 + FMath::Max( FMath::Abs(IntX), FMath::Abs(IntY) ) ), 1, MaxBitsPerComponent ) - 1;

	// Serialize how many bits each component will have
	Ar.SerializeInt( Bits, MaxBitsPerComponent );

	int32  Bias	= 1<<(Bits+1);
	uint32 Max	= 1<<(Bits+2);
	uint32 DX	= IntX + Bias;
	uint32 DY	= IntY + Bias;

	if (DX >= Max) { bClamp=true; DX = static_cast<int32>(DX) > 0 ? Max-1 : 0; }
	if (DY >= Max) { bClamp=true; DY = static_cast<int32>(DY) > 0 ? Max-1 : 0; }
	
	Ar.SerializeInt( DX, Max );
	Ar.SerializeInt( DY, Max );

	return !bClamp;
}

template<uint32 ScaleFactor, int32 MaxBitsPerComponent>
bool ReadPackedVector2D(FVector2D &Value, FArchive& Ar)
{
	uint32 Bits	= 0;

	// Serialize how many bits each component will have
	Ar.SerializeInt( Bits, MaxBitsPerComponent );

	int32  Bias = 1<<(Bits+1);
	uint32 Max	= 1<<(Bits+2);
	uint32 DX	= 0;
	uint32 DY	= 0;
	
	Ar.SerializeInt( DX, Max );
	Ar.SerializeInt( DY, Max );
	
	
	float fact = (float)ScaleFactor;

	Value.X = (float)(static_cast<int32>(DX)-Bias) / fact;
	Value.Y = (float)(static_cast<int32>(DY)-Bias) / fact;

	return true;
}

/**
 * Method of packing 2-dimensional vector.
 */
template<uint32 ScaleFactor, int32 MaxBitsPerComponent>
bool SerializePackedVector2D(FVector2D &Vector, FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		return  WritePackedVector2D<ScaleFactor, MaxBitsPerComponent>(Vector, Ar);
	}

	ReadPackedVector2D<ScaleFactor, MaxBitsPerComponent>(Vector, Ar);
	return true;
}

/**
  * 0 decimal place of precision.
  * Up to 20 bits per component.
  * Valid range: 2^20 = +/- 1,048,576
 */
USTRUCT()
struct FVector2D_NetQuantize : public FVector2D
{
	GENERATED_USTRUCT_BODY()

	FORCEINLINE FVector2D_NetQuantize()
	{}

	explicit FORCEINLINE FVector2D_NetQuantize(EForceInit E)
	: FVector2D(E)
	{}

	FORCEINLINE FVector2D_NetQuantize( float InX, float InY)
	: FVector2D(InX, InY)
	{}

	FORCEINLINE FVector2D_NetQuantize(const FVector2D &InVec)
	{
		FVector2D::operator=(InVec);
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = SerializePackedVector2D<1, 20>(*this, Ar);
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FVector2D_NetQuantize> : public TStructOpsTypeTraitsBase2<FVector2D_NetQuantize>
{
	enum 
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};