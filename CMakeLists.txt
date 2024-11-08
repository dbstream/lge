# CMakeLists for LGE (Lightweight Game Engine)
# Copyright (C) 2024  dbstream

cmake_minimum_required (VERSION 3.25)
project (lge LANGUAGES CXX)

if (NOT UNIX)
	message (FATAL_ERROR "The only supported platform right now ix Unix.")
endif ()

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (BUILD_SHARED_LIBS OFF)

add_subdirectory (vendor/glm)
add_subdirectory (vendor/vkfw)

add_library (lge)
target_include_directories (lge PUBLIC include)

target_link_libraries (lge glm::glm-header-only)
target_link_libraries (lge vkfw)

target_sources (lge PRIVATE
	"LGE/Application.cc"
	"LGE/Descriptor.cc"
	"LGE/GPUMemory.cc"
	"LGE/Init.cc"
	"LGE/Log.cc"
	"LGE/Pipeline.cc"
	"LGE/Vulkan.cc"
	"LGE/VulkanMemoryAllocator.cc"
	"LGE/Window.cc"
)

if (PROJECT_IS_TOP_LEVEL)
	add_executable (example)
	target_link_libraries (example lge)
	target_sources (example PRIVATE
		"Example/Main.cc"
	)
endif ()
