// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Filesystem.hpp"

#include "Core/Core.hpp"
#include "Core/Win32/Win32Configs.hpp"
#include "Core/Exception.hpp"

#include <algorithm>

namespace Recluse {


Filesystem& gFilesystem() 
{
  return Filesystem::instance();
}


void Filesystem::onStartUp()
{
  if (!gCore().isActive()) {
    R_DEBUG(rError, "Core is not active! Activate first!\n");
    return;
  }

  char buffer[MAX_PATH];
  GetModuleFileName(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  m_CurrentDirectoryPath = std::string(buffer).substr(0, pos);
  std::replace(m_CurrentDirectoryPath.begin(), m_CurrentDirectoryPath.end(), '\\', '/');  
}


void Filesystem::onShutDown()
{
}


const TChar* Filesystem::CurrentAppDirectory()
{
  return m_CurrentDirectoryPath.data();
}


FilesystemResult Filesystem::ReadFrom(const TChar* filepath, FileHandle* buf)
{
  HANDLE fileH = CreateFile(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fileH == INVALID_HANDLE_VALUE) {
    return FilesystemResult_NotFound;
  }
  DWORD sz = GetFileSize(fileH, NULL);
  DWORD bytesRead;
  buf->Buf = new TChar[sz + 1];
  buf->Sz = sz;
  ReadFile(fileH, buf->Buf, sz, &bytesRead, NULL);
  CloseHandle(fileH);
  buf->Buf[sz] = '\0';
  return FilesystemResult_Success;
}
} // Recluse