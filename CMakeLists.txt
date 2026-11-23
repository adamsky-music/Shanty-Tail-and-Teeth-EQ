cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS")
endif()
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

project(ShantyTailAndTeethEQ VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_binary_data(ShantyTailTeethAssets
    SOURCES
        Assets/shanti_v3_sheet.png
        Assets/adamsky_logo.png
)

juce_add_plugin(ShantyTailAndTeethEQ
    COMPANY_NAME "Adamsky Music"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE AdMs
    PLUGIN_CODE ShTt
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Shanty Tail and Teeth EQ"
    BUNDLE_ID "com.adamskymusic.shantytailteetheq"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    VST3_CATEGORIES "Fx" "EQ"
)

juce_generate_juce_header(ShantyTailAndTeethEQ)

target_sources(ShantyTailAndTeethEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/StructuralSplitDSP.h
        Source/BandEQAndSatDSP.h
        Source/SpectrumAnalyzerComponent.h
        Source/CustomLookAndFeel.h
        Source/ShantyMascot.h
)

target_compile_definitions(ShantyTailAndTeethEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ShantyTailAndTeethEQ
    PRIVATE
        ShantyTailTeethAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
