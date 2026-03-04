#pragma once

#include <list>
#include <map>

#include "../../common/interface.hpp"

// You CAN and SHOULD change this
struct Orderbook {
    std::map<PriceType, std::list<Order>, std::greater<PriceType>> buyOrders;
    std::map<PriceType, std::list<Order>> sellOrders;

    uint32_t match_order(const Order& incoming);
    void modify_order_by_id(IdType order_id, QuantityType new_quantity);
    uint32_t get_volume_at_level(Side side, PriceType price);
    Order lookup_order_by_id(IdType order_id);
    bool order_exists(IdType order_id);
};

static_assert(OrderbookConcept<Orderbook>);
