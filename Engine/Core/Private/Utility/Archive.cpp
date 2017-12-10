// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Utility/Archive.hpp"
#include "Logging/Log.hpp"
#include "Exception.hpp"

namespace Recluse {


b8 Archive::Open(const std::string Filename)
{
  m_File.open(Filename);
  if (!m_File.is_open()) {
    R_ASSERT(false, "Failed to open file for archiving!\n");
    return false;
  }
  return IArchive::Open(Filename);
}


b8 Archive::Close()
{
  if (!Opened()) {
    R_ASSERT(false, "Archive file was not open to begin with! Can not close anything!\n");
    return false;
  }
  
  m_File.close();
  return IArchive::Close();
}


// TOD(): Serialize and Deserialize stuff...

IArchive& Archive::operator<<(u8 Val) 
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(i8 Val)
{
  return operator<<(static_cast<u8>(Val));
}


IArchive& Archive::operator<<(u16 Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(i16 Val)
{
  return operator<<(static_cast<u16>(Val));
}


IArchive& Archive::operator<<(u32 Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(i32 Val)
{
  return operator<<(static_cast<u32>(Val));
}


IArchive& Archive::operator<<(u64 Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(i64 Val)
{
  return operator<<(static_cast<u64>(Val));
}


IArchive& Archive::operator<<(std::string Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator>>(u8& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(i8& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(u16& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(i16& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(u32& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(i32& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(u64& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(i64& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(std::string& Val)
{
  m_File >> Val;
  return (*this);
}
} // Recluse