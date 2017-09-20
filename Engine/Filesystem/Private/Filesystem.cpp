// Copyright (c) 2017 Recluse Project.
#include "Filesystem.hpp"


namespace Recluse {


Filesystem& gFilesystem() 
{
  return Filesystem::Instance();
}


void Filesystem::OnStartUp()
{
}


void Filesystem::OnShutDown()
{
}
} // Recluse