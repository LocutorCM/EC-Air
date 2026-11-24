cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Min macOS")

project(ECAir VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ECAir
    COMPANY_NAME "EC"
    BUNDLE_ID com.ec.ecair
    PLUGIN_MANUFACTURER_CODE Ecaa
    PLUGIN_CODE Ecar
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "EC Air"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(ECAir)

target_sources(ECAir PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ECAir PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ECAir
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
