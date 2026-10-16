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
        using CollisionDetectionResult = helios::physics::collision::types::CollisionDetectionResult<THandle>;

        CollisionDetectionResult resultGo_;

        CollisionDetectionResult resultPrt_;

    public:

        UniCollisionDetectionResult(
            CollisionDetectionResult&& resultGo,
            CollisionDetectionResult&& resultPrt
        ) : resultGo_(std::move(resultGo)),
            resultPrt_(std::move(resultPrt)) {}


        [[nodiscard]] std::span<const CollisionPair> collisionPairsGo() const {
            return resultGo_.collisionPairs();
        }

        [[nodiscard]] std::span<const CollisionPair> collisionPairsPrt() const {
            return resultPrt_.collisionPairs();
        }
    };


}
