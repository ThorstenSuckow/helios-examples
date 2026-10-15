module;

#include <cstddef>

export module thesis.types.DeltaResResult;

export namespace thesis::types {


    template<typename THandle>
    struct DeltaResResult {
        std::size_t processedCollisions;
    };


}
