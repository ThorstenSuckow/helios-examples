module;

#include <cstddef>
#include <vector>
#include <span>
#include <utility>
export module thesis.types.UniCollisionDetectionResult;

import helios.physics.collision.types;

export namespace thesis::types {

    template<typename THandle>
    struct UniCollisionDetectionResult {

        using CollisionPair = helios::physics::collision::types::CollisionPair<THandle>;

        std::vector<CollisionPair> collisionPairsGo_;

        std::vector<CollisionPair> collisionPairsPrt_;

    public:

        explicit UniCollisionDetectionResult(std::vector<CollisionPair>&& pairsGo, std::vector<CollisionPair>&& pairsPrt)
        : collisionPairsGo_(std::move(pairsGo)), collisionPairsPrt_(std::move(pairsPrt)) {};

        UniCollisionDetectionResult(
            const helios::physics::collision::types::CollisionDetectionResult<THandle>& resultGo,
            const helios::physics::collision::types::CollisionDetectionResult<THandle>& resultPrt
        ) : collisionPairsGo_(resultGo.collisionPairs().begin(), resultGo.collisionPairs().end()),
            collisionPairsPrt_(resultPrt.collisionPairs().begin(), resultPrt.collisionPairs().end()) {}


        [[nodiscard]] std::span<const CollisionPair> collisionPairsGo() const {
            return collisionPairsGo_;
        }

        [[nodiscard]] std::span<const CollisionPair> collisionPairsPrt() const {
            return collisionPairsPrt_;
        }
    };


}
