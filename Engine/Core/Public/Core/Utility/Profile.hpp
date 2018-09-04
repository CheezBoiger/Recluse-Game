// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "Core/Core.hpp"
#include "Core/Types.hpp"
#include "Time.hpp"


#define R_ENABLE_PROFILE_FLAG  1


namespace Recluse { 


enum ProfileTypes {
  PROFILE_TYPES_NONE,
  PROFILE_TYPES_NORMAL,
  PROFILE_TYPES_DEBUG,
  PROFILE_TYPES_ENGINE,
  PROFILE_TYPES_RENDERER,
  PROFILE_TYPES_PHYSICS,
  PROFILE_TYPES_AUDIO,
  PROFILE_TYPES_GAME,
  PROFILE_TYPES_FILESYSTEM,
  PROFILE_TYPES_UI
};


struct ProfileData {
  std::string   _tag;
  ProfileTypes  _type;
  // start time in milliseconds.
  r32           _start;
  // end time in milliseconds.
  r32           _end;
  // total elapsed time between start and end.
  r32           _total;
};


// Profile data handler.
class Profiler {
public:
  static void StoreTimed(ProfileData& obj, const std::string& tag);
  static ProfileData* GetProfileData(ProfileTypes type, const std::string& tag);

  static std::vector<ProfileData> GetAll(ProfileTypes type);
};


class ProfileObject {
public:
  ProfileObject(ProfileTypes type = PROFILE_TYPES_NORMAL, const std::string& tag = "")
    : m_time(static_cast<r32>(Time::CurrentTime()))
    , m_type(type)
    , m_tag(tag) { }

  ~ProfileObject()
  {
    r32 _end = static_cast<r32>(Time::CurrentTime());
    ProfileData data;
    data._start = m_time;
    data._end = _end;
    data._total = _end - m_time;
    data._type = m_type;
    data._tag = m_tag;
    Profiler::StoreTimed(data, m_tag);
  }

private:
  //
  ProfileTypes  m_type;

  // start time on the global clock.
  r32           m_time;

  // tag used for meta data.
  std::string   m_tag;
};

#if R_ENABLE_PROFILE_FLAG >= 1
  #define R_TIMED_PROFILE(type, tag) ProfileObject __profile__obj(type, tag)
  #define R_TIMED_PROFILE_RENDERER() R_TIMED_PROFILE(PROFILE_TYPES_RENDERER, __FUNCTION__)
  #define R_TIMED_PROFILE_PHYSICS() R_TIMED_PROFILE(PROFILE_TYPES_PHYSICS, __FUNCTION__)
  #define R_TIMED_PROFILE_AUDIO() R_TIMED_PROFILE(PROFILE_TYPES_AUDIO, __FUNCTION__)
  #define R_TIMED_PROFILE_UI() R_TIMED_PROFILE(PROFILE_TYPES_UI, __FUNCTION__)
  #define R_TIMED_PROFILE_GAME() R_TIMED_PROFILE(PROFILE_TYPES_GAME, __FUNCTION__)
  #define R_GET_TIMED_FUNCTOR(type, tag) Profiler::GetProfileData(type, tag)
  #define R_GET_TIMED_RENDERER_FUNCTOR(tag) Profiler::GetProfileData(PROFILE_TYPES_RENDERER, tag)
  #define R_GET_TIMED_PHYSICS_FUNCTOR(tag) Profiler::GetProfileData(PROFILE_TYPES_PHYSICS, tag)
  #define R_GET_TIMED_AUDIO_FUNCTOR(tag) Profiler::GetProfileData(PROFILE_TYPES_AUDIO, tag)
  #define R_GET_TIMED_UI_FUNCTOR(tag) Profiler::GetProfileData(PROFILE_TYPES_UI, tag)
#else
  #define R_TIMED_PROFILE(type, tag) 
  #define R_TIMED_PROFILE_RENDERER() 
  #define R_TIMED_PROFILE_PHYSICS() 
  #define R_TIMED_PROFILE_AUDIO() 
  #define R_TIMED_PROFILE_UI() 
  #define R_TIMED_PROFILE_GAME() 
  #define R_GET_TIMED_FUNCTOR(type, tag) 
  #define R_GET_TIMED_RENDERER_FUNCTOR(tag) 
  #define R_GET_TIMED_PHYSICS_FUNCTOR(tag) 
  #define R_GET_TIMED_AUDIO_FUNCTOR(tag) 
  #define R_GET_TIMED_UI_FUNCTOR(tag) 
#endif
} // Recluse