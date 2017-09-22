// Copyright (c) 2017 Recluse Project. All rights reserved.
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