/** Created and owned by Furious Production LTD @ 2023. **/

#include "Networking/ArcadeVehicleNetSerialization.h"

FFloat_NetQuantize FFloat_NetQuantize::ZeroFloat = 0.f;

FFloat_NetQuantize::FFloat_NetQuantize()
	: Value(0.f)
{
}

FFloat_NetQuantize::FFloat_NetQuantize(const float& InValue)
	: Value(InValue)
{
}

bool FFloat_NetQuantize::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	int8 compressedFloat = 0;
	if(Ar.IsSaving())
	{
		compressedFloat = FMath::RoundToInt(Value * 127.f);
		Ar.SerializeBits(&compressedFloat, 8);
	}
	else
	{
		Ar.SerializeBits(&compressedFloat, 8);
		Value = compressedFloat / 127.f;
	}

	return bOutSuccess;
}
