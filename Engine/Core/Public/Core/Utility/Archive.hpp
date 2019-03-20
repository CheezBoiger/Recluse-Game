// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include <fstream>


namespace Recluse {


// Interface archive for serializing data into a storage unit.
// This is mainly used to store data into a file for use from the game
// object.
class IArchive {
public:
  virtual ~IArchive() { }

  virtual b8        Open(const std::string Filename) { 
    m_Name = Filename;
    m_Opened = true; 
    return true; 
  }

  virtual b8        close() { m_Opened = false;  return true; }

  virtual IArchive& operator<<(u8 Val) = 0;
  virtual IArchive& operator<<(i8 Val) = 0;
  virtual IArchive& operator<<(u16 Val) = 0;
  virtual IArchive& operator<<(i16 Val) = 0;
  virtual IArchive& operator<<(u32 Val) = 0;
  virtual IArchive& operator<<(i32 Val) = 0;
  virtual IArchive& operator<<(u64 Val) = 0;
  virtual IArchive& operator<<(i64 Val) = 0;
  virtual IArchive& operator<<(std::string Str) = 0;

  virtual IArchive& operator>>(u8& Val) = 0;
  virtual IArchive& operator>>(i8& Val) = 0;
  virtual IArchive& operator>>(u16& Val) = 0;
  virtual IArchive& operator>>(i16& Val) = 0;
  virtual IArchive& operator>>(u32& Val) = 0;
  virtual IArchive& operator>>(i32& Val) = 0;
  virtual IArchive& operator>>(u64& Val) = 0;
  virtual IArchive& operator>>(i64& Val) = 0;
  virtual IArchive& operator>>(std::string& Val) = 0;

  b8                Opened() const { return m_Opened; }
  std::string       getName() const { return m_Name; }

protected:
  std::string m_Name;
  b8          m_Opened;
};


// Archive file, used to store information, however, this info is only
// plain data, which shows absolutely nothing about how to reconstruct the 
// information back, so be sure to add in a header in order to figure out!
class Archive : public IArchive {
public:
  virtual b8          Open(const std::string Filename) override;
  virtual b8          close() override;

  IArchive&           operator<<(u8 Val) override;
  IArchive&           operator<<(i8 Val) override;
  IArchive&           operator<<(u16 Val) override;
  IArchive&           operator<<(i16 Val) override;
  IArchive&           operator<<(u32 Val) override;
  IArchive&           operator<<(i32 Val) override;
  IArchive&           operator<<(u64 Val) override;
  IArchive&           operator<<(i64 Val) override;
  IArchive&           operator<<(std::string Val) override;

  IArchive&           operator>>(u8& Val) override;
  IArchive&           operator>>(i8& Val) override;
  IArchive&           operator>>(u16& Val) override;
  IArchive&           operator>>(i16& Val) override;
  IArchive&           operator>>(u32& Val) override;
  IArchive&           operator>>(i32& Val) override;
  IArchive&           operator>>(u64& Val) override;
  IArchive&           operator>>(i64& Val) override;
  IArchive&           operator>>(std::string& Val) override;

private:
  std::fstream m_File;
};
} // Recluse