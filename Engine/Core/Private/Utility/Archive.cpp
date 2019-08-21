// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Utility/Archive.hpp"
#include "Logging/Log.hpp"
#include "Exception.hpp"

namespace Recluse {


B8 Archive::Open(const std::string Filename)
{
  m_File.open(Filename);
  if (!m_File.is_open()) {
    R_ASSERT(false, "Failed to open file for archiving!\n");
    return false;
  }
  return IArchive::Open(Filename);
}


B8 Archive::close()
{
  if (!Opened()) {
    R_ASSERT(false, "Archive file was not open to begin with! Can not close anything!\n");
    return false;
  }
  
  m_File.close();
  return IArchive::close();
}


// TOD(): Serialize and Deserialize stuff...

IArchive& Archive::operator<<(U8 Val) 
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(I8 Val)
{
  return operator<<(static_cast<U8>(Val));
}


IArchive& Archive::operator<<(U16 Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(I16 Val)
{
  return operator<<(static_cast<U16>(Val));
}


IArchive& Archive::operator<<(U32 Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(I32 Val)
{
  return operator<<(static_cast<U32>(Val));
}


IArchive& Archive::operator<<(U64 Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator<<(I64 Val)
{
  return operator<<(static_cast<U64>(Val));
}


IArchive& Archive::operator<<(std::string Val)
{
  m_File << Val;
  return (*this);
}


IArchive& Archive::operator>>(U8& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(I8& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(U16& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(I16& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(U32& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(I32& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(U64& Val)
{
  m_File >> Val;
  return (*this);
}


IArchive& Archive::operator>>(I64& Val)
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