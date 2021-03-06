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

  template<typename I>
  APtr(I&& obj) 
    : mObject(new I(std::move(obj))) { }

  APtr(APtr&& ptr)
    : mObject(ptr.mObject)
  {
    ptr.mObject = nullptr;
  }

  ~APtr() 
  {
    cleanUp();
    R_ASSERT(!mObject, "Object did not destroy prior to APtr destruction! Memory Leak!\n");
  }

  APtr&   operator=(APtr&& ptr)
  {
    cleanUp();

    mObject = ptr.mObject;
    ptr.mObject = nullptr;
    return (*this);
  }
  
  template<typename I>
  APtr&   operator=(I&& obj) 
  {
    cleanUp();

    mObject = new I(std::move(obj));
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

  void    cleanUp() 
  { 
    if (mObject) 
    {
      delete mObject;
      mObject = nullptr;
    }
  }

  void    Refresh(T&& obj) 
  {
    cleanUp();
    mObject = new T(obj);
  }


  void DeepCopy(const APtr& ptr) {
    cleanUp();
    mObject = new T(ptr.mObject);
  }

private:
  T*      mObject;
};
} // Recluse