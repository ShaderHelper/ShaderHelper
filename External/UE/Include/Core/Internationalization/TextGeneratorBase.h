// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "ITextGenerator.h"

/**
 * Base class implementation for ITextGenerator.
 */
class CORE_API FTextGeneratorBase : public ITextGenerator
{
public: // Serialization
	/**
	 * Gets the type ID of this generator. The type ID is used to reconstruct this type for serialization and must be registered with FText::RegisterTextGenerator().
	 */
	virtual FName GetTypeID() const override;

	/**
	 * Serializes this generator.
	 */
	virtual void Serialize(FStructuredArchive::FRecord Record) override;
};
