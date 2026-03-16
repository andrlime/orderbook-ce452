#pragma once

#include <map>
#include <vector>

#include "../../common/interface.hpp"

// Intermediate implementation: std::map for price levels, std::vector<Order>
// per level (instead of std::list<Order> in the reference).
//
// Compared to reference:
//   - Orders at each price level are packed contiguously in a vector, improving
//     spatial locality during iteration and volume summation.
//   - No O(1) ID lookup or volume cache — both are still O(n).
//
// Compared to array:
//   - Price traversal still goes through a red-black tree (std::map).
//   - No IDOrderMap or VolumeMap arrays.

struct Orderbook {
    std::map<PriceType, std::vector<Order>, std::greater<PriceType>> buyOrders;
    std::map<PriceType, std::vector<Order>> sellOrders;

    uint32_t match_order(const Order& incoming);
    void modify_order_by_id(IdType order_id, QuantityType new_quantity);
    uint32_t get_volume_at_level(Side side, PriceType price);
    Order lookup_order_by_id(IdType order_id);
    bool order_exists(IdType order_id);
};

static_assert(OrderbookConcept<Orderbook>);
