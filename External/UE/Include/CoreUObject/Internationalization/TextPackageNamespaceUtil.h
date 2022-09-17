// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Internationalization/TextNamespaceUtil.h"

namespace TextNamespaceUtil
{

#if USE_STABLE_LOCALIZATION_KEYS

/**
 * Given a package, try and get the namespace it should use for localization.
 *
 * @param InPackage			The package to try and get the namespace for.
 *
 * @return The package namespace, or an empty string if the package has no namespace.
 */
COREUOBJECT_API FString GetPackageNamespace(const UPackage* InPackage);

/**
 * Given an object, try and get the namespace it should use for localization (from its owner package).
 *
 * @param InObject			The object to try and get the namespace for.
 *
 * @return The package namespace, or an empty string if the package has no namespace.
 */
COREUOBJECT_API FString GetPackageNamespace(const UObject* InObject);

/**
 * Given a package, try and ensure it has a namespace it should use for localization.
 *
 * @param InPackage			The package to try and get the namespace for.
 *
 * @return The package namespace, or an empty string if the package has no namespace and one could not be added.
 */
COREUOBJECT_API FString EnsurePackageNamespace(UPackage* InPackage);

/**
 * Given an object, try and ensure it has a namespace it should use for localization (from its owner package).
 *
 * @param InObject			The object to try and get the namespace for.
 *
 * @return The package namespace, or an empty string if the package has no namespace and one could not be added.
 */
COREUOBJECT_API FString EnsurePackageNamespace(UObject* InObject);

/**
 * Given a package, clear any namespace it has set for localization.
 *
 * @param InPackage			The package to clear the namespace for.
 */
COREUOBJECT_API void ClearPackageNamespace(UPackage* InPackage);

/**
 * Given an object, clear any namespace it has set for localization (from its owner package).
 *
 * @param InObject			The object to clear the namespace for.
 */
COREUOBJECT_API void ClearPackageNamespace(UObject* InObject);

/**
 * Given a package, force it to have the given namespace for localization (even if a transient package!).
 *
 * @param InPackage			The package to set the namespace for.
 * @param InNamespace		The namespace to set.
 */
COREUOBJECT_API void ForcePackageNamespace(UPackage* InPackage, const FString& InNamespace);

/**
 * Given an object, force it to have the given namespace for localization (from its owner package, even if a transient package!).
 *
 * @param InObject			The object to set the namespace for.
 * @param InNamespace		The namespace to set.
 */
COREUOBJECT_API void ForcePackageNamespace(UObject* InObject, const FString& InNamespace);

#endif // USE_STABLE_LOCALIZATION_KEYS

/**
 * Make a copy of the given text that's valid to use with the given package, optionally preserving its existing key.
 * @note Returns the result verbatim if there is no change when applying the package namespace to the text.
 *
 * @param InText						The current FText instance.
 * @param InPackage/InObject			The package (or object to get the owner package from) to get the namespace for (will call EnsurePackageNamespace).
 * @param InCopyMethod					The method that should be used to copy the FText instance.
 * @param bAlwaysApplyPackageNamespace	If true, this will always apply the package namespace to the text namespace (always treated as Verbatim when USE_STABLE_LOCALIZATION_KEYS is false).
 *										If false, this will only apply the package namespace if the text namespace already contains package namespace makers.
 *
 * @return A copy of the given text that's valid to use with the given package.
 */
COREUOBJECT_API FText CopyTextToPackage(const FText& InText, UPackage* InPackage, const ETextCopyMethod InCopyMethod = ETextCopyMethod::NewKey, const bool bAlwaysApplyPackageNamespace = false);
COREUOBJECT_API FText CopyTextToPackage(const FText& InText, UObject* InObject, const ETextCopyMethod InCopyMethod = ETextCopyMethod::NewKey, const bool bAlwaysApplyPackageNamespace = false);

}
