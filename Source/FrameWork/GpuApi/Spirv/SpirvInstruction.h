#pragma once
#include "SpirvCore.h"
#include "SpirvExt.h"

namespace FW
{
	class SpvInstruction;
	class SpvVisitor
	{
	public:
		virtual void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) {}
		
		virtual void Visit(SpvInstruction*) {}
		
		//Core
		virtual void Visit(class SpvOpEntryPoint* Inst) {}
		virtual void Visit(class SpvOpExecutionMode* Inst) {}
		virtual void Visit(class SpvOpString* Inst) {}
		virtual void Visit(class SpvOpDecorate* Inst) {}
		virtual void Visit(class SpvOpTypeVoid* Inst) {}
		virtual void Visit(class SpvOpTypeBool* Inst) {}
		virtual void Visit(class SpvOpTypeFloat* Inst) {}
		virtual void Visit(class SpvOpTypeInt* Inst) {}
		virtual void Visit(class SpvOpTypeVector* Inst) {}
		virtual void Visit(class SpvOpTypePointer* Inst) {}
		virtual void Visit(class SpvOpTypeStruct* Inst) {}
		
		virtual void Visit(class SpvOpConstant* Inst) {}
		virtual void Visit(class SpvOpConstantTrue* Inst) {}
		virtual void Visit(class SpvOpConstantFalse* Inst) {}
		virtual void Visit(class SpvOpConstantComposite* Inst) {}
		virtual void Visit(class SpvOpFunction* Inst) {}
		virtual void Visit(class SpvOpFunctionParameter* Inst) {}
		virtual void Visit(class SpvOpFunctionCall* Inst) {}
		virtual void Visit(class SpvOpVariable* Inst) {}
		virtual void Visit(class SpvOpLabel* Inst) {}
		virtual void Visit(class SpvOpLoad* Inst) {}
		virtual void Visit(class SpvOpStore* Inst) {}
		virtual void Visit(class SpvOpCompositeConstruct* Inst) {}
		virtual void Visit(class SpvOpAccessChain* Inst) {}
		
		//NonSemantic.Shader.DebugInfo.100
		virtual void Visit(class SpvDebugCompilationUnit* Inst) {}
		virtual void Visit(class SpvDebugFunction* Inst) {}
		virtual void Visit(class SpvDebugLine* Inst) {}
		virtual void Visit(class SpvDebugScope* Inst) {}
		virtual void Visit(class SpvDebugDeclare* Inst) {}
		virtual void Visit(class SpvDebugLocalVariable* Inst) {}
		virtual void Visit(class SpvDebugGlobalVariable* Inst) {}
		
		//GLSL.std.450
	};

	using SpvInstKind = std::variant<SpvOp, SpvDebugInfo100>;

	class SpvInstruction
	{
	public:
		SpvInstruction(SpvInstKind InKind) : Kind(InKind) {}
		virtual ~SpvInstruction() = default;
		
	public:
		const SpvInstKind& GetKind() const { return Kind;}
		std::optional<SpvId> GetId() const { return ResultId; }
		void SetId(SpvId Id) { ResultId = Id; }
		virtual void Accpet(SpvVisitor* Visitor) = 0;
		
	private:
		SpvInstKind Kind;
		std::optional<SpvId> ResultId;
	};

	template<typename T>
	class SpvInstructionBase : public SpvInstruction
	{
	public:
		using SpvInstruction::SpvInstruction;
		
		void Accpet(SpvVisitor* Visitor) override
		{
			Visitor->Visit(static_cast<T*>(this));
		}
	};

	class SpvOpExecutionMode : public SpvInstructionBase<SpvOpExecutionMode>
	{
	public:
		SpvOpExecutionMode(SpvId InEntryPoint, SpvExecutionMode InMode,
						   const TArray<uint8>& InOperands)
			: SpvInstructionBase(SpvOp::ExecutionMode)
			, EntryPoint(InEntryPoint)
			, Mode(InMode)
			, Operands(InOperands)
		{}
	public:
		SpvId GetEntryPointId() const { return EntryPoint; }
		SpvExecutionMode GetMode() const { return Mode; }
		const TArray<uint8>& GetExtraOperands() const { return Operands; }
		
	private:
		SpvId EntryPoint;
		SpvExecutionMode Mode;
		TArray<uint8> Operands;
	};

	class SpvOpEntryPoint : public SpvInstructionBase<SpvOpEntryPoint>
	{
	public:
		SpvOpEntryPoint(SpvExecutionModel InModel, SpvId InEntryPoint, const FString& InEntryPointName)
		: SpvInstructionBase(SpvOp::EntryPoint)
		, Model(InModel)
		, EntryPoint(InEntryPoint)
		, EntryPointName(InEntryPointName)
		{}
		
	private:
		SpvExecutionModel Model;
		SpvId EntryPoint;
		FString EntryPointName;
	};

	class SpvOpDecorate : public SpvInstructionBase<SpvOpDecorate>
	{
	public:
		SpvOpDecorate(SpvId InTarget, SpvDecorationKind InDecorationKind, const TArray<uint8>& InOperands)
		: SpvInstructionBase(SpvOp::Decorate)
		, Target(InTarget)
		, DecorationKind(InDecorationKind)
		, Operands(InOperands)
		{}
		
		SpvId GetTargetId() const { return Target; }
		SpvDecorationKind GetKind() const { return DecorationKind; }
		const TArray<uint8>& GetExtraOperands() const { return Operands; }
		
	private:
		SpvId Target;
		SpvDecorationKind DecorationKind;
		TArray<uint8> Operands;
	};

	class SpvOpTypeFloat : public SpvInstructionBase<SpvOpTypeFloat>
	{
	public:
		SpvOpTypeFloat(uint32 InWidth)
		: SpvInstructionBase(SpvOp::TypeFloat)
		, Width(InWidth)
		{}
		uint32 GetWidth() const { return Width; }
		
	private:
		uint32 Width;
	};

	class SpvOpTypeInt : public SpvInstructionBase<SpvOpTypeInt>
	{
	public:
		SpvOpTypeInt(uint32 InWidth, uint32 InSignedness)
		: SpvInstructionBase(SpvOp::TypeInt)
		, Width(InWidth)
		, Signedness(InSignedness)
		{}
		
		uint32 GetWidth() const { return Width; }
		uint32 GetSignedness() const { return Signedness; }
		
	private:
		uint32 Width;
		uint32 Signedness;
	};

	class SpvOpTypeVector: public SpvInstructionBase<SpvOpTypeVector>
	{
	public:
		SpvOpTypeVector(SpvId InComponentType, uint32 InComponentCount)
		: SpvInstructionBase(SpvOp::TypeVector)
		, ComponentType(InComponentType)
		, ComponentCount(InComponentCount)
		{}
		
		SpvId GetComponentTypeId() const { return ComponentType; }
		uint32 GetComponentCount() const { return ComponentCount; }
		
	private:
		SpvId ComponentType;
		uint32 ComponentCount;
	};

	class SpvOpTypeVoid : public SpvInstructionBase<SpvOpTypeVoid>
	{
	public:
		SpvOpTypeVoid() : SpvInstructionBase(SpvOp::TypeVoid) {}
	};

	class SpvOpTypeBool : public SpvInstructionBase<SpvOpTypeBool>
	{
	public:
		SpvOpTypeBool() : SpvInstructionBase(SpvOp::TypeBool) {}
	};

	class SpvOpTypePointer : public SpvInstructionBase<SpvOpTypePointer>
	{
	public:
		SpvOpTypePointer(SpvStorageClass InStorageClass, SpvId InType)
		: SpvInstructionBase(SpvOp::TypePointer)
		, StorageClass(InStorageClass)
		, PointeeType(InType)
		{}
		
		SpvStorageClass GetStorageClass() const { return StorageClass; }
		SpvId GetPointeeType() const { return PointeeType; }
		
	private:
		SpvStorageClass StorageClass;
		SpvId PointeeType;
	};

	class SpvOpTypeStruct : public SpvInstructionBase<SpvOpTypeStruct>
	{
	public:
		SpvOpTypeStruct(const TArray<SpvId>& InMemberTypes)
		: SpvInstructionBase(SpvOp::TypeStruct)
		, MemberTypes(InMemberTypes)
		{}
		
		const TArray<SpvId>& GetMemberTypeIds() const { return MemberTypes; }
	
	private:
		TArray<SpvId> MemberTypes;
	};

	class SpvOpConstant : public SpvInstructionBase<SpvOpConstant>
	{
	public:
		SpvOpConstant(SpvId InResultType, const TArray<uint8>& InValue, SpvOp OpCode = SpvOp::Constant)
		: SpvInstructionBase(OpCode)
		, ResultType(InResultType)
		, Value(InValue)
		{}
		SpvId GetResultType() const { return ResultType; }
		const TArray<uint8>& GetValue() const { return Value; }
		
	protected:
		SpvId ResultType;
		TArray<uint8> Value;
	};

	class SpvOpConstantTrue : public SpvOpConstant
	{
	public:
		SpvOpConstantTrue(SpvId InResultType)
		: SpvOpConstant(InResultType, {(uint8*)new uint32{1}, 4}, SpvOp::ConstantTrue)
		{}
	};

	class SpvOpConstantFalse : public SpvOpConstant
	{
	public:
		SpvOpConstantFalse(SpvId InResultType)
		: SpvOpConstant(InResultType, {(uint8*)new uint32{0}, 4}, SpvOp::ConstantFalse)
		{}
	};

	class SpvOpConstantComposite : public SpvInstructionBase<SpvOpConstantComposite>
	{
	public:
		SpvOpConstantComposite(SpvId InResultType, const TArray<SpvId>& InConstituents)
		: SpvInstructionBase(SpvOp::ConstantComposite)
		, ResultType(InResultType)
		, Constituents(InConstituents)
		{}
		SpvId GetResultType() const { return ResultType; }
		const TArray<SpvId>& GetConstituents() const { return Constituents; }
		
	private:
		SpvId ResultType;
		TArray<SpvId> Constituents;
	};

	class SpvOpString : public SpvInstructionBase<SpvOpString>
	{
	public:
		SpvOpString(const FString& InStr)
		: SpvInstructionBase(SpvOp::String)
		, Str(InStr)
		{}
		
		const FString& GetStr() const { return Str; }
		
	private:
		FString Str;
	};

	class SpvOpFunction : public SpvInstructionBase<SpvOpFunction>
	{
	public:
		SpvOpFunction(SpvId InResultType, SpvId InFunctionType) : SpvInstructionBase(SpvOp::Function)
		, ResultType(InResultType)
		, FunctionType(InFunctionType)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFunctionType() const { return FunctionType; }
		
	private:
		SpvId ResultType;
		SpvId FunctionType;
	};

	class SpvOpFunctionParameter : public SpvInstructionBase<SpvOpFunctionParameter>
	{
	public:
		SpvOpFunctionParameter(SpvId InResultType) : SpvInstructionBase(SpvOp::FunctionParameter)
		, ResultType(InResultType)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		
	private:
		SpvId ResultType;
	};

	class SpvOpFunctionCall : public SpvInstructionBase<SpvOpFunctionCall>
	{
	public:
		SpvOpFunctionCall(SpvId InResultType, SpvId InFunction, const TArray<SpvId>& InArguments) : SpvInstructionBase(SpvOp::FunctionCall)
		, ResultType(InResultType)
		, Function(InFunction)
		, Arguments(InArguments)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFunction() const { return Function; }
		const TArray<SpvId>& GetArguments() const { return Arguments; }
		
	private:
		SpvId ResultType;
		SpvId Function;
		TArray<SpvId> Arguments;
	};

	class SpvOpVariable : public SpvInstructionBase<SpvOpVariable>
	{
	public:
		SpvOpVariable(SpvId InResultType, SpvStorageClass InStorageClass) : SpvInstructionBase(SpvOp::Variable)
		, ResultType(InResultType)
		, StorageClass(InStorageClass)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvStorageClass GetStorageClass() const { return StorageClass; }
		
	private:
		SpvId ResultType;
		SpvStorageClass StorageClass;
	};

	class SpvOpLabel : public SpvInstructionBase<SpvOpLabel>
	{
	public:
		SpvOpLabel() : SpvInstructionBase(SpvOp::Label) {}
	};

	class SpvOpLoad : public SpvInstructionBase<SpvOpLoad>
	{
	public:
		SpvOpLoad(SpvId InResultType, SpvId InPointer) : SpvInstructionBase(SpvOp::Load)
		, ResultType(InResultType)
		, Pointer(InPointer)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetPointer() const { return Pointer; }
		
	private:
		SpvId ResultType;
		SpvId Pointer;
	};

	class SpvOpStore : public SpvInstructionBase<SpvOpStore>
	{
	public:
		SpvOpStore(SpvId InPointer, SpvId InObject) : SpvInstructionBase(SpvOp::Store)
		, Pointer(InPointer)
		, Object(InObject)
		{}
		
		SpvId GetPointer() const { return Pointer; }
		SpvId GetObject() const { return Object; }
		
	private:
		SpvId Pointer;
		SpvId Object;
	};

	class SpvOpCompositeConstruct : public SpvInstructionBase<SpvOpCompositeConstruct>
	{
	public:
		SpvOpCompositeConstruct(SpvId InResultType, const TArray<SpvId>& InConstituents) : SpvInstructionBase(SpvOp::CompositeConstruct)
		, ResultType(InResultType)
		, Constituents(InConstituents)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		const TArray<SpvId>& GetConstituents() const { return Constituents; }
		
	private:
		SpvId ResultType;
		TArray<SpvId> Constituents;
	};

	class SpvOpAccessChain : public SpvInstructionBase<SpvOpAccessChain>
	{
	public:
		SpvOpAccessChain(SpvId InResultType, SpvId InBasePointer, const TArray<SpvId>& InIndexes) : SpvInstructionBase(SpvOp::AccessChain)
		, ResultType(InResultType)
		, BasePointer(InBasePointer)
		, Indexes(InIndexes)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetBasePointer() const { return BasePointer; }
		const TArray<SpvId>& GetIndexes() const { return Indexes; }
		
	private:
		SpvId ResultType;
		SpvId BasePointer;
		TArray<SpvId> Indexes;
	};


	class SpvDebugCompilationUnit : public SpvInstructionBase<SpvDebugCompilationUnit>
	{
	public:
		SpvDebugCompilationUnit() : SpvInstructionBase(SpvDebugInfo100::DebugCompilationUnit) {}
	};

	class SpvDebugFunction : public SpvInstructionBase<SpvDebugFunction>
	{
	public:
		SpvDebugFunction(SpvId InName, SpvId InTypeDesc, int32 InLine, SpvId InParent)
		: SpvInstructionBase(SpvDebugInfo100::DebugFunction)
		, Name(InName), TypeDesc(InTypeDesc)
		, Line(InLine), Parent(InParent)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		int32 GetLineNumber() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		int32 Line;
		SpvId Parent;
	};

	class SpvDebugScope : public SpvInstructionBase<SpvDebugScope>
	{
	public:
		SpvDebugScope(SpvId InScope) : SpvInstructionBase(SpvDebugInfo100::DebugScope)
		, Scope(InScope)
		{}
		
		SpvId GetScope() const { return Scope; }
		
	private:
		SpvId Scope;
	};

	class SpvDebugLine : public SpvInstructionBase<SpvDebugLine>
	{
	public:
		SpvDebugLine(SpvId InLineStart) : SpvInstructionBase(SpvDebugInfo100::DebugLine)
		, LineStart(InLineStart)
		{}
		
		SpvId GetLineStart() const { return LineStart; }
		
	private:
		SpvId LineStart;
	};

	class SpvDebugDeclare : public SpvInstructionBase<SpvDebugDeclare>
	{
	public:
		SpvDebugDeclare(SpvId InVarDesc, SpvId InVarId) : SpvInstructionBase(SpvDebugInfo100::DebugDeclare)
		, VarDesc(InVarDesc)
		, VarId(InVarId)
		{}
		
		SpvId GetVarDesc() const { return VarDesc; }
		SpvId GetVarId() const { return VarId; }
		
	private:
		SpvId VarDesc;
		SpvId VarId;
	};

	class SpvDebugLocalVariable : public SpvInstructionBase<SpvDebugLocalVariable>
	{
	public:
		SpvDebugLocalVariable(SpvId InName, SpvId InTypeDesc, int32 InLine, SpvId InParent)
		: SpvInstructionBase(SpvDebugInfo100::DebugLocalVariable)
		, Name(InName), TypeDesc(InTypeDesc)
		, Line(InLine), Parent(InParent)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		int32 GetLineNumber() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		int32 Line;
		SpvId Parent;
	};

	class SpvDebugGlobalVariable : public SpvInstructionBase<SpvDebugGlobalVariable>
	{
	public:
		SpvDebugGlobalVariable(SpvId InName, SpvId InTypeDesc, int32 InLine, SpvId InParent, SpvId InVar)
		: SpvInstructionBase(SpvDebugInfo100::DebugGlobalVariable)
		, Name(InName), TypeDesc(InTypeDesc), Line(InLine), Parent(InParent), Var(InVar)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		int32 GetLineNumber() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		SpvId GetVarId() const { return Var; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		int32 Line;
		SpvId Parent;
		SpvId Var;
	};
}
