#pragma once

#include <array>
#include <limits>
#include <optional>
#include <vector>

#include "../../common/interface.hpp"

// Maximally cache-friendly implementation: std::map is eliminated entirely.
// Price levels are stored in a flat std::array indexed directly by price
// (valid because PriceType is uint16_t, range [0, 65535]).
//
// Compared to array impl:
//   - Price traversal during matching scans a flat array instead of a
//     red-black tree — sequential memory access vs pointer chasing.
//   - best_bid/best_ask hints keep the scan O(active levels) amortized.
//
// Compared to reference/map_vec:
//   - No heap-allocated tree nodes for price→orders mapping.
//   - O(1) ID lookup and volume queries via large static arrays.

static constexpr std::size_t FLAT_PRICE_COUNT =
    static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()) + 1; // 65536

template <typename T>
using LevelVector = std::vector<T>;

using IDOrderMap = std::array<std::optional<Order>, std::numeric_limits<uint16_t>::max()>;
using VolumeMap  = std::array<QuantityType, FLAT_PRICE_COUNT>;

struct Orderbook {
    std::array<LevelVector<IdType>, FLAT_PRICE_COUNT> buyOrders;
    std::array<LevelVector<IdType>, FLAT_PRICE_COUNT> sellOrders;
    IDOrderMap idToOrdersMap;
    VolumeMap  buyVolume;
    VolumeMap  sellVolume;

    // Cached hints for the best resting prices.  May point to empty levels
    // after cancellations; the matching loop skips empty levels via volumeMap.
    PriceType best_bid{0};
    PriceType best_ask{std::numeric_limits<PriceType>::max()};

    Orderbook()
    {
        buyVolume.fill(0);
        sellVolume.fill(0);
    }

    uint32_t match_order(const Order& incoming);
    void modify_order_by_id(IdType order_id, QuantityType new_quantity);
    uint32_t get_volume_at_level(Side side, PriceType price);
    Order lookup_order_by_id(IdType order_id);
    bool order_exists(IdType order_id);
};

static_assert(OrderbookConcept<Orderbook>);
