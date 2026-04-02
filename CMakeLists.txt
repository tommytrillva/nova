cmake_minimum_required(VERSION 3.22)
project(VoidSynth VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Generate audio resources if they don't exist
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Resources/Wavetables/BasicSaw.wav")
        message(STATUS "Generating audio resources...")
        execute_process(
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/generate_audio_resources.py"
                    --output-dir "${CMAKE_SOURCE_DIR}/Resources"
            RESULT_VARIABLE AUDIO_GEN_RESULT
        )
        if(NOT AUDIO_GEN_RESULT EQUAL 0)
            message(WARNING "Audio resource generation failed (exit code ${AUDIO_GEN_RESULT}). Build will continue without audio resources.")
        endif()
    endif()
endif()

# Collect audio resource files
file(GLOB WAVETABLE_FILES "${CMAKE_SOURCE_DIR}/Resources/Wavetables/*.wav")
file(GLOB GRAIN_FILES "${CMAKE_SOURCE_DIR}/Resources/GrainSources/*.wav")
file(GLOB PRESET_RAGE_FILES "${CMAKE_SOURCE_DIR}/Resources/Presets/Rage/*.json")
file(GLOB PRESET_GRAN_FILES "${CMAKE_SOURCE_DIR}/Resources/Presets/Granular/*.json")
file(GLOB PRESET_PAD_FILES "${CMAKE_SOURCE_DIR}/Resources/Presets/Pads/*.json")

set(ALL_RESOURCE_FILES
    ${WAVETABLE_FILES}
    ${GRAIN_FILES}
    ${PRESET_RAGE_FILES}
    ${PRESET_GRAN_FILES}
    ${PRESET_PAD_FILES}
)

# Binary data target for audio resources
if(ALL_RESOURCE_FILES)
    juce_add_binary_data(AudioResources
        HEADER_NAME "BinaryData.h"
        NAMESPACE BinaryData
        SOURCES ${ALL_RESOURCE_FILES}
    )
else()
    # Create a minimal binary data target with a placeholder
    file(WRITE "${CMAKE_BINARY_DIR}/placeholder.txt" "placeholder")
    juce_add_binary_data(AudioResources
        HEADER_NAME "BinaryData.h"
        NAMESPACE BinaryData
        SOURCES "${CMAKE_BINARY_DIR}/placeholder.txt"
    )
endif()

# Plugin target
juce_add_plugin(VoidSynth
    COMPANY_NAME "TommyTrillAI"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Ttai
    PLUGIN_CODE Void
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "VOIDSYNTH"
)

target_sources(VoidSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/OscillatorEngine.cpp
    Source/SynthVoice.cpp
    Source/SubOscillator.cpp
    Source/MultiFilter.cpp
    Source/DistortionProcessor.cpp
    Source/GranularEngine.cpp
    Source/FXChain.cpp
    Source/MacroSystem.cpp
    Source/PresetManager.cpp
    Source/LookAndFeel.cpp
)

target_compile_definitions(VoidSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(VoidSynth
    PRIVATE
        AudioResources
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
