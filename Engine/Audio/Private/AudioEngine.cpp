// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "AudioEngine.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


FMOD_VECTOR ToFMODVector(const Vector3& v3)
{
  FMOD_VECTOR fmodVec;
  fmodVec.x = v3.x;
  fmodVec.y = v3.y;
  fmodVec.z = v3.z;
  return fmodVec;
}


AudioResult FMODAudioEngine::initialize()
{
  AudioResult result = Audio_Success;
  FMOD_RESULT nativeResult = FMOD::Studio::System::create(&m_pStudioSystem);
  if (nativeResult != FMOD_OK) {
    result = Audio_Fail;
    return result;
  }
  
  nativeResult = m_pStudioSystem->initialize(32, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_PROFILE_ENABLE, nullptr);
  if (nativeResult != FMOD_OK) { result = Audio_Fail; return result; }

  nativeResult = m_pStudioSystem->getLowLevelSystem(&m_pSystem);
  if (nativeResult != FMOD_OK) { result = Audio_Fail; return result; }

  R_DEBUG(rNotify, "FMOD Audio has been initialized.\n");
  return result;
}


void FMODAudioEngine::cleanUp()
{
  if (m_pStudioSystem) {
    m_pStudioSystem->unloadAll();
    m_pStudioSystem->release();
    R_DEBUG(rNotify, "FMOD Audio cleaned up.\n");
  }
}


void FMODAudioEngine::updateState(r64 dt)
{
  static std::vector<ChannelMap::iterator> stoppedChannels(32);
  u32 top = 0;
  u32 sz = 0;

  for (auto it = m_channelMap.begin(), itEnd = m_channelMap.end(); it != itEnd; ++it) {
    bool bIsPlaying = false;
    it->second->isPlaying(&bIsPlaying);
    if (!bIsPlaying) {
      R_ASSERT(top < stoppedChannels.size(), "Audio stack overflow!");
      stoppedChannels[top++] = it;
      sz += 1;
    }
  }
  for (u32 i = 0; i < sz; ++i) {
    auto it = stoppedChannels[i];
    m_channelMap.erase(it);
  }

  {
    FMOD_VECTOR vpos = ToFMODVector(m_globalListenerPosition);
    FMOD_VECTOR f = ToFMODVector(m_globalListenerForward);
    FMOD_VECTOR u = ToFMODVector(m_globalListenerUp); 
    m_pSystem->set3DListenerAttributes(0, &vpos, nullptr, &f, &u);
  }
  FMOD_RESULT r = m_pStudioSystem->update();
  m_pSystem->update();
  R_ASSERT(r == FMOD_OK, "FMOD update failed.");
}

AudioId FMODAudioEngine::createAudioObject()
{
  return ~0ul;
}


void FMODAudioEngine::loadSound(const std::string& soundName, b32 is3D, b32 looping, b32 streaming) 
{
  auto it = m_soundMap.find(soundName);
  if (it != m_soundMap.end()) { return; }

  FMOD_MODE eMode = FMOD_DEFAULT; 
  eMode |= is3D ? FMOD_3D : FMOD_2D;
  eMode |= looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
  eMode |= streaming ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
  
  FMOD::Sound* pSound = nullptr;
  FMOD_RESULT result = m_pSystem->createSound(soundName.c_str(), eMode, nullptr, &pSound);
  R_ASSERT(result == FMOD_OK, "");
  R_ASSERT(pSound, "");
  m_soundMap[soundName] = pSound;
}


void FMODAudioEngine::unLoadSound(const std::string& soundName) 
{
  auto it = m_soundMap.find(soundName);
  if (it == m_soundMap.end()) { return; }

  FMOD_RESULT result = it->second->release(); 
  R_ASSERT(result == FMOD_OK, "");
  m_soundMap.erase(it);
}


u32 FMODAudioEngine::initiateSound(const std::string& soundName, const Vector3& pos, r32 volume) 
{ 
  u32 nChannelId = m_nextChannelId++;
  auto it = m_soundMap.find(soundName);
  if (it == m_soundMap.end()) {
    loadSound(soundName);
    it = m_soundMap.find(soundName);
    if (it == m_soundMap.end()) { 
      return nChannelId;
    }
  }
  
  FMOD::Channel* pChannel = nullptr;
  FMOD_RESULT result = m_pSystem->playSound(it->second, nullptr, true, &pChannel);
  R_ASSERT(result == FMOD_OK, "");
  
  if (pChannel) {
    FMOD_MODE currMode;
    it->second->getMode(&currMode);
    if (currMode & FMOD_3D) {
      FMOD_VECTOR v = ToFMODVector(pos);
      result = pChannel->set3DAttributes(&v, nullptr);
      R_ASSERT(result == FMOD_OK, "");
    }
    pChannel->setVolume(volume);
    pChannel->setPaused(false);
    m_channelMap[nChannelId] = pChannel;
  }

  return nChannelId;
}


void FMODAudioEngine::SetChannel3DPosition(u32 nChannelId, const Vector3& pos, const Vector3& vel) 
{ 
  auto it = m_channelMap.find(nChannelId);
  if (it == m_channelMap.end()) { return; }
  FMOD_VECTOR v = ToFMODVector(pos);
  FMOD_VECTOR velocity = ToFMODVector(vel);
  FMOD_RESULT result = it->second->set3DAttributes(&v, &velocity); // velocity can be set, but for now we won't.
  R_ASSERT(result == FMOD_OK, "");
}


void FMODAudioEngine::SetChannelVolume(u32 nChannelId, r32 volume) 
{ 
  auto it = m_channelMap.find(nChannelId);
  if (it == m_channelMap.end()) { return; }

  FMOD_RESULT result = it->second->setVolume(volume);
  R_ASSERT(result == FMOD_OK, "");
}


void FMODAudioEngine::setListener3DOrientation(const Vector3& pos, const Vector3& forward, const Vector3& up)
{
  m_globalListenerPosition = pos;
  m_globalListenerForward = forward;
  m_globalListenerUp = up;
}


void FMODAudioEngine::loadEventBank(const std::string& bankPath)
{
}
} // Recluse