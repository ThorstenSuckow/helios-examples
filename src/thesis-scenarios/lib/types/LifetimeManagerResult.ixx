module;

#include <cstddef>

export module thesis.types.LifetimeManagerResult;

export namespace thesis::types {


    struct LifetimeManagerResult {
        std::size_t expiredCount;
    };


}
