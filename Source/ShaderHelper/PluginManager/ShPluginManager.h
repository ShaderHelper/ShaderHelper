#pragma once


namespace SH
{
	struct ShPlugin
	{
		FString Name;
		FString Author;
		int Major, Minor;
		int ShMajor, ShMinor;
		FString Desc;
		FString Path;
		bool bActive{};
	};

	class ShPluginManager
	{
	public:
		ShPluginManager();

		void RegisterActivePlugins();
		bool RegisterPlugin(const ShPlugin& InPlugin, bool IgnoreCache = true);
		void UnregisterPlugin(const ShPlugin& InPlugin);

		const TArray<ShPlugin>& GetPlugins() const { return Plugins;  }

	private:
		TArray<ShPlugin> Plugins;
	};
}
