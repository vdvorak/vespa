// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/searchcommon/attribute/iattributevector.h>

namespace vespalib { class MemoryUsage; }

namespace search::attribute {

class IPostingListAttributeBase
{
public:
    virtual
    ~IPostingListAttributeBase()
    {
    }

    virtual void
    clearPostings(IAttributeVector::EnumHandle eidx,
                  uint32_t fromLid,
                  uint32_t toLid) = 0;

    virtual void forwardedShrinkLidSpace(uint32_t newSize) = 0;
    virtual vespalib::MemoryUsage getMemoryUsage() const = 0;
};

} // namespace search::attribute

