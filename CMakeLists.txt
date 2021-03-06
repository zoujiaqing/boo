cmake_minimum_required(VERSION 3.0)
project(boo)

if (NOT MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-narrowing")
endif()

if (NOT TARGET logvisor)
    add_subdirectory(logvisor)
endif()

set(WITH_LSR_BINDINGS OFF)
set(BUILD_TESTS OFF)
set(BUILD_SHARED_LIBS OFF)
set(SOXR_SILENT ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/soxr/cmake/Modules")
add_subdirectory(soxr)

set(BOO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE PATH "boo include path" FORCE)

include_directories(include ${LOGVISOR_INCLUDE_DIR})

if(NOT GEKKO AND NOT CAFE)
list(APPEND PLAT_SRCS
    lib/graphicsdev/GL.cpp
    lib/graphicsdev/glew.c)

list(APPEND PLAT_HDRS
     include/boo/graphicsdev/GLSLMacros.hpp
     include/boo/graphicsdev/GL.hpp
     include/boo/graphicsdev/Vulkan.hpp
     include/boo/graphicsdev/VulkanDispatchTable.hpp)
endif()

if(WIN32)

  unset(VULKAN_SDK_DIRS CACHE)
  get_filename_component(VULKAN_SDK_DIRS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\LunarG\\VulkanSDK;VK_SDK_PATHs]" ABSOLUTE CACHE)
  if (NOT ${VULKAN_SDK_DIRS} STREQUAL "/registry")
    message(STATUS "Enabling Vulkan support")
    list(GET VULKAN_SDK_DIRS 0 VULKAN_SDK_DIR)
    include_directories("${VULKAN_SDK_DIR}/Include")
    list(APPEND _BOO_SYS_DEFINES -DBOO_HAS_VULKAN=1)
    list(APPEND _BOO_SYS_INCLUDES "${VULKAN_SDK_DIR}/Include")
    list(APPEND PLAT_SRCS lib/graphicsdev/Vulkan.cpp
         lib/graphicsdev/VulkanDispatchTable.cpp)
  endif()

  list(APPEND PLAT_SRCS
       lib/win/ApplicationWin32.cpp
       lib/win/WindowWin32.cpp
       lib/win/Win32Common.hpp
       lib/inputdev/HIDListenerWinUSB.cpp
       lib/inputdev/HIDDeviceWinUSB.cpp
       lib/graphicsdev/D3D11.cpp
       lib/graphicsdev/D3D12.cpp
       lib/audiodev/WASAPI.cpp)

  list(APPEND PLAT_HDRS
        include/boo/graphicsdev/D3D.hpp)

  list(APPEND _BOO_SYS_DEFINES -DUNICODE -D_UNICODE)

  list(APPEND _BOO_SYS_LIBS Winusb opengl32 Setupapi Imm32 Winmm Shlwapi)
elseif(APPLE)
  list(APPEND PLAT_SRCS
       lib/mac/ApplicationCocoa.mm
       lib/mac/WindowCocoa.mm
       lib/mac/CocoaCommon.hpp
       lib/inputdev/HIDListenerIOKit.cpp
       lib/inputdev/HIDDeviceIOKit.cpp
       lib/graphicsdev/Metal.mm
       lib/audiodev/AQS.cpp)
  set_source_files_properties(lib/mac/ApplicationCocoa.mm
                              lib/mac/WindowCocoa.mm
                              lib/graphicsdev/Metal.mm
                              PROPERTIES COMPILE_FLAGS -fobjc-arc)

  list(APPEND PLAT_HDRS
       include/boo/graphicsdev/Metal.hpp)

  find_library(APPKIT_LIBRARY AppKit)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(OPENGL_LIBRARY OpenGL)
  unset(BOO_HAS_METAL CACHE)
  if (NOT CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER 10.10)
    set(BOO_HAS_METAL ON CACHE BOOL "Metal is available in this OS X version" FORCE)
    find_library(METAL_LIBRARY Metal)
    list(APPEND _BOO_SYS_DEFINES -DBOO_HAS_METAL=1)
  else()
    set(METAL_LIBRARY "")
  endif()
  find_library(QUARTZCORE_LIBRARY QuartzCore)
  find_library(COREVIDEO_LIBRARY CoreVideo)
  find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
  find_library(COREAUDIO_LIBRARY CoreAudio)
  find_library(COREMIDI_LIBRARY CoreMIDI)
  list(APPEND _BOO_SYS_LIBS ${APPKIT_LIBRARY} ${IOKIT_LIBRARY} ${OPENGL_LIBRARY} ${METAL_LIBRARY}
                            ${QUARTZCORE_LIBRARY} ${COREVIDEO_LIBRARY} ${AUDIOTOOLBOX_LIBRARY}
                            ${COREAUDIO_LIBRARY} ${COREMIDI_LIBRARY})
else(NOT GEKKO)
  list(APPEND PLAT_SRCS
       lib/x11/XlibCommon.hpp
       lib/x11/ApplicationUnix.cpp
       lib/x11/ApplicationXlib.hpp
       lib/x11/ApplicationWayland.hpp
       lib/x11/WindowXlib.cpp
       lib/x11/WindowWayland.cpp
       lib/graphicsdev/GL.cpp
       lib/graphicsdev/GLX.cpp
       lib/graphicsdev/glew.c
       lib/audiodev/ALSA.cpp)

  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(dbus_pkg QUIET libdbus dbus-1)
  endif()

  find_path(DBUS_INCLUDE_DIR
    NAMES
    dbus/dbus.h
    HINTS
    ${dbus_pkg_INCLUDE_DIRS}
    PATH_SUFFIXES
    include/
    include/dbus-1.0/
    dbus-1.0/)

  find_path(DBUS_ARCH_INCLUDE_DIR
    NAMES
    dbus/dbus-arch-deps.h
    HINTS
    ${dbus_pkg_INCLUDE_DIRS}
    PATHS
    # TODO use CMAKE_SYSTEM_PROCESSOR or similar?
    /usr/lib/dbus-1.0/include
    /usr/local/lib/dbus-1.0/include
    /usr/lib/x86_64-linux-gnu/dbus-1.0/include/
    PATH_SUFFIXES
    dbus-1.0/include/)

  find_library(DBUS_LIBRARY
    NAMES
    dbus dbus-1
    HINTS
    ${dbus_pkg_LIBRARY_DIRS}
    PATH_SUFFIXES
    lib
    lib32
    lib64)

  if(DBUS_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR "Unix build of boo requires dbus")
  endif()

  include_directories(${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})
  list(APPEND _BOO_SYS_LIBS X11 Xi GL asound ${DBUS_LIBRARY} pthread)

  message(STATUS "Enabling Vulkan support")
  list(APPEND _BOO_SYS_DEFINES -DBOO_HAS_VULKAN=1)
  list(APPEND _BOO_SYS_LIBS xcb X11-xcb dl)
  list(APPEND PLAT_SRCS lib/graphicsdev/Vulkan.cpp
       lib/graphicsdev/VulkanDispatchTable.cpp)

  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
      list(APPEND PLAT_SRCS
          lib/inputdev/HIDListenerUdev.cpp
          lib/inputdev/HIDDeviceUdev.cpp)
      list(APPEND _BOO_SYS_LIBS udev)
  else()
      list(APPEND PLAT_SRCS
          lib/inputdev/HIDListenerBSD.cpp
          lib/inputdev/HIDDeviceBSD.cpp)
  endif()

endif()

# For some reason, clang takes forever if glew.c is not built with -Os
if(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  set_source_files_properties(lib/graphicsdev/glew.c PROPERTIES COMPILE_FLAGS -Os)
endif()

# Empty link args for boo's use
function(glslang_set_link_args TARGET)
endfunction(glslang_set_link_args)

add_subdirectory(glslang/glslang)
add_subdirectory(glslang/hlsl)
add_subdirectory(glslang/OGLCompilersDLL)
add_subdirectory(glslang/SPIRV)
add_subdirectory(glslang/StandAlone)

target_include_directories(glslang-default-resource-limits
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang
)

list(APPEND _BOO_SYS_LIBS glslang HLSL soxr OSDependent OGLCompiler SPIRV glslang-default-resource-limits)

set(BOO_SYS_LIBS ${_BOO_SYS_LIBS} CACHE PATH "boo system libraries" FORCE)
set(BOO_SYS_DEFINES ${_BOO_SYS_DEFINES} CACHE PATH "boo system defines" FORCE)
set(BOO_SYS_INCLUDES ${_BOO_SYS_INCLUDES} CACHE PATH "boo system includes" FORCE)

add_definitions(${_BOO_SYS_DEFINES})
include_directories(include glslang soxr/src)

add_library(boo
            lib/inputdev/DeviceBase.cpp include/boo/inputdev/DeviceBase.hpp
            lib/inputdev/CafeProPad.cpp include/boo/inputdev/CafeProPad.hpp
            lib/inputdev/RevolutionPad.cpp include/boo/inputdev/RevolutionPad.hpp
            lib/inputdev/DolphinSmashAdapter.cpp include/boo/inputdev/DolphinSmashAdapter.hpp
            lib/inputdev/DualshockPad.cpp include/boo/inputdev/DualshockPad.hpp
            lib/inputdev/GenericPad.cpp include/boo/inputdev/GenericPad.hpp
            lib/inputdev/DeviceSignature.cpp include/boo/inputdev/DeviceSignature.hpp
            lib/inputdev/DeviceFinder.cpp include/boo/inputdev/DeviceFinder.hpp
            lib/inputdev/IHIDDevice.hpp
            lib/audiodev/WAVOut.cpp
            lib/audiodev/AudioMatrix.hpp
            #lib/audiodev/AudioMatrix.cpp
            lib/audiodev/AudioMatrixSSE.cpp
            lib/audiodev/AudioVoiceEngine.hpp
            lib/audiodev/AudioVoiceEngine.cpp
            lib/audiodev/AudioVoice.hpp
            lib/audiodev/AudioVoice.cpp
            lib/audiodev/AudioSubmix.hpp
            lib/audiodev/AudioSubmix.cpp
            lib/audiodev/MIDIEncoder.cpp
            lib/audiodev/MIDIDecoder.cpp
            lib/audiodev/MIDICommon.hpp
            lib/audiodev/MIDICommon.cpp
            include/boo/inputdev/IHIDListener.hpp
            include/boo/IGraphicsContext.hpp
            include/boo/graphicsdev/IGraphicsDataFactory.hpp
            include/boo/graphicsdev/IGraphicsCommandQueue.hpp
            include/boo/audiodev/IAudioSubmix.hpp
            include/boo/audiodev/IAudioVoice.hpp
            include/boo/audiodev/IMIDIPort.hpp
            include/boo/audiodev/IMIDIReader.hpp
            include/boo/audiodev/MIDIEncoder.hpp
            include/boo/audiodev/MIDIDecoder.hpp
            include/boo/audiodev/IAudioVoiceEngine.hpp
            include/boo/IWindow.hpp
            include/boo/IApplication.hpp
            include/boo/ThreadLocalPtr.hpp
            include/boo/DeferredWindowEvents.hpp
            include/boo/System.hpp
            include/boo/boo.hpp
            InputDeviceClasses.cpp
            ${PLAT_SRCS}
            ${PLAT_HDRS})

add_subdirectory(test)
