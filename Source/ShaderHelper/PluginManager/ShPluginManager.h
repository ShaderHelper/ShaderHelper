#pragma once
#include "PluginManager/PluginManager.h"
#include "ProjectManager/ShProjectManager.h"

namespace SH
{
	class ShPluginContext : public FW::PluginContext
	{
	public:
		static FW::Graph* GetGraph();
		static FW::ShObject* GetPropertyObject();
		static std::unique_ptr<FW::Window> GetMainWindow();
	};

	using ShPluginManager = FW::PluginManager<ShPluginContext>;

}
