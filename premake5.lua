workspace("CloneCraft")
	configurations({"Debug",  "Release"})
	platforms({"Win64"})
	architecture("x64")
	defines("UNICODE")
	location("build")
	flags("OmitDefaultLibrary")

	includedirs({
		"$(BOOST_SDK)",
		"includes/Gateware",
		"includes/LibNoise/include",
		"includes/stb-master",
		"$(VULKAN_SDK)/include"
	})

	links({
		"$(VULKAN_SDK)/Lib/vulkan-1.lib"
	})

	project("CloneCraft")
	kind("ConsoleApp")
	language("C++")

		postbuildcommands({
			"{RMDIR} %{cfg.buildtarget.directory}textures",
			"{MKDIR} %{cfg.buildtarget.directory}textures",
			"{COPYDIR} ../textures %{prj.location}textures", -- for running with the debugger attached in VS
			"{COPYDIR} ../textures %{cfg.buildtarget.directory}textures" -- for running without the debugger attached
		})

		files{
			"src/*.*",
			"textures/*.*"
		}

		filter("platforms:Win64")
			system("windows")
			defines({
				"_WIN32"
			})

		filter("configurations:Debug")
			defines({
				"_DEBUG"
			})
			links({
				"$(VULKAN_SDK)/Lib/shaderc_combinedd.lib",
				"lib/LibNoise/libnoise-d.lib"
			})

		filter("configurations:Release")
			optimize ("Full")
			defines({
				"NDEBUG"
			})
			links({
				"$(VULKAN_SDK)/Lib/shaderc_combined.lib",
				"lib/LibNoise/libnoise-r.lib"
			})
