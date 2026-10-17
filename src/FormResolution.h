#pragma once
#include <cstdint>

namespace PL {
template<class BoundObject,class Handler>
BoundObject* resolveVanillaInventoryObject(Handler* handler,std::uint32_t id) {
    // The untyped lookup must precede the polymorphic cast. An exact-type
    // lookup of a base class rejects every derived inventory form.
    auto* form=handler->LookupForm(id,"Skyrim.esm");
    return form ? form->template As<BoundObject>() : nullptr;
}
}
