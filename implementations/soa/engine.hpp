#pragma once

#include <array>
#include <limits>
#include <vector>

#include "../../common/interface.hpp"

// Structure-of-Arrays ID map variant of the flat implementation.
//
// Structural change vs flat:
//   - Replaces array<optional<Order>, 65535> (AoS, ~786 KB) with four
//     parallel arrays split by field:
//
//       qty_by_id   (128 KB) — hot in the match loop
//       price_by_id (128 KB) — touched only on insert / modify
//       side_by_id  ( 64 KB) — touched only on insert / modify
//       valid_by_id ( 64 KB) — hot in the match loop
//
//   - Match-path working set: valid_by_id + qty_by_id = 192 KB (fits L2)
//     vs. 786 KB for the full AoS optional<Order>.
//
// Price index and volume cache are identical to flat.

static constexpr std::size_t SOA_PRICE_COUNT =
    static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()) + 1; // 65536

static constexpr std::size_t SOA_ID_COUNT =
    static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()); // 65535

template <typename T>
using LevelVector = std::vector<T>;

using VolumeMap = std::array<QuantityType, SOA_PRICE_COUNT>;

struct Orderbook {
    std::array<LevelVector<IdType>, SOA_PRICE_COUNT> buyOrders;
    std::array<LevelVector<IdType>, SOA_PRICE_COUNT> sellOrders;

    // SoA ID map — split by field for narrower hot-path working set.
    // valid_by_id is the liveness bit; qty==0 is NOT sufficient because qty
    // transiently reaches 0 mid-match before valid is cleared.
    std::array<QuantityType, SOA_ID_COUNT> qty_by_id;   // 128 KB — hot in match loop
    std::array<PriceType,    SOA_ID_COUNT> price_by_id; // 128 KB — insert/modify only
    std::array<uint8_t,      SOA_ID_COUNT> side_by_id;  //  64 KB — insert/modify only
    std::array<uint8_t,      SOA_ID_COUNT> valid_by_id; //  64 KB — hot in match loop

    VolumeMap buyVolume;
    VolumeMap sellVolume;

    PriceType best_bid{0};
    PriceType best_ask{std::numeric_limits<PriceType>::max()};

    Orderbook()
    {
        buyVolume.fill(0);
        sellVolume.fill(0);
        valid_by_id.fill(0);
    }

    uint32_t match_order(const Order& incoming);
    void modify_order_by_id(IdType order_id, QuantityType new_quantity);
    uint32_t get_volume_at_level(Side side, PriceType price);
    Order lookup_order_by_id(IdType order_id);
    bool order_exists(IdType order_id);
};

static_assert(OrderbookConcept<Orderbook>);
