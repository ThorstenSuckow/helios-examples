module;

#include <cstddef>

export module thesis.types.UniDeltaResResult;

export namespace thesis::types {


    template<typename THandle>
    struct UniDeltaResResult {
        std::size_t processedCollisionsGo;
        std::size_t processedCollisionsPrt;
    };


}
