#include "CommonHeader.h"
#include "SpirvPixelPreviewer.h"

namespace FW
{
	SpvId SpvPixelPreviewerVisitor::PatchPreviewerParams()
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));

		SpvId PreviewerParamsType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>(TArray<SpvId>{ UIntType, UIntType, UIntType, UIntType }));
		SpvId PreviewerParamsPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, PreviewerParamsType));

		int MemberOffset0 = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(PreviewerParamsType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset0, sizeof(int) }));
		int MemberOffset1 = 4;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(PreviewerParamsType, 1, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset1, sizeof(int) }));
		int MemberOffset2 = 8;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(PreviewerParamsType, 2, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset2, sizeof(int) }));
		int MemberOffset3 = 12;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(PreviewerParamsType, 3, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset3, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(PreviewerParamsType, SpvDecorationKind::Block));

		SpvId Params = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(PreviewerParamsPointerType, SpvStorageClass::Uniform);
			VarOp->SetId(Params);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(Params, "_PreviewerParams_"));
		}
		int SetNumber = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(Params, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = PreviewerParamsBindingSlot;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(Params, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		return Params;
	}

	SpvId SpvPixelPreviewerVisitor::FindOutputVariable()
	{
		for (auto& [Id, Var] : Context.GlobalVariables)
		{
			if (Var.StorageClass == SpvStorageClass::Output)
			{
				TArray<SpvDecoration> Decs;
				Context.Decorations.MultiFind(Id, Decs);
				for (const auto& Dec : Decs)
				{
					if (Dec.Kind == SpvDecorationKind::Location && Dec.Location.Number == 0)
					{
						return Id;
					}
				}
			}
		}
		return {};
	}

	bool SpvPixelPreviewerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		//Preview is handled entirely at call sites via PostAppendVar.
		return false;
	}

	void SpvPixelPreviewerVisitor::PostAppendVar(TArray<TUniquePtr<SpvInstruction>>& InstList, SpvPointer* Pointer, SpvId PackedHeaderConst, SpvId VarIdConst)
	{
		//Skip constant-value stores — no useful preview
		uint32 PH = PackDebugHeader(SpvDebuggerStateType::VarChange, CurSource.GetValue(), (uint32)CurLine);
		uint64 ConstKey = ((uint64)PH << 32) | Pointer->Var->Id.GetValue();
		if (Context.ConstInitVarChanges.Contains(ConstKey))
		{
			return;
		}

		int32 IndexNum = Pointer->Indexes.Num();
		SpvType* PointeeType = Pointer->Type->PointeeType;
		SpvType* VarType = Pointer->Var->PointerType->PointeeType;

		//Determine preview type and pointer to load from
		SpvType* PreviewType;
		SpvId PreviewPointer;
		if (IndexNum == 0)
		{
			//Whole-variable write: load from the variable itself
			PreviewType = VarType;
			PreviewPointer = Pointer->Var->Id;
		}
		else if (VarType->IsScalar() || VarType->GetKind() == SpvTypeKind::Vector)
		{
			//Component access of a scalar/vector variable (e.g. fragColor.r): load the full variable
			PreviewType = VarType;
			PreviewPointer = Pointer->Var->Id;
		}
		else
		{
			//Struct/array member or nested access: load from the accessed element
			PreviewType = PointeeType;
			PreviewPointer = Pointer->Id;
		}

		//Only preview scalar and vector types
		if (!PreviewType->IsScalar() && PreviewType->GetKind() != SpvTypeKind::Vector)
		{
			return;
		}

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));

		//Load TargetPackedHeader from uniform member 1
		SpvId TargetHeaderPtr = Patcher.NewId();
		auto TargetHeaderPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, PreviewerParams, TArray<SpvId>{ Patcher.FindOrAddConstant(1u) });
		TargetHeaderPtrOp->SetId(TargetHeaderPtr);
		InstList.Add(MoveTemp(TargetHeaderPtrOp));

		SpvId TargetHeader = Patcher.NewId();
		auto TargetHeaderOp = MakeUnique<SpvOpLoad>(UIntType, TargetHeaderPtr);
		TargetHeaderOp->SetId(TargetHeader);
		InstList.Add(MoveTemp(TargetHeaderOp));

		//Load TargetVarId from uniform member 2
		SpvId TargetVarIdPtr = Patcher.NewId();
		auto TargetVarIdPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, PreviewerParams, TArray<SpvId>{ Patcher.FindOrAddConstant(2u) });
		TargetVarIdPtrOp->SetId(TargetVarIdPtr);
		InstList.Add(MoveTemp(TargetVarIdPtrOp));

		SpvId TargetVarId = Patcher.NewId();
		auto TargetVarIdOp = MakeUnique<SpvOpLoad>(UIntType, TargetVarIdPtr);
		TargetVarIdOp->SetId(TargetVarId);
		InstList.Add(MoveTemp(TargetVarIdOp));

		//Check if (PackedHeader, VarId) match the target
		SpvId MatchHeader = Patcher.NewId();
		auto MatchHeaderOp = MakeUnique<SpvOpIEqual>(BoolType, PackedHeaderConst, TargetHeader);
		MatchHeaderOp->SetId(MatchHeader);
		InstList.Add(MoveTemp(MatchHeaderOp));

		SpvId MatchVarId = Patcher.NewId();
		auto MatchVarIdOp = MakeUnique<SpvOpIEqual>(BoolType, VarIdConst, TargetVarId);
		MatchVarIdOp->SetId(MatchVarId);
		InstList.Add(MoveTemp(MatchVarIdOp));

		//Load TargetCallStackHash from uniform member 3
		SpvId TargetHashPtr = Patcher.NewId();
		auto TargetHashPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, PreviewerParams, TArray<SpvId>{ Patcher.FindOrAddConstant(3u) });
		TargetHashPtrOp->SetId(TargetHashPtr);
		InstList.Add(MoveTemp(TargetHashPtrOp));

		SpvId TargetHash = Patcher.NewId();
		auto TargetHashOp = MakeUnique<SpvOpLoad>(UIntType, TargetHashPtr);
		TargetHashOp->SetId(TargetHash);
		InstList.Add(MoveTemp(TargetHashOp));

		//Load current CallStackHash value
		SpvId CurHash = Patcher.NewId();
		auto CurHashOp = MakeUnique<SpvOpLoad>(UIntType, CallStackHashVar);
		CurHashOp->SetId(CurHash);
		InstList.Add(MoveTemp(CurHashOp));

		SpvId MatchHash = Patcher.NewId();
		auto MatchHashOp = MakeUnique<SpvOpIEqual>(BoolType, CurHash, TargetHash);
		MatchHashOp->SetId(MatchHash);
		InstList.Add(MoveTemp(MatchHashOp));

		//Combine: (PackedHeader && VarId && CallStackHash)
		SpvId MatchHeaderAndVarId = Patcher.NewId();
		auto MatchHeaderAndVarIdOp = MakeUnique<SpvOpLogicalAnd>(BoolType, MatchHeader, MatchVarId);
		MatchHeaderAndVarIdOp->SetId(MatchHeaderAndVarId);
		InstList.Add(MoveTemp(MatchHeaderAndVarIdOp));

		SpvId IsTarget = Patcher.NewId();
		auto IsTargetOp = MakeUnique<SpvOpLogicalAnd>(BoolType, MatchHeaderAndVarId, MatchHash);
		IsTargetOp->SetId(IsTarget);
		InstList.Add(MoveTemp(IsTargetOp));

		//Outer branch: only enter when (PackedHeader, VarId) match the target
		auto OuterMerge = Patcher.NewId();
		auto OuterThen = Patcher.NewId();
		InstList.Add(MakeUnique<SpvOpSelectionMerge>(OuterMerge, SpvSelectionControl::None));
		InstList.Add(MakeUnique<SpvOpBranchConditional>(IsTarget, OuterThen, OuterMerge));

		auto OuterThenOp = MakeUnique<SpvOpLabel>();
		OuterThenOp->SetId(OuterThen);
		InstList.Add(MoveTemp(OuterThenOp));

		//Iteration counting: increment counter, then check if it matches the target iteration
		SpvId LoadedCounter = Patcher.NewId();
		auto LoadCounterOp = MakeUnique<SpvOpLoad>(UIntType, StateCounter);
		LoadCounterOp->SetId(LoadedCounter);
		InstList.Add(MoveTemp(LoadCounterOp));

		SpvId NewCounter = Patcher.NewId();
		auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedCounter, Patcher.FindOrAddConstant(1u));
		AddOp->SetId(NewCounter);
		InstList.Add(MoveTemp(AddOp));

		InstList.Add(MakeUnique<SpvOpStore>(StateCounter, NewCounter));

		//Load TargetIteration from uniform member 0
		SpvId TargetIterPtr = Patcher.NewId();
		auto TargetIterPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, PreviewerParams, TArray<SpvId>{ Patcher.FindOrAddConstant(0u) });
		TargetIterPtrOp->SetId(TargetIterPtr);
		InstList.Add(MoveTemp(TargetIterPtrOp));

		SpvId TargetIter = Patcher.NewId();
		auto TargetIterOp = MakeUnique<SpvOpLoad>(UIntType, TargetIterPtr);
		TargetIterOp->SetId(TargetIter);
		InstList.Add(MoveTemp(TargetIterOp));

		SpvId MatchIter = Patcher.NewId();
		auto MatchIterOp = MakeUnique<SpvOpIEqual>(BoolType, LoadedCounter, TargetIter);
		MatchIterOp->SetId(MatchIter);
		InstList.Add(MoveTemp(MatchIterOp));

		//Inner branch: iteration matches -> load variable and store to _PreviewOutput_
		auto InnerMerge = Patcher.NewId();
		auto InnerThen = Patcher.NewId();
		InstList.Add(MakeUnique<SpvOpSelectionMerge>(InnerMerge, SpvSelectionControl::None));
		InstList.Add(MakeUnique<SpvOpBranchConditional>(MatchIter, InnerThen, InnerMerge));

		auto InnerThenOp = MakeUnique<SpvOpLabel>();
		InnerThenOp->SetId(InnerThen);
		InstList.Add(MoveTemp(InnerThenOp));

		SpvId FullValue = Patcher.NewId();
		auto LoadOp = MakeUnique<SpvOpLoad>(PreviewType->GetId(), PreviewPointer);
		LoadOp->SetId(FullValue);
		InstList.Add(MoveTemp(LoadOp));

		SpvId Float4Val;
		ConvertToFloat4(PreviewType, FullValue, InstList, Float4Val);
		InstList.Add(MakeUnique<SpvOpStore>(PreviewOutputVar, Float4Val));

		InstList.Add(MakeUnique<SpvOpBranch>(InnerMerge));

		auto InnerMergeOp = MakeUnique<SpvOpLabel>();
		InnerMergeOp->SetId(InnerMerge);
		InstList.Add(MoveTemp(InnerMergeOp));

		InstList.Add(MakeUnique<SpvOpBranch>(OuterMerge));

		auto OuterMergeOp = MakeUnique<SpvOpLabel>();
		OuterMergeOp->SetId(OuterMerge);
		InstList.Add(MoveTemp(OuterMergeOp));
	}

	void SpvPixelPreviewerVisitor::PostAppendCall(TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId PackedHeader, SpvId CallId)
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));

		//Update hash: hash = hash * 31 + CallId
		SpvId LoadedHash = Patcher.NewId();
		auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, CallStackHashVar);
		LoadOp->SetId(LoadedHash);
		InstList.Add(MoveTemp(LoadOp));

		SpvId HashMul = Patcher.NewId();
		auto MulOp = MakeUnique<SpvOpIMul>(UIntType, LoadedHash, Patcher.FindOrAddConstant(31u));
		MulOp->SetId(HashMul);
		InstList.Add(MoveTemp(MulOp));

		SpvId NewHash = Patcher.NewId();
		auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, HashMul, CallId);
		AddOp->SetId(NewHash);
		InstList.Add(MoveTemp(AddOp));

		InstList.Add(MakeUnique<SpvOpStore>(CallStackHashVar, NewHash));
	}

	void SpvPixelPreviewerVisitor::PostCallReturn(TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId CallId)
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));

		//Reverse hash: hash = (hash - CallId) * 31^(-1) mod 2^32
		//31^(-1) mod 2^32 = 3186588639
		SpvId LoadedHash = Patcher.NewId();
		auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, CallStackHashVar);
		LoadOp->SetId(LoadedHash);
		InstList.Add(MoveTemp(LoadOp));

		SpvId SubResult = Patcher.NewId();
		auto SubOp = MakeUnique<SpvOpISub>(UIntType, LoadedHash, CallId);
		SubOp->SetId(SubResult);
		InstList.Add(MoveTemp(SubOp));

		SpvId RestoredHash = Patcher.NewId();
		auto MulOp = MakeUnique<SpvOpIMul>(UIntType, SubResult, Patcher.FindOrAddConstant(3186588639u));
		MulOp->SetId(RestoredHash);
		InstList.Add(MoveTemp(MulOp));

		InstList.Add(MakeUnique<SpvOpStore>(CallStackHashVar, RestoredHash));
	}

	void SpvPixelPreviewerVisitor::ConvertScalarToFloat(SpvType* SrcType, SpvId SrcValue, TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId& OutFloatValue)
	{
		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));

		if (SrcType->GetKind() == SpvTypeKind::Float)
		{
			OutFloatValue = SrcValue;
		}
		else if (SrcType->GetKind() == SpvTypeKind::Integer)
		{
			OutFloatValue = Patcher.NewId();
			auto* IntType = static_cast<SpvIntegerType*>(SrcType);
			if (IntType->IsSigend())
			{
				auto ConvOp = MakeUnique<SpvOpConvertSToF>(FloatType, SrcValue);
				ConvOp->SetId(OutFloatValue);
				InstList.Add(MoveTemp(ConvOp));
			}
			else
			{
				auto ConvOp = MakeUnique<SpvOpConvertUToF>(FloatType, SrcValue);
				ConvOp->SetId(OutFloatValue);
				InstList.Add(MoveTemp(ConvOp));
			}
		}
		else if (SrcType->GetKind() == SpvTypeKind::Bool)
		{
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			OutFloatValue = Patcher.NewId();
			auto SelectOp = MakeUnique<SpvOpSelect>(FloatType, SrcValue, Patcher.FindOrAddConstant(1.0f), Patcher.FindOrAddConstant(0.0f));
			SelectOp->SetId(OutFloatValue);
			InstList.Add(MoveTemp(SelectOp));
		}
		else
		{
			OutFloatValue = Patcher.FindOrAddConstant(0.0f);
		}
	}

	void SpvPixelPreviewerVisitor::ConvertToFloat4(SpvType* SrcType, SpvId SrcValue, TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId& OutFloat4Value)
	{
		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));
		SpvId ConstOne = Patcher.FindOrAddConstant(1.0f);
		SpvId ConstZero = Patcher.FindOrAddConstant(0.0f);

		if (SrcType->GetKind() == SpvTypeKind::Vector)
		{
			auto* VecType = static_cast<SpvVectorType*>(SrcType);
			uint32 Count = VecType->ElementCount;
			SpvScalarType* ElemType = VecType->ElementType;

			//Convert element type to float if needed
			bool NeedConvert = ElemType->GetKind() != SpvTypeKind::Float;
			SpvId FloatVecType = Float4Type;
			if (NeedConvert && Count < 4)
			{
				FloatVecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, Count));
			}

			SpvId FloatVecValue = SrcValue;
			if (NeedConvert)
			{
				if (ElemType->GetKind() == SpvTypeKind::Integer)
				{
					auto* IntElemType = static_cast<SpvIntegerType*>(ElemType);
					FloatVecValue = Patcher.NewId();
					if (Count == 4) FloatVecType = Float4Type;
					else FloatVecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, Count));

					if (IntElemType->IsSigend())
					{
						auto ConvOp = MakeUnique<SpvOpConvertSToF>(FloatVecType, SrcValue);
						ConvOp->SetId(FloatVecValue);
						InstList.Add(MoveTemp(ConvOp));
					}
					else
					{
						auto ConvOp = MakeUnique<SpvOpConvertUToF>(FloatVecType, SrcValue);
						ConvOp->SetId(FloatVecValue);
						InstList.Add(MoveTemp(ConvOp));
					}
				}
				else if (ElemType->GetKind() == SpvTypeKind::Bool)
				{
					//Bool vector: select 1.0 or 0.0 per component
					SpvId BoolVecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>()), Count));
					FloatVecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, Count));

					TArray<SpvId> Ones, Zeros;
					for (uint32 i = 0; i < Count; i++) { Ones.Add(ConstOne); Zeros.Add(ConstZero); }
					SpvId OnesVec = Patcher.NewId();
					auto OnesOp = MakeUnique<SpvOpCompositeConstruct>(FloatVecType, Ones);
					OnesOp->SetId(OnesVec);
					InstList.Add(MoveTemp(OnesOp));
					SpvId ZerosVec = Patcher.NewId();
					auto ZerosOp = MakeUnique<SpvOpCompositeConstruct>(FloatVecType, Zeros);
					ZerosOp->SetId(ZerosVec);
					InstList.Add(MoveTemp(ZerosOp));

					FloatVecValue = Patcher.NewId();
					auto SelectOp = MakeUnique<SpvOpSelect>(FloatVecType, SrcValue, OnesVec, ZerosVec);
					SelectOp->SetId(FloatVecValue);
					InstList.Add(MoveTemp(SelectOp));
				}
			}

			if (Count == 4)
			{
				OutFloat4Value = FloatVecValue;
			}
			else if (Count == 3)
			{
				//Shuffle xyz, then construct with w=1.0
				OutFloat4Value = Patcher.NewId();
				auto ConstructOp = MakeUnique<SpvOpCompositeConstruct>(Float4Type, TArray<SpvId>{ FloatVecValue, ConstOne });
				ConstructOp->SetId(OutFloat4Value);
				InstList.Add(MoveTemp(ConstructOp));
			}
			else if (Count == 2)
			{
				OutFloat4Value = Patcher.NewId();
				auto ConstructOp = MakeUnique<SpvOpCompositeConstruct>(Float4Type, TArray<SpvId>{ FloatVecValue, ConstZero, ConstOne });
				ConstructOp->SetId(OutFloat4Value);
				InstList.Add(MoveTemp(ConstructOp));
			}
		}
		else if (SrcType->IsScalar())
		{
			SpvId FloatVal;
			ConvertScalarToFloat(SrcType, SrcValue, InstList, FloatVal);
			OutFloat4Value = Patcher.NewId();
			auto ConstructOp = MakeUnique<SpvOpCompositeConstruct>(Float4Type, TArray<SpvId>{ FloatVal, FloatVal, FloatVal, ConstOne });
			ConstructOp->SetId(OutFloat4Value);
			InstList.Add(MoveTemp(ConstructOp));
		}
		else
		{
			AUX::Unreachable();
		}
	}

	void SpvPixelPreviewerVisitor::ParseInternal()
	{
		//Find the output variable before base patching
		OutputVarId = FindOutputVariable();

		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));

		//Add _PreviewOutput_ float4 Private global initialized to vec4(0)
		SpvId Float4PointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, Float4Type));
		PreviewOutputVar = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(Float4PointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(PreviewOutputVar);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PreviewOutputVar, "_PreviewOutput_"));
		}

		//Add _PreviewStateCounter_ uint Private global initialized to 0
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		StateCounter = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(StateCounter);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateCounter, "_PreviewStateCounter_"));
		}

		//Add _PreviewCallStackHash_ uint Private global initialized to 0
		CallStackHashVar = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(CallStackHashVar);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(CallStackHashVar, "_PreviewCallStackHash_"));
		}

		//Add _PreviewerParams_ uniform buffer
		PreviewerParams = PatchPreviewerParams();

		//Call base class patching (creates debug buffer, AppendVar/Tag/Call infrastructure)
		//PatchActiveCondition override handles preview logic inside AppendVar functions
		SpvDebuggerVisitor::ParseInternal();

		//Insert PreviewOutput override before each OpReturn in the entry point function
		if (OutputVarId.IsValid())
		{
			int32 EntryIndex = GetInstIndex(Insts, Context.EntryPoint);

			//Insert grid-pattern default for _PreviewOutput_ at the start of the entry point
			//Computes a checkerboard: if (floor(fragCoord.x/8) + floor(fragCoord.y/8)) is odd -> 0.15 else 0.1
			if (Context.BuiltIns.Contains(SpvBuiltIn::FragCoord))
			{
				for (int32 i = EntryIndex; i < Insts->Num(); i++)
				{
					if (auto* Label = dynamic_cast<const SpvOpLabel*>((*Insts)[i].Get()))
					{
						TArray<TUniquePtr<SpvInstruction>> GridInsts;
						SpvId ConstGridSize = Patcher.FindOrAddConstant(8.0f);
						SpvId ConstDark = Patcher.FindOrAddConstant(0.1f);
						SpvId ConstLight = Patcher.FindOrAddConstant(0.15f);

						//Load FragCoord
						SpvId LoadedFragCoord = Patcher.NewId();
						auto LoadFragCoordOp = MakeUnique<SpvOpLoad>(Float4Type, Context.BuiltIns[SpvBuiltIn::FragCoord]);
						LoadFragCoordOp->SetId(LoadedFragCoord);
						GridInsts.Add(MoveTemp(LoadFragCoordOp));

						//Extract x and y
						SpvId FragX = Patcher.NewId();
						auto ExtractXOp = MakeUnique<SpvOpCompositeExtract>(FloatType, LoadedFragCoord, TArray<uint32>{0});
						ExtractXOp->SetId(FragX);
						GridInsts.Add(MoveTemp(ExtractXOp));

						SpvId FragY = Patcher.NewId();
						auto ExtractYOp = MakeUnique<SpvOpCompositeExtract>(FloatType, LoadedFragCoord, TArray<uint32>{1});
						ExtractYOp->SetId(FragY);
						GridInsts.Add(MoveTemp(ExtractYOp));

						//Divide by grid size
						SpvId ScaledX = Patcher.NewId();
						auto DivXOp = MakeUnique<SpvOpFDiv>(FloatType, FragX, ConstGridSize);
						DivXOp->SetId(ScaledX);
						GridInsts.Add(MoveTemp(DivXOp));

						SpvId ScaledY = Patcher.NewId();
						auto DivYOp = MakeUnique<SpvOpFDiv>(FloatType, FragY, ConstGridSize);
						DivYOp->SetId(ScaledY);
						GridInsts.Add(MoveTemp(DivYOp));

						//Floor
						SpvId ExtSet450 = *Context.ExtSets.FindKey(SpvExtSet::GLSLstd450);
						SpvId FloorX = Patcher.NewId();
						auto FloorXOp = MakeUnique<SpvFloor>(FloatType, ExtSet450, ScaledX);
						FloorXOp->SetId(FloorX);
						GridInsts.Add(MoveTemp(FloorXOp));

						SpvId FloorY = Patcher.NewId();
						auto FloorYOp = MakeUnique<SpvFloor>(FloatType, ExtSet450, ScaledY);
						FloorYOp->SetId(FloorY);
						GridInsts.Add(MoveTemp(FloorYOp));

						//Convert to uint
						SpvId UintX = Patcher.NewId();
						auto ConvXOp = MakeUnique<SpvOpConvertFToU>(UIntType, FloorX);
						ConvXOp->SetId(UintX);
						GridInsts.Add(MoveTemp(ConvXOp));

						SpvId UintY = Patcher.NewId();
						auto ConvYOp = MakeUnique<SpvOpConvertFToU>(UIntType, FloorY);
						ConvYOp->SetId(UintY);
						GridInsts.Add(MoveTemp(ConvYOp));

						//Add and check parity: (ux + uy) & 1
						SpvId Sum = Patcher.NewId();
						auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, UintX, UintY);
						AddOp->SetId(Sum);
						GridInsts.Add(MoveTemp(AddOp));

						SpvId Parity = Patcher.NewId();
						auto AndOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, Sum, Patcher.FindOrAddConstant(1u));
						AndOp->SetId(Parity);
						GridInsts.Add(MoveTemp(AndOp));

						//Compare parity != 0
						SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
						SpvId IsOdd = Patcher.NewId();
						auto CmpOp = MakeUnique<SpvOpINotEqual>(BoolType, Parity, Patcher.FindOrAddConstant(0u));
						CmpOp->SetId(IsOdd);
						GridInsts.Add(MoveTemp(CmpOp));

						//Select dark or light
						SpvId GridVal = Patcher.NewId();
						auto SelectOp = MakeUnique<SpvOpSelect>(FloatType, IsOdd, ConstLight, ConstDark);
						SelectOp->SetId(GridVal);
						GridInsts.Add(MoveTemp(SelectOp));

						//Construct float4 and store to _PreviewOutput_
						SpvId GridColor = Patcher.NewId();
						auto ConstructOp = MakeUnique<SpvOpCompositeConstruct>(Float4Type, TArray<SpvId>{ GridVal, GridVal, GridVal, Patcher.FindOrAddConstant(1.0f) });
						ConstructOp->SetId(GridColor);
						GridInsts.Add(MoveTemp(ConstructOp));

						GridInsts.Add(MakeUnique<SpvOpStore>(PreviewOutputVar, GridColor));

						Patcher.AddInstructions(Label->GetWordOffset().value() + Label->ToBinary().Num(), MoveTemp(GridInsts));
						break;
					}
				}
			}

			for (int32 i = EntryIndex; i < Insts->Num(); i++)
			{
				if (dynamic_cast<const SpvOpFunctionEnd*>((*Insts)[i].Get()))
				{
					break;
				}
				if (auto* Ret = dynamic_cast<const SpvOpReturn*>((*Insts)[i].Get()))
				{
					TArray<TUniquePtr<SpvInstruction>> OverrideInsts;
					SpvId LoadedPreview = Patcher.NewId();
					auto LoadOp = MakeUnique<SpvOpLoad>(Float4Type, PreviewOutputVar);
					LoadOp->SetId(LoadedPreview);
					OverrideInsts.Add(MoveTemp(LoadOp));
					OverrideInsts.Add(MakeUnique<SpvOpStore>(OutputVarId, LoadedPreview));
					Patcher.AddInstructions(Ret->GetWordOffset().value(), MoveTemp(OverrideInsts));
				}
			}
		}
	}
}
