#include "CommonHeader.h"
#include "SpirvPixelVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	SpvVmPixelVisitor::SpvVmPixelVisitor(SpvVmPixelContext& InPixelContext)
	: PixelContext(InPixelContext)
	{}

	SpvVmContext& SpvVmPixelVisitor::GetActiveContext()
	{
		return PixelContext.Quad[PixelContext.CurActiveIndex];
	}

	void SpvVmPixelVisitor::ParseQuad(int32 QuadIndex, int32 StopIndex)
	{
		PixelContext.CurActiveIndex = QuadIndex;
		SpvThreadState& CurPixelThread = PixelContext.Quad[QuadIndex].ThreadState;
		while(CurPixelThread.InstIndex < Insts->Num())
		{
		   CurPixelThread.NextInstIndex = CurPixelThread.InstIndex + 1;
		   if(StopIndex == CurPixelThread.InstIndex)
		   {
			   break;
		   }
		   
		   (*Insts)[CurPixelThread.InstIndex]->Accpet(this);
		   if(AnyError || CurPixelThread.StackFrames.IsEmpty())
		   {
			   break;
		   }
		   CurPixelThread.InstIndex = CurPixelThread.NextInstIndex;
		}
	}

	void SpvVmPixelVisitor::Visit(SpvOpDPdx* Inst)
	{
		
	}

	void SpvVmPixelVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts)
	{
		SpvVmVisitor::Parse(Insts);
		
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmContext& Context = PixelContext.Quad[QuadIndex];
			
			//Get entry point loc
			int32 InstIndex = GetInstIndex(Context.EntryPoint);
			
			//Init global external/location variable
			for(auto& [Id, Var] : Context.GlobalVariables)
			{
				int32 Location = INDEX_NONE;
				std::optional<SpvBuiltIn> BuiltIn;
				int32 SetNumber = INDEX_NONE;
				int32 BindingNumber = INDEX_NONE;
				TArray<SpvDecoration> Decorations;
				Context.Decorations.MultiFind(Id, Decorations);
				for(const auto& Decoration : Decorations)
				{
					if(Decoration.Kind == SpvDecorationKind::DescriptorSet)
					{
						SetNumber = Decoration.DescriptorSet.Number;
					}
					else if(Decoration.Kind == SpvDecorationKind::Binding)
					{
						BindingNumber = Decoration.Binding.Number;
					}
					else if(Decoration.Kind == SpvDecorationKind::BuiltIn)
					{
						BuiltIn = Decoration.BuiltIn.Attribute;
					}
					else if(Decoration.Kind == SpvDecorationKind::Location)
					{
						Location = Decoration.Location.Number;
					}
				}
				if(SetNumber != INDEX_NONE && BindingNumber != INDEX_NONE ;
				   SpvVmBinding* VmBinding = PixelContext.Bindings.FindByPredicate([&](const SpvVmBinding& InItem) { return InItem.Binding == BindingNumber && InItem.DescriptorSet == SetNumber; }))
				{
					auto& Storage = std::get<SpvObject::External>(Var.Storage);
					Storage.Resource = VmBinding->Resource;
					if(VmBinding->Resource->GetType() == GpuResourceType::Buffer)
					{
						GpuBuffer* Buffer = static_cast<GpuBuffer*>(VmBinding->Resource);
						uint8* Data = (uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
						Storage.Value = {Data, Buffer->GetByteSize()};
						GGpuRhi->UnMapGpuBuffer(Buffer);
					}
					Var.Initialized = true;
				}
				else if(BuiltIn && Context.ThreadState.BuiltInInput.contains(BuiltIn.value()))
				{
					std::get<SpvObject::Internal>(Var.Storage).Value = Context.ThreadState.BuiltInInput[BuiltIn.value()];
					Var.Initialized = true;
				}
				else if(auto Search = Context.ThreadState.LocationInput.find(Location) ; Search != Context.ThreadState.LocationInput.end())
				{
					auto LocationValue = Search->second;
					std::get<SpvObject::Internal>(Var.Storage).Value = LocationValue;;
					Var.Initialized = true;
				}
			}
			
			Context.ThreadState.InstIndex = InstIndex;
			Context.ThreadState.RecordedInfo.AllVariables = Context.GlobalVariables;
			Context.ThreadState.RecordedInfo.LineDebugStates.SetNum(1);
			Context.ThreadState.StackFrames.SetNum(1);
		}

		ParseQuad(PixelContext.DebugIndex);
	}
}
