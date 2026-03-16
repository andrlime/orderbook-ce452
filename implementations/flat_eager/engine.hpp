#pragma once

#include <array>
#include <limits>
#include <optional>
#include <vector>

#include "../../common/interface.hpp"

// Eager-deletion variant of the flat implementation.
//
// Structural change vs flat:
//   - modify_order_by_id(id, 0) immediately erases the ID from the level
//     vector via std::find + erase (O(level_depth ≈ 16) cost).
//   - Because every ID in a level vector is guaranteed live, the match loop
//     has no stale-ID branch — every slot dereference is valid.
//
// Data structures are identical to flat; only the deletion strategy differs.

static constexpr std::size_t FE_PRICE_COUNT =
    static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()) + 1; // 65536

template <typename T>
using LevelVector = std::vector<T>;

using IDOrderMap = std::array<std::optional<Order>, std::numeric_limits<uint16_t>::max()>;
using VolumeMap  = std::array<QuantityType, FE_PRICE_COUNT>;

struct Orderbook {
    std::array<LevelVector<IdType>, FE_PRICE_COUNT> buyOrders;
    std::array<LevelVector<IdType>, FE_PRICE_COUNT> sellOrders;
    IDOrderMap idToOrdersMap;
    VolumeMap  buyVolume;
    VolumeMap  sellVolume;

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
