#pragma once

#include <array>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../../common/interface.hpp"

// Unordered-map price index variant of the array implementation.
//
// Structural change vs array:
//   - std::map<PriceType, vector<IdType>> replaced with
//     std::unordered_map<PriceType, vector<IdType>> — O(1) avg price lookup
//     vs O(log n) RB-tree traversal.
//   - best_bid / best_ask hints added (copied from flat) because unordered_map
//     has no ordered iteration for the sweep; the sweep scans the flat
//     volumeMap array and only touches the hash map when volume > 0.
//   - reserve(4096) in constructor prevents rehash during the seeding phase.
//
// Memory vs flat:
//   - flat pre-allocates 65536 × sizeof(vector) × 2 ≈ 3 MB regardless of
//     active levels. hash_map allocates only ~2000 bucket entries for the
//     active price range — much smaller resident footprint at runtime.
//
// ID map and volume cache are identical to array.

static constexpr std::size_t HM_PRICE_COUNT =
    static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()) + 1; // 65536

template <typename T>
using LevelVector = std::vector<T>;

using IDOrderMap = std::array<std::optional<Order>, std::numeric_limits<uint16_t>::max()>;
using VolumeMap  = std::array<QuantityType, HM_PRICE_COUNT>;

struct Orderbook {
    std::unordered_map<PriceType, LevelVector<IdType>> buyOrders;
    std::unordered_map<PriceType, LevelVector<IdType>> sellOrders;
    IDOrderMap idToOrdersMap;
    VolumeMap  buyVolume;
    VolumeMap  sellVolume;

    PriceType best_bid{0};
    PriceType best_ask{std::numeric_limits<PriceType>::max()};

    Orderbook()
    {
        buyVolume.fill(0);
        sellVolume.fill(0);
        buyOrders.reserve(4096);
        sellOrders.reserve(4096);
    }

    uint32_t match_order(const Order& incoming);
    void modify_order_by_id(IdType order_id, QuantityType new_quantity);
    uint32_t get_volume_at_level(Side side, PriceType price);
    Order lookup_order_by_id(IdType order_id);
    bool order_exists(IdType order_id);
};

static_assert(OrderbookConcept<Orderbook>);
