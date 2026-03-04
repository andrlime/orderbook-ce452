#pragma once

#include <array>
#include <limits>
#include <map>
#include <optional>
#include <vector>

#include "../../common/interface.hpp"

// Per-price-level list of resting order IDs.
// small_vector<IdType, 64> in the original (boost); std::vector here.
template <typename T>
using LevelVector = std::vector<T>;

template <typename C>
using SideOrders = std::map<PriceType, LevelVector<IdType>, C>;

// O(1) lookup by ID. IDs must be < 2^16 for this impl.
using IDOrderMap = std::array<std::optional<Order>, std::numeric_limits<uint16_t>::max()>;

// O(1) volume lookup by price.
using VolumeMap = std::array<QuantityType, std::numeric_limits<uint16_t>::max()>;

// You CAN and SHOULD change this
struct Orderbook {
    SideOrders<std::greater<PriceType>> buyOrders;
    SideOrders<std::less<PriceType>> sellOrders;
    IDOrderMap idToOrdersMap;
    VolumeMap buyVolume;
    VolumeMap sellVolume;

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
