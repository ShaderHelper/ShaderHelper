#include "CommonHeader.h"
#include "ShPluginManager.h"
#include "Common/Path/PathHelper.h"
#include "App/App.h"

#undef check
THIRD_PARTY_INCLUDES_START
__pragma(warning(disable: 4191))
__pragma(warning(disable: 4686))
#include "pybind11/embed.h"
#include "python.h"
THIRD_PARTY_INCLUDES_END
namespace py = pybind11;

using namespace FW;

DECLARE_LOG_CATEGORY_EXTERN(LogPy, Log, All);
DEFINE_LOG_CATEGORY(LogPy);

class RedirectorStdout
{
public:
	void write(std::string text)
	{
		SH_LOG(LogPy, Display, TEXT("%s"), UTF8_TO_TCHAR(text.c_str()));
	}
	void flush() {}
};


class RedirectorStderr
{
public:
	void write(std::string text)
	{
		SH_LOG(LogPy, Error, TEXT("%s"), UTF8_TO_TCHAR(text.c_str()));
	}
	void flush() {}
};

PYBIND11_EMBEDDED_MODULE(sh_redirector, m)
{
	py::class_< RedirectorStdout >(m, "RedirectorStdout")
		.def(py::init<>())
		.def("write", &RedirectorStdout::write)
		.def("flush", &RedirectorStdout::flush);

	py::class_< RedirectorStderr >(m, "RedirectorStderr")
		.def(py::init<>())
		.def("write", &RedirectorStderr::write)
		.def("flush", &RedirectorStderr::flush);
}

namespace SH
{
	void InitPy()
	{
		if(!Py_IsInitialized())
		{
			FString PyHome = FPaths::GetPath(FPlatformProcess::ExecutablePath()) + FString("\\Python");
			FString DefaultModuleSearchPaths = FString::Format(TEXT("{0};{0}\\python311.zip;{1};"), { PyHome, PathHelper::PluginDir() });

			PyConfig config;
			PyConfig_InitPythonConfig(&config);
			PyConfig_SetString(&config, &config.pythonpath_env, *DefaultModuleSearchPaths);
			py::initialize_interpreter(&config, 0, nullptr, false);

			py::exec(R"(
import sys
import sh_redirector
sys.stdout = sh_redirector.RedirectorStdout()
sys.stderr = sh_redirector.RedirectorStderr()
		)");
		}
	}

	void Finalize()
	{
		if(Py_IsInitialized())
		{
			py::finalize_interpreter();
		}
	}

	ShPluginManager::ShPluginManager()
	{
		InitPy();
		TArray<FString> PluginFolders;
		IFileManager::Get().FindFiles(PluginFolders, *(PathHelper::PluginDir() / TEXT("*")), false, true);

		TArray<FString> ActivePlugins;
		Editor::GetEditorConfig()->GetArray(TEXT("Common"), TEXT("ActivePlugins"), ActivePlugins);

		for(const auto& PluginFolder : PluginFolders)
		{
			try
			{
				py::module_ PluginModule = py::module_::import(TCHAR_TO_UTF8(*PluginFolder));
				py::dict PluginMetaData = PluginModule.attr("sh_info");
				ShPlugin Plugin{ .Path = PathHelper::PluginDir() / PluginFolder };
				for (auto MetaItem : PluginMetaData)
				{
					std::string MetaKey = MetaItem.first.cast<std::string>();
					if (MetaKey == "name")
					{
						Plugin.Name = MetaItem.second.cast<std::string>().c_str();
					}
					else if (MetaKey == "author")
					{
						Plugin.Author = MetaItem.second.cast<std::string>().c_str();
					}
					else if (MetaKey == "version")
					{
						Plugin.Major = MetaItem.second.cast<py::tuple>()[0].cast<int>();
						Plugin.Minor = MetaItem.second.cast<py::tuple>()[1].cast<int>();
					}
					else if (MetaKey == "shaderhelper")
					{
						Plugin.ShMajor = MetaItem.second.cast<py::tuple>()[0].cast<int>();
						Plugin.ShMinor = MetaItem.second.cast<py::tuple>()[1].cast<int>();
					}
					else if (MetaKey == "description")
					{
						Plugin.Desc = MetaItem.second.cast<std::string>().c_str();
					}
				}
				Plugin.bActive = ActivePlugins.Contains(Plugin.Name);
				Plugins.Add(MoveTemp(Plugin));
			}
			catch (py::error_already_set& e) 
			{
				SH_LOG(LogPy, Error, TEXT("failed to find the plugin(%s):"), *PluginFolder);
				py::module::import("traceback").attr("print_exception")(e.type(), e.value() ? e.value() : py::none(), e.trace() ? e.trace() : py::none());
			}
		}
	}

	void ShPluginManager::RegisterActivePlugins()
	{
		for(const auto& Plugin : Plugins)
		{
			if(Plugin.bActive)
			{
				RegisterPlugin(Plugin, false);
			}
		}
	}

	bool ShPluginManager::RegisterPlugin(const ShPlugin& InPlugin, bool IgnoreCache)
	{
		if (IgnoreCache)
		{
			Finalize();
			InitPy();
		}

		try
		{
			FString AddLibPath = FString::Printf(TEXT("import sys\nsys.path.append('%s')"), *(InPlugin.Path / "Lib"));
			py::exec(TCHAR_TO_UTF8(*AddLibPath));
			py::module_ PluginModule = py::module_::import(TCHAR_TO_UTF8(*FPaths::GetBaseFilename(InPlugin.Path)));
			PluginModule.attr("register")();
		}
		catch (py::error_already_set& e)
		{
			SH_LOG(LogPy, Error, TEXT("failed to register the plugin(%s):"), *InPlugin.Name);
			py::module::import("traceback").attr("print_exception")(e.type(), e.value() ? e.value() : py::none(), e.trace() ? e.trace() : py::none());
			return false;
		}
		return true;
	}

	void ShPluginManager::UnregisterPlugin(const ShPlugin& InPlugin)
	{
		try
		{
			py::exec(R"(
import sys
sys.path.pop()
			)");
			py::module_ PluginModule = py::module_::import(TCHAR_TO_UTF8(*FPaths::GetBaseFilename(InPlugin.Path)));
			PluginModule.attr("unregister")();
		}
		catch (py::error_already_set& e)
		{
			SH_LOG(LogPy, Error, TEXT("failed to unregister the plugin(%s):"), *InPlugin.Name);
			py::module::import("traceback").attr("print_exception")(e.type(), e.value() ? e.value() : py::none(), e.trace() ? e.trace() : py::none());
		}
	}

}
