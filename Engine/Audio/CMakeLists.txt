cmake_minimum_required(VERSION 3.0)
project("Audio")


set(AUDIO_PUBLIC_DIR         Public/Audio)
set(AUDIO_PRIVATE_DIR        Private)

string(REPLACE "\\" "/" RECLUSE_WWISE_DIR ${RECLUSE_WWISE_DIR})
set(AUDIO_WWISE_DEFAULTS_DIR ${RECLUSE_WWISE_DIR}/samples/SoundEngine)
set(WWISE_LIB_DIR            ${RECLUSE_WWISE_DIR}/x64_vc150/Release\(StaticCRT\)/lib)


include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${AUDIO_PUBLIC_DIR}
  ${RECLUSE_WWISE_DIR}/include
  ${RECLUSE_WWISE_DIR}/samples/SoundEngine/Common
  ${RECLUSE_WWISE_DIR}/samples/SoundEngine/Win32
)

set(AUDIO_DEPENDENCIES
  dinput8.lib
  dsound.lib
  dxguid.lib
)


set(AUDIO_LIBS
  ${WWISE_LIB_DIR}/AkAudioInputSource.lib
  ${WWISE_LIB_DIR}/AkCompressorFX.lib
  ${WWISE_LIB_DIR}/AkDelayFX.lib
  ${WWISE_LIB_DIR}/AkExpanderFX.lib
  ${WWISE_LIB_DIR}/AkFlangerFX.lib
  ${WWISE_LIB_DIR}/AkGainFX.lib
  ${WWISE_LIB_DIR}/AkGuitarDistortionFX.lib
  ${WWISE_LIB_DIR}/AkHarmonizerFX.lib
  ${WWISE_LIB_DIR}/AkMatrixReverbFX.lib
  ${WWISE_LIB_DIR}/AkMemoryMgr.lib
  ${WWISE_LIB_DIR}/AkMeterFX.lib
  ${WWISE_LIB_DIR}/AkMusicEngine.lib
  ${WWISE_LIB_DIR}/AkParametricEQFX.lib
  ${WWISE_LIB_DIR}/AkPeakLimiterFX.lib
  ${WWISE_LIB_DIR}/AkPitchShifterFX.lib
  ${WWISE_LIB_DIR}/AkRecorderFX.lib
  ${WWISE_LIB_DIR}/AkReflectFX.lib
  ${WWISE_LIB_DIR}/AkRoomVerbFX.lib
  ${WWISE_LIB_DIR}/AkSilenceSource.lib
  ${WWISE_LIB_DIR}/AkSineSource.lib
  ${WWISE_LIB_DIR}/AkSoundEngine.lib
  ${WWISE_LIB_DIR}/AkSpatialAudio.lib
  ${WWISE_LIB_DIR}/AkStereoDelayFX.lib
  ${WWISE_LIB_DIR}/AkStreamMgr.lib
  ${WWISE_LIB_DIR}/AkSynthOne.lib
  ${WWISE_LIB_DIR}/AktimeStretchFX.lib
  ${WWISE_LIB_DIR}/AkToneSource.lib
  ${WWISE_LIB_DIR}/AkTremoloFX.lib
  ${WWISE_LIB_DIR}/AkVorbisDecoder.lib
)


set(AUDIO_FILES
  ${AUDIO_PUBLIC_DIR}/Audio.hpp
  ${AUDIO_PUBLIC_DIR}/AudioListener.hpp
  ${AUDIO_PUBLIC_DIR}/AudioConfigs.hpp
  
  ${AUDIO_PRIVATE_DIR}/Audio.cpp
  ${AUDIO_PRIVATE_DIR}/AudioListener.cpp
  ${AUDIO_PRIVATE_DIR}/WwiseEngine.hpp
  ${AUDIO_PRIVATE_DIR}/WwiseEngine.cpp
  ${AUDIO_PRIVATE_DIR}/AudioMemAllocDefines.cpp

  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkDefaultLowLevelIODispatcher.cpp
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkDefaultLowLevelIODispatcher.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFileLocationBase.cpp
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFileLocationBase.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFilePackage.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFilePackage.cpp
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFilePackageLowLevelIO.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFilePackageLowLevelIO.inl
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFilePackageLUT.cpp 
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkFilePackageLUT.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkMultipleFileLocation.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Common/AkMultipleFileLocation.inl

  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkDefaultIOHookBlocking.cpp
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkDefaultIOHookBlocking.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkDefaultIOHookDeferred.cpp
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkDefaultIOHookDeferred.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkFileHelpers.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkFilePackageLowLevelIOBlocking.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/AkFilePackageLowLevelIODeferred.h
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/stdafx.cpp
  ${AUDIO_WWISE_DEFAULTS_DIR}/Win32/stdafx.h
)


add_library(${RECLUSE_AUDIO} STATIC
  ${AUDIO_FILES}
)


target_link_libraries(${RECLUSE_AUDIO}
  ${AUDIO_LIBS}
  ${AUDIO_DEPENDENCIES}
)

# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS AUDIO_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_AUDIO} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()

