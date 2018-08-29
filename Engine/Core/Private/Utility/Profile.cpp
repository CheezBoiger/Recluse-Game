// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Utility/Profile.hpp"
#include "Logging/Log.hpp"
#include "Thread/Threading.hpp"

#include "Exception.hpp"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace Recluse {

// Profile data stored by profile. Data is overwritten when tags are re entered.
// Determined by profile type.
std::unordered_map<ProfileTypes, 
  std::unordered_map<std::string, ProfileData > > kProfileDataMap;
std::thread     kProfileThreadHandle;
std::mutex      kProfileMapMutex;



void Profiler::StoreTimed(ProfileData& obj, const std::string& tag)
{
  kProfileDataMap[obj._type][tag] = std::move(obj);
}


ProfileData* GetProfileData(ProfileTypes type, const std::string& tag)
{
  ProfileData* profile = nullptr;
  auto it = kProfileDataMap.find(type);
  if (it == kProfileDataMap.end()) return profile;
  auto it2 = it->second.find(tag);
  if (it2 == it->second.end()) return profile;
  profile = &it2->second;
  return profile;
}


std::vector<ProfileData> Profiler::GetAll(ProfileTypes type)
{
  std::vector<ProfileData> data;
  auto& map = kProfileDataMap[type];
  for (auto d : map) {
    data.push_back(d.second);
  }
  return data;
}
} // Recluse