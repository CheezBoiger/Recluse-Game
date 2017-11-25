// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


// Smart pointer, with the minimal needed features.
// TODO(): Will need to add additional alloc component later.
// Objects must have a defined move constructor for this APtr to
// work properly.
template<typename T>
class APtr {
public:
  APtr()
    : mObject(nullptr) { }

  APtr(T&& obj) 
    : mObject(new T(obj)) { }

  APtr(APtr&& ptr)
    : mObject(ptr.mObject)
  {
    ptr.mObject = nullptr;
  }

  ~APtr() 
  {
    CleanUp();
    R_ASSERT(!mObject, "Object did not destroy prior to APtr destruction! Memory Leak!\n");
  }

  APtr&   operator=(APtr&& ptr)
  {
    CleanUp();

    mObject = ptr.mObject;
    ptr.mObject = nullptr;
    return (*this);
  }
  
  APtr&   operator=(T&& obj) 
  {
    CleanUp();

    mObject = new T(obj);
    return (*this);
  }

  T*      operator->() 
  { 
    return Ptr(); 
  }

  T&      operator*() 
  {
    return *mObject;
  }

  T*      Ptr() 
  { 
    return mObject; 
  }

  void    CleanUp() 
  { 
    if (mObject) 
    {
      delete mObject;
      mObject = nullptr;
    }
  }

  void    Refresh(T&& obj) 
  {
    CleanUp();
    mObject = new T(obj);
  }


  void DeepCopy(const APtr& ptr) {
    CleanUp();
    mObject = new T(ptr.mObject);
  }

private:
  T*      mObject;
};
} // Recluse