cmake_minimum_required(VERSION 3.22)
project(Wander VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")   # CI builds "arm64;x86_64"
endif()

option(WANDER_COPY_PLUGIN "Copy the built plugins into the system plugin folders" ON)

# JUCE: a local copy in external/JUCE if there is one (fast, offline), otherwise fetched (CI).
if(EXISTS "${CMAKE_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# The sound has no JUCE in it, so the offline test builds without the plugin around it.
add_library(wander_core STATIC src/Synth.cpp)
target_include_directories(wander_core PUBLIC src)

set(WANDER_SOURCES src/PluginProcessor.cpp src/PluginEditor.cpp)

juce_add_plugin(Wander
    COMPANY_NAME "Pavel Konrad"
    BUNDLE_ID "dev.roboti.wander"
    PLUGIN_MANUFACTURER_CODE Pkon
    PLUGIN_CODE Wndr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Wander"
    DESCRIPTION "Two oscillators, a filter and some drift"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${WANDER_COPY_PLUGIN})

target_sources(Wander PRIVATE ${WANDER_SOURCES})
target_compile_definitions(Wander PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Wander
    PRIVATE wander_core juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Renders notes through the synth to a WAV, without JUCE: checks pitch and level against the JSFX.
add_executable(wander_render test/render.cpp)
target_link_libraries(wander_render PRIVATE wander_core)

# Draws the editor to a PNG, without a host: checks the GUI.
juce_add_console_app(wander_snapshot PRODUCT_NAME "wander_snapshot")
target_sources(wander_snapshot PRIVATE test/snapshot.cpp ${WANDER_SOURCES})
target_compile_definitions(wander_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(wander_snapshot
    PRIVATE wander_core juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
