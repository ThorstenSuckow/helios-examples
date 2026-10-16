/**
 * @file System.ixx
 * @brief Spatial-partitioning based collision detection using a uniform 3D grid.
 */
module;

#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <helios-physics-config.h>
#include <unordered_set>
#include <vector>
#include <cstddef>
#include <execution>
#include <exception>


export module thesis.strategies:GridCollisionDetection;


import helios.physics.collision.components;
import helios.physics.collision.types;
import helios.physics.motion.components;

import helios.ecs.component;
import helios.ecs.entity;
import helios.ecs.common;

import helios.core.thread;

import helios.math;

import helios.engine.spatial.components;

export namespace thesis::strategies {

    template<typename THandle>
    class GridCollisionDetection {

        using CollisionComponent = helios::physics::collision::components::CollisionComponent;
        using CollisionPair = helios::physics::collision::types::CollisionPair<THandle>;

        struct CollisionCandidate {
            THandle entityHandle;
            helios::math::aabbf bounds;
        };


        struct GridCell {
            std::vector<std::size_t> collisionCandidateIndices;
            void clear() {
                collisionCandidateIndices.clear();
            }
        };


        std::vector<CollisionPair> collisionPairs_;

        std::vector<CollisionCandidate> collisionCandidates_;

        /**
         * @brief Size of each grid cell in world units.
         */
        float cellSize_;

        /**
         * @brief World-space bounds defining the spatial region covered by the grid.
         */
        helios::math::aabbf gridBounds_;

        /**
         * @brief Flat storage for all grid cells, indexed as x + y * cellsX + z * (cellsX * cellsY).
         */
        std::vector<GridCell> cells_;

        /**
         * @brief Number of cells along the X axis.
         */
        unsigned int cellsX_;

        /**
         * @brief Number of cells along the Y axis.
         */
        unsigned int cellsY_;

        /**
         * @brief Number of cells along the Z axis.
         */
        unsigned int cellsZ_;

        /**
         * @brief Helper to keep track of updated cells (with potential collisions candidates) in one pass.
         */
        std::vector<size_t> trackedCells_;

        /**
         * @brief Initializes the grid based on world bounds and cell size.
         *
         * Computes the number of cells needed in each dimension and allocates
         * the cell storage.
         */
        void initGrid() {

            helios::math::vec3f size = gridBounds_.size();

            cellsX_ = std::max(1u, static_cast<unsigned int>(std::ceil(size[0] / cellSize_)));
            cellsY_ = std::max(1u, static_cast<unsigned int>(std::ceil(size[1] / cellSize_)));
            cellsZ_ = std::max(1u, static_cast<unsigned int>(std::ceil(size[2] / cellSize_)));

            const size_t cellCount = static_cast<size_t>(cellsX_) * cellsY_ * cellsZ_;

            // this would make 100'000'000 * sizeof(GridCell) Bytes
            // if we have 24 Bytes per GridCell, we end up with 2,4 GB alone for this spatial grid.
            if (cellCount > 100'000'000) {
                assert(false && "Cell count too high.");
                std::terminate();
            }

            trackedCells_.reserve(cellCount);
            cells_.resize(cellCount);
        }

        void solveCells (const std::size_t from, const std::size_t to, std::vector<CollisionPair>& localCollisionPairs) {
            for (std::size_t i = from; i < to; i++) {
                if (cells_[trackedCells_[i]].collisionCandidateIndices.size() < 2) {
                    continue;
                }
                solveCell(cells_[trackedCells_[i]], localCollisionPairs);
            }

            std::sort(localCollisionPairs.begin(), localCollisionPairs.end());

            localCollisionPairs.erase(
                std::unique(localCollisionPairs.begin(), localCollisionPairs.end()),
                localCollisionPairs.end()
            );
        };


        static constexpr std::size_t THRESHOLD = 10000;
        static constexpr std::size_t CHUNK_COUNT = 8;
        static constexpr bool USE_THREADS = false;

        std::vector<std::vector<CollisionPair>> chunks_{CHUNK_COUNT};


    public:



        explicit GridCollisionDetection(
            const helios::math::aabbf& bounds,
            const float cellSize
        ) : gridBounds_(bounds),
            cellSize_(cellSize) {
            assert(cellSize_ > 0.0f && "cellSize must not be <= 0.0f");
            initGrid();

        }


        /**
         * @brief Prepares the grid for a new collision detection pass.
         *
         * Clears all collision candidates from every cell and resets the set of
         * already-solved collision pairs.
         */
        void reset() {
            for (auto& cell: cells_) {
                cell.clear();
            }

            for (auto& chunk: chunks_) {
                chunk.clear();
            }

            trackedCells_.clear();
            collisionPairs_.clear();
            collisionCandidates_.clear();
        }

        bool addCollisionCandidate(const THandle handle, const helios::math::aabbf& aabb) noexcept {
            auto w2g = worldBoundsToGridBounds(aabb);

            if (!w2g) {
                return false;
            }

            updateCollisionCandidate(
                handle,
                *w2g,
                aabb
            );

            return true;
        }



        helios::physics::collision::types::CollisionDetectionResult<THandle> solveCollisions(helios::core::thread::ThreadPool& threadPool) noexcept {


            if (USE_THREADS && trackedCells_.size() > THRESHOLD) {
                threadPool.runAndWait(CHUNK_COUNT, [&](std::size_t taskNumber) {
                    const auto start = taskNumber * trackedCells_.size() / CHUNK_COUNT;
                    const auto end = (taskNumber + 1) * trackedCells_.size() / CHUNK_COUNT;
                    solveCells(start, end, chunks_[taskNumber]);
                });

                collisionPairs_ = chunks_[0];
                for (std::size_t i = 1; i < chunks_.size(); ++i) {
                    std::vector<CollisionPair> merged;
                    merged.reserve(collisionPairs_.size() + chunks_[i].size());

                    std::merge(
                        collisionPairs_.begin(), collisionPairs_.end(),
                        chunks_[i].begin(), chunks_[i].end(),
                        std::back_inserter(merged)
                    );

                    collisionPairs_ = std::move(merged);
                }

                collisionPairs_.erase(
                    std::unique(collisionPairs_.begin(), collisionPairs_.end()),
                    collisionPairs_.end()
                );

            } else {
                solveCells(0, trackedCells_.size(), collisionPairs_);
            }

            return helios::physics::collision::types::CollisionDetectionResult<THandle>(
                std::move(collisionPairs_)
            );
        }


        /**
         * @brief Converts world-space AABB bounds to grid cell indices.
         *
         * Transforms a floating-point AABB in world space to integer cell coordinates,
         * clamped to the valid grid range. Used to determine which cells an entity occupies.
         *
         * @param aabbf The world-space AABB to convert.
         *
         * @return An integer AABB representing the range of grid cell indices the entity spans.
         */
        [[nodiscard]] std::optional<helios::math::aabbi> worldBoundsToGridBounds(const helios::math::aabbf& aabbf) const noexcept {

            if (!gridBounds_.intersects(aabbf)) {
                return std::nullopt;
            }

            helios::math::vec3f min = aabbf.min() - gridBounds_.min();
            helios::math::vec3f max = aabbf.max() - gridBounds_.min();

            int xMin = static_cast<int>(std::floor(min[0] / cellSize_));
            int yMin = static_cast<int>(std::floor(min[1] / cellSize_));
            int zMin = static_cast<int>(std::floor(min[2] / cellSize_));

            int xMax = static_cast<int>(std::floor(max[0] / cellSize_));
            int yMax = static_cast<int>(std::floor(max[1] / cellSize_));
            int zMax = static_cast<int>(std::floor(max[2] / cellSize_));

            return helios::math::aabbi{
                std::clamp(xMin, 0, static_cast<int>(cellsX_ - 1)),
                std::clamp(yMin, 0, static_cast<int>(cellsY_ - 1)),
                std::clamp(zMin, 0, static_cast<int>(cellsZ_ - 1)),
                std::clamp(xMax, 0, static_cast<int>(cellsX_ - 1)),
                std::clamp(yMax, 0, static_cast<int>(cellsY_ - 1)),
                std::clamp(zMax, 0, static_cast<int>(cellsZ_ - 1)),
            };
        }


        /**
         * @brief Inserts a collision candidate into all grid cells it overlaps.
         *
         * Iterates through all cells within the given bounds and adds the candidate
         * to each cell's collision candidate list for subsequent narrow-phase testing.
         *
         * @param entityHandle Handle of the collision candidate.
         * @param bounds Grid cell index bounds (integer AABB) the entity spans.
         * @param worldBoundsComponent Pointer to the entity's world-space bounds component.
         * @param collisionComponent Pointer to the entity's collision component.
         */
        inline void updateCollisionCandidate(
            THandle entityHandle,
            const helios::math::aabbi& gridBounds,
            const helios::math::aabbf& worldBounds
        ) {
            const auto xMin = gridBounds.min()[0];
            const auto xMax = gridBounds.max()[0];
            const auto yMin = gridBounds.min()[1];
            const auto yMax = gridBounds.max()[1];
            const auto zMin = gridBounds.min()[2];
            const auto zMax = gridBounds.max()[2];

            for (int x = xMin; x <= xMax; x++) {
                for (int y = yMin; y <= yMax; y++) {
                    for (int z = zMin; z <= zMax; z++) {
                        auto& [collisionCandidateIndices] = cell(x, y, z);

                        collisionCandidates_.push_back(
                            CollisionCandidate{
                                entityHandle,
                                worldBounds
                            }
                        );

                        collisionCandidateIndices.push_back(collisionCandidates_.size() - 1);



                        // only consider the first added to prevent dups
                        if (collisionCandidateIndices.size() == 1) {
                            const auto idx = cellIndex(x, y, z);
                            trackedCells_.push_back(idx);
                        }
                    }
                }
            }
        }

        /**
         * @brief Performs pairwise AABB overlap detection for all candidates in a cell.
         */
        inline void solveCell(GridCell& cell, std::vector<CollisionPair>& localCollisionPairs) {

            auto& candidateIndices = cell.collisionCandidateIndices;

            for (size_t i = 0; i < candidateIndices.size(); i++) {

                CollisionCandidate& candidate = collisionCandidates_[candidateIndices[i]];
                const helios::math::aabbf& aabbCandidate = candidate.bounds;

                for (size_t j = i+1; j < candidateIndices.size(); j++) {

                    auto& match = collisionCandidates_[candidateIndices[j]];
                    const helios::math::aabbf& aabbMatch = match.bounds;

                    if (!aabbCandidate.intersects(aabbMatch)) {
                        continue;
                    }

                    auto lHandle = candidate.entityHandle;
                    auto rHandle = match.entityHandle;

                    const auto* a = &candidate.bounds;
                    const auto* b = &match.bounds;

                    if (lHandle > rHandle) {
                        std::swap(lHandle, rHandle);
                        std::swap(a, b);
                    }

                    localCollisionPairs.push_back(CollisionPair{
                        lHandle,
                        rHandle,
                        helios::math::overlapCenter(*a, *b),
                        helios::math::overlapNormal(*a, *b)
                    });
                }
            }
        }

        /**
         * @brief Accesses a grid cell by its 3D coordinates.
         *
         * @param x X-coordinate of the cell (0 to cellsX - 1).
         * @param y Y-coordinate of the cell (0 to cellsY - 1).
         * @param z Z-coordinate of the cell (0 to cellsZ - 1).
         *
         * @return Reference to the xGridCell at the specified coordinates.
         */
        [[nodiscard]] inline GridCell& cell(const unsigned int x, const  unsigned int y, const unsigned int z) noexcept {
            return cells_[cellIndex(x, y, z)];
        }

        /**
         * @brief Computes the 1D index of a cell in a 3D grid based on its x, y, and z coordinates.
         *
         * @param x The x-coordinate of the cell (must be less than the grid's x-dimension).
         * @param y The y-coordinate of the cell (must be less than the grid's y-dimension).
         * @param z The z-coordinate of the cell (must be less than the grid's z-dimension).
         *
         * @return The 1D index of the cell in the grid.
         */
        [[nodiscard]] inline constexpr size_t cellIndex(const unsigned int x, const unsigned int y, const unsigned int z) const noexcept {
            assert (x < cellsX_ && y < cellsY_ && z < cellsZ_ && "Invalid range values");

            return x + y * cellsX_ + (z * cellsX_ * cellsY_);
        }

    };


}
