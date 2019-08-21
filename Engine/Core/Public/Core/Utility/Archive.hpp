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

  virtual B8        Open(const std::string Filename) { 
    m_Name = Filename;
    m_Opened = true; 
    return true; 
  }

  virtual B8        close() { m_Opened = false;  return true; }

  virtual IArchive& operator<<(U8 Val) = 0;
  virtual IArchive& operator<<(I8 Val) = 0;
  virtual IArchive& operator<<(U16 Val) = 0;
  virtual IArchive& operator<<(I16 Val) = 0;
  virtual IArchive& operator<<(U32 Val) = 0;
  virtual IArchive& operator<<(I32 Val) = 0;
  virtual IArchive& operator<<(U64 Val) = 0;
  virtual IArchive& operator<<(I64 Val) = 0;
  virtual IArchive& operator<<(std::string Str) = 0;

  virtual IArchive& operator>>(U8& Val) = 0;
  virtual IArchive& operator>>(I8& Val) = 0;
  virtual IArchive& operator>>(U16& Val) = 0;
  virtual IArchive& operator>>(I16& Val) = 0;
  virtual IArchive& operator>>(U32& Val) = 0;
  virtual IArchive& operator>>(I32& Val) = 0;
  virtual IArchive& operator>>(U64& Val) = 0;
  virtual IArchive& operator>>(I64& Val) = 0;
  virtual IArchive& operator>>(std::string& Val) = 0;

  B8                Opened() const { return m_Opened; }
  std::string       getName() const { return m_Name; }

protected:
  std::string m_Name;
  B8          m_Opened;
};


// Archive file, used to store information, however, this info is only
// plain data, which shows absolutely nothing about how to reconstruct the 
// information back, so be sure to add in a header in order to figure out!
class Archive : public IArchive {
public:
  virtual B8          Open(const std::string Filename) override;
  virtual B8          close() override;

  IArchive&           operator<<(U8 Val) override;
  IArchive&           operator<<(I8 Val) override;
  IArchive&           operator<<(U16 Val) override;
  IArchive&           operator<<(I16 Val) override;
  IArchive&           operator<<(U32 Val) override;
  IArchive&           operator<<(I32 Val) override;
  IArchive&           operator<<(U64 Val) override;
  IArchive&           operator<<(I64 Val) override;
  IArchive&           operator<<(std::string Val) override;

  IArchive&           operator>>(U8& Val) override;
  IArchive&           operator>>(I8& Val) override;
  IArchive&           operator>>(U16& Val) override;
  IArchive&           operator>>(I16& Val) override;
  IArchive&           operator>>(U32& Val) override;
  IArchive&           operator>>(I32& Val) override;
  IArchive&           operator>>(U64& Val) override;
  IArchive&           operator>>(I64& Val) override;
  IArchive&           operator>>(std::string& Val) override;

private:
  std::fstream m_File;
};
} // Recluse