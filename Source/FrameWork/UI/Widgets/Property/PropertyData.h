#pragma once
#include "Renderer/RenderResource/UniformBuffer.h"

namespace FRAMEWORK
{
	class PropertyData : public TSharedFromThis<PropertyData>
	{
	public:
		PropertyData(FString InName) : DisplayName(MoveTemp(InName))
		{}
		virtual ~PropertyData() = default;


		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		void GetChildren(TArray<TSharedRef<PropertyData>>& OutChildren) const { OutChildren = Children; };
		int32 GetChildrenNum() const { return Children.Num(); }
		FString GetDisplayName() const { return DisplayName; }
		PropertyData* GetParent() const { return Parent; }

		void AddChild(TSharedRef<PropertyData> InChild) 
		{ 
			InChild->Parent = this;
			Children.Add(InChild); 
		}

		void Remove()
		{
			check(Parent);
			Parent->RemoveChild(AsShared());
		}

		void RemoveChild(TSharedRef<PropertyData> InChild)
		{
			Children.Remove(InChild);
		}

		virtual void AddToUniformBuffer(UniformBufferBuilder& Builder) const {}
		virtual void UpdateUniformBuffer(UniformBuffer* Buffer) const {}

	protected:
		FString DisplayName;
		PropertyData* Parent = nullptr;
		TArray<TSharedRef<PropertyData>> Children;
	};

	class PropertyCategory : public PropertyData
	{
	public:
		PropertyCategory(FString InName)
			: PropertyData(MoveTemp(InName))
		{}

		void SetAddMenuWidget(TSharedPtr<SWidget> InWidget) { AddMenuWidget = MoveTemp(InWidget); }
		bool IsRootCategory() const { return Parent == nullptr; }

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

	private:
		TSharedPtr<SWidget> AddMenuWidget;
	};

	template<typename T>
	class PropertyItem : public PropertyData
	{
	public:
		PropertyItem(FString InName, const TAttribute<T>& InValue)
			: PropertyData(MoveTemp(InName))
			, ValueAttribute(InValue)
		{}

		void SetEnabled(bool Enabled) { IsEnabled = Enabled; }
		void SetOnValueChanged(const TFunction<void(T)>& ValueChanged) { OnValueChanged = ValueChanged; }
		void SetOnDisplayNameChanged(const TFunction<void(const FString&)> DisplayNameChanged) { OnDisplayNameChanged = DisplayNameChanged; }
		void AddToUniformBuffer(UniformBufferBuilder& Builder) const override;
		virtual void UpdateUniformBuffer(UniformBuffer* Buffer) const override;
		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override;

	private:
		TAttribute<T> ValueAttribute;
		bool IsEnabled = true;
		TFunction<void(T)> OnValueChanged;
		TFunction<void(const FString&)> OnDisplayNameChanged;
	};

}

#include "PropertyData.hpp"


