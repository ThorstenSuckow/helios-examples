module;

#include <cstddef>

export module thesis.types.UniCollisionManagerResult;

export namespace thesis::types {


    struct UniCollisionManagerResult {
        std::size_t collisionCountGo;
        std::size_t collisionCountPrt;
    };


}
