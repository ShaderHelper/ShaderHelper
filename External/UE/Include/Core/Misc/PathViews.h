// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/StringFwd.h"

class FName;
class FString;
template <typename FuncType> class TFunctionRef;

class CORE_API FPathViews
{
public:
	/**
	 * Returns the portion of the path after the last separator.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B/C.D" -> "C.D"
	 * "A/B/C"   -> "C"
	 * "A/B/"    -> ""
	 * "A"       -> "A"
	 *
	 * @return The portion of the path after the last separator.
	 */
	static FStringView GetCleanFilename(const FStringView& InPath);

	/**
	 * Returns the portion of the path after the last separator and before the last dot.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B/C.D" -> "C"
	 * "A/B/C"   -> "C"
	 * "A/B/"    -> ""
	 * "A"       -> "A"
	 *
	 * @return The portion of the path after the last separator and before the last dot.
	 */
	static FStringView GetBaseFilename(const FStringView& InPath);

	/**
	 * Returns the portion of the path before the last dot.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B/C.D" -> "A/B/C"
	 * "A/B/C"   -> "A/B/C"
	 * "A/B/"    -> "A/B/"
	 * "A"       -> "A"
	 *
	 * @return The portion of the path before the last dot.
	 */
	static FStringView GetBaseFilenameWithPath(const FStringView& InPath);

	/** Returns the portion of the path before the last dot and optionally after the last separator. */
	UE_DEPRECATED(4.25, "FPathViews::GetBaseFilename(InPath, bRemovePath) has been superseded by "
		"FPathViews::GetBaseFilename(InPath) and FPathViews::GetBaseFilenameWithPath(InPath).")
	static FStringView GetBaseFilename(const FStringView& InPath, bool bRemovePath);

	/**
	 * Returns the portion of the path before the last separator.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B/C.D" -> "A/B"
	 * "A/B/C"   -> "A/B"
	 * "A/B/"    -> "A/B"
	 * "A"       -> ""
	 *
	 * @return The portion of the path before the last separator.
	 */
	static FStringView GetPath(const FStringView& InPath);

	/**
	 * Returns the portion of the path after the last dot following the last separator, optionally including the dot.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B.C.D" -> "D" (bIncludeDot=false) or ".D" (bIncludeDot=true)
	 * "A/B/C.D" -> "D" (bIncludeDot=false) or ".D" (bIncludeDot=true)
	 * "A/B/.D"  -> "D" (bIncludeDot=false) or ".D" (bIncludeDot=true)
	 * ".D"      -> "D" (bIncludeDot=false) or ".D" (bIncludeDot=true)
	 * "A/B/C"   -> ""
	 * "A.B/C"   -> ""
	 * "A.B/"    -> ""
	 * "A"       -> ""
	 *
	 * @param bIncludeDot Whether to include the leading dot in the returned view.
	 *
	 * @return The portion of the path after the last dot following the last separator, optionally including the dot.
	 */
	static FStringView GetExtension(const FStringView& InPath, bool bIncludeDot=false);

	/**
	 * Returns the last non-empty path component.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B/C.D" -> "C.D"
	 * "A/B/C"   -> "C"
	 * "A/B/"    -> "B"
	 * "A"       -> "A"
	 *
	 * @return The last non-empty path component.
	 */
	static FStringView GetPathLeaf(const FStringView& InPath);

	/**
	 * Return whether the given relative or absolute path is a leaf path - has no separators.
	 * 
	 * Examples: (Using '/' but '\' functions the same way)
	 * A		-> true
	 * A/		-> true
	 * D:/		-> true
	 * /		-> true
	 * //		-> true
	 * A/B		-> false
	 * D:/A		-> false
	 * //A		-> false
	 */
	static bool IsPathLeaf(FStringView InPath);

	/**
	 * Splits InPath into individual directory components, and calls ComponentVisitor on each.
	 *
	 * Examples:
	 * "A/B.C" -> {"A", "B.C"}
	 * "A/B/C" -> {"A", "B", "C"}
	 * "../../A/B/C.D" -> {"..", "..", "A", "B", "C.D" }
	 */
	static void IterateComponents(FStringView InPath, TFunctionRef<void(FStringView)> ComponentVisitor);

	/**
	 * Splits a path into three parts, any of which may be empty: the path, the clean name, and the extension.
	 *
	 * Examples: (Using '/' but '\' is valid too.)
	 * "A/B/C.D" -> ("A/B", "C",   "D")
	 * "A/B/C"   -> ("A/B", "C",   "")
	 * "A/B/"    -> ("A/B", "",    "")
	 * "A/B/.D"  -> ("A/B", "",    "D")
	 * "A/B.C.D" -> ("A",   "B.C", "D")
	 * "A"       -> ("",    "A",   "")
	 * "A.D"     -> ("",    "A",   "D")
	 * ".D"      -> ("",    "",    "D")
	 *
	 * @param OutPath [out] Receives the path portion of the input string, excluding the trailing separator.
	 * @param OutName [out] Receives the name portion of the input string.
	 * @param OutExt  [out] Receives the extension portion of the input string, excluding the dot.
	 */
	static void Split(const FStringView& InPath, FStringView& OutPath, FStringView& OutName, FStringView& OutExt);

	/**
	 * Appends each suffix argument to the path in the builder and ensures that there is a separator between them.
	 *
	 * Examples:
	 * ("",    "")    -> ""
	 * ("A",   "")    -> "A/"
	 * ("",    "B")   -> "B"
	 * ("/",   "B")   -> "/B"
	 * ("A",   "B")   -> "A/B"
	 * ("A/",  "B")   -> "A/B"
	 * ("A\\", "B")   -> "A\\B"
	 * ("A/B", "C/D") -> "A/B/C/D"
	 * ("A/", "B", "C/", "D") -> "A/B/C/D"
	 *
	 * @param Builder A possibly-empty path that may end in a separator.
	 * @param Args Arguments that can write to a string builder and do not start with a separator.
	 */
	template <typename CharType, typename... ArgTypes>
	static void Append(TStringBuilderBase<CharType>& Builder, ArgTypes&&... Args)
	{
		const auto AddSeparator = [&Builder]() -> TStringBuilderBase<CharType>&
		{
			if (!(Builder.Len() == 0 || Builder.LastChar() == '/' || Builder.LastChar() == '\\'))
			{
				Builder.AppendChar('/');
			}
			return Builder;
		};
		((AddSeparator() << Args), ...);
	}

	/**
	 * Replaces the pre-existing file extension of a filename. 
	 *
	 * @param InPath A valid file path with a pre-existing extension.
	 * @param InNewExtension The new extension to use (prefixing with a '.' is optional)
	 *
	 * @return The new file path complete with the new extension unless InPath is not valid in which
	 * case a copy of InPath will be returned instead.
	 */
	static FString ChangeExtension(const FStringView& InPath, const FStringView& InNewExtension);

	/** Return whether the given character is a path-separator character (/ or \) */
	static bool IsSeparator(TCHAR c);

	/**
	 * Return true if the given paths are the same path (with exceptions noted below).
	 * Case-insensitive
	 * / is treated as equal to \
	 * Presence or absence of terminating separator (/) is ignored in the comparison.
	 * Directory elements of . and .. are currently not interpreted and are treated as literal characters.
	 *    Callers should not rely on this behavior as it may be corrected in the future.
	 *    callers should instead conform the paths before calling.
	 * Relative paths and absolute paths are not resolved, and relative paths will never equal absolute paths.
	 *    Callers should not rely on this behavior as it may be corrected in the future;
	 *    callers should instead conform the paths before calling.
	 * Examples:
	 * ("../A/B.C", "../A/B.C") -> true
	 * ("../A/B", "../A/B.C")	-> false
	 * ("../A/", "../A/")		-> true
	 * ("../A/", "../A")		-> true
	 * ("d:/root/Engine/", "d:\root\Engine") -> true
	 * (../../../Engine/Content", "d:/root/Engine/Content") -> false
	 * (d:/root/Engine/..", "d:/root") -> false
	 * (d:/root/Engine/./Content", "d:/root/Engine/Content") -> false
	 */
	static bool Equals(FStringView A, FStringView B);

	/**
	 * Return true if the the first path is lexicographically less than the second path (with caveats noted below).
	 * Case-insensitive
	 * / is treated as equal to \
	 * Presence or absence of terminating separator (/) is ignored in the comparison.
	 * Directory elements of . and .. are currently not interpreted and are treated as literal characters.
	 *    Callers should not rely on this behavior as it may be corrected in the future.
	 *    callers should instead conform the paths before calling.
	 * Relative paths and absolute paths are not resolved, and relative paths will never equal absolute paths.
	 *    Callers should not rely on this behavior as it may be corrected in the future;
	 *    callers should instead conform the paths before calling.
	 * Examples:
	 * ("../A/B.C", "../A/B.C") -> false (they are equal)
	 * ("../A/B", "../A/B.C")	-> true (a string is greater than any prefix of itself)
	 * ("../A/", "../A/")		-> false (they are equal)
	 * ("../A/", "../A")		-> false (they are equal)
	 * ("../A", "../A/")		-> false (they are equal)
	 * ("d:/root/Engine/", "d:\root\Engine") -> false (they are equal)
	 * (../../../Engine/Content", "d:/root/Engine/Content") -> true ('.' is less than 'd')
	 * (d:/root/Engine/..", "d:/root") -> false (A string is greater than any prefix of itself)
	 * (d:/root/Engine/./Content", "d:/root/Engine/Content") -> false
	 */
	static bool Less(FStringView A, FStringView B);

	/**
	 * Check whether Parent is a parent path of Child and report the relative path if so.
	 * Case-insensitive
	 * / is treated as equal to \
	 * Presence or absence of terminating separator (/) is ignored in the comparison.
	 * Directory elements of . and .. are currently not interpreted and are treated as literal characters.
	 *    Callers should not rely on this behavior as it may be corrected in the future.
	 *    callers should instead conform the paths before calling.
	 * Relative paths and absolute paths are not resolved, and relative paths will never equal absolute paths.
	 *    Callers should not rely on this behavior as it may be corrected in the future;
	 *    callers should instead conform the paths before calling.
	 * Examples:
	 * ("../A/B", "../A")	-> (true, "B")
	 * ("../A\B", "../A/")	-> (true, "B")
	 * ("../A/", "../A")	-> (true, "")
	 * (".././A/", "../A")	-> (false, "")
	 * ("../../../Engine", "d:/root/Engine") -> (false, "")
	 *
	 * @param Child An absolute path that may be a child path of Parent.
	 * @param Parent An absolute path that may be a parent path of Child.
	 * @param OutRelPath Receives the relative path from Parent to Child, or empty if Parent is not a parent of Child.
	 *
	 * @return True if and only if Child is a child path of Parent (or is equal to it).
	 */
	static bool TryMakeChildPathRelativeTo(FStringView Child, FStringView Parent, FStringView& OutRelPath);

	/**
	 * Return whether Parent is a parent path of (or is equal to) Child.
	 * Case-insensitive
	 * / is treated as equal to \
	 * Presence or absence of terminating separator (/) is ignored in the comparison.
	 * Directory elements of . and .. are currently not interpreted and are treated as literal characters.
	 *    Callers should not rely on this behavior as it may be corrected in the future.
	 *    callers should instead conform the paths before calling.
	 * Relative paths and absolute paths are not resolved, and relative paths will never equal absolute paths.
	 *    Callers should not rely on this behavior as it may be corrected in the future;
	 *    callers should instead conform the paths before calling.
	 * Examples:
	 * ("../A", "../A/B")	-> true
	 * ("../A/", "../A\B")	-> true
	 * ("../A", "../A/")	-> true
	 * ("../A", ".././A/")	-> false
	 * ("d:/root/Engine", "../../../Engine") -> false
	 *
	 * @param Parent An absolute path that may be a parent path of Child.
	 * @param Child An absolute path that may be a child path of Parent.
	 *
	 * @return True if and only if Child is a child path of Parent (or is equal to it).
	 */
	static bool IsParentPathOf(FStringView Parent, FStringView Child);

	/**
	 * Return whether the given path is a relativepath - does not start with a separator or volume:.
	 * Returns true for empty paths.
	 */
	static bool IsRelativePath(FStringView InPath);

	/** Convert to absolute using process BaseDir(), normalize and append. FPaths::ConvertRelativePathToFull() equivalent. */
	static void ToAbsolutePath(FStringView InPath, FStringBuilderBase& OutPath);
	
	/** Convert to absolute using explicit BasePath, normalize and append. FPaths::ConvertRelativePathToFull() equivalent. */
	static void ToAbsolutePath(FStringView BasePath, FStringView InPath, FStringBuilderBase& OutPath);
	
	/** Convert to absolute using process BaseDir() and normalize inlined. FPaths::ConvertRelativePathToFull() equivalent. */
	static void ToAbsolutePathInline(FStringBuilderBase& InOutPath);
	
	/** Convert to absolute using explicit BasePath and normalize inlined. FPaths::ConvertRelativePathToFull() equivalent. */
	static void ToAbsolutePathInline(FStringView BasePath, FStringBuilderBase& InOutPath);

	/** Convert \\ to / and do platform-specific normalization */
	static void NormalizeFilename(FStringBuilderBase& InOutPath);

	/** Normalize and remove trailing slash unless the preceding character is '/' or ':' */
	static void NormalizeDirectoryName(FStringBuilderBase& InOutPath);

	/** Collapses redundant paths like "/./" and "SkipDir/..". FPaths::CollapseRelativeDirectories() equivalent. */
	static bool CollapseRelativeDirectories(FStringBuilderBase& InOutPath);

	/** Removes duplicate forward slashes, e.g. "a/b//c////f.e" -> "a/b/c/f.e" */
	static void RemoveDuplicateSlashes(FStringBuilderBase& InOutPath);

	/**
	 * Split the given absolute or relative path into its topmost directory and the relative path from that directory.
	 * Directory elements of . and .. are currently not interpreted and are treated as literal characters.
	 *    Callers should not rely on this behavior as it may be corrected in the future.
	 *    callers should instead conform the paths before calling.
	 *
	 * @param InPath The path to split.
	 * @param OutFirstComponent Receives the first directory element in the path, or InPath if it is a leaf path.
	 * @param OutRemainder Receives the relative path from OutFirstComponent to InPath, or empty if InPath is a leaf path.
	 */
	static void SplitFirstComponent(FStringView InPath, FStringView& OutFirstComponent, FStringView& OutRemainder);

	/**
	 * If AppendPath is a relative path, append it as a relative path onto InOutPath.
	 * If AppendPath is absolute, reset InOutPath and replace it with RelPath.
	 * Handles presence or absence of terminating separator in BasePath.
	 * Does not interpret . or ..; each occurrence of these in either path will remain in the combined InOutPath.
	 */
	static void AppendPath(FStringBuilderBase& InOutPath, FStringView AppendPath);
};
