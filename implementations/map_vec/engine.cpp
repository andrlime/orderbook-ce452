#include "engine.hpp"

#include <functional>
#include <optional>
#include <stdexcept>

template <typename OrderMap, typename Condition>
static uint32_t
process_orders(Order& order, OrderMap& ordersMap, Condition cond)
{
    uint32_t matchCount = 0;
    auto it = ordersMap.begin();
    while (it != ordersMap.end() && order.quantity > 0 && (it->first == order.price || cond(it->first, order.price))) {
        auto& ordersAtPrice = it->second;
        for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && order.quantity > 0;) {
            QuantityType trade = std::min(order.quantity, orderIt->quantity);
            order.quantity -= trade;
            orderIt->quantity -= trade;
            ++matchCount;
            if (orderIt->quantity == 0)
                orderIt = ordersAtPrice.erase(orderIt);
            else
                ++orderIt;
        }
        if (ordersAtPrice.empty())
            it = ordersMap.erase(it);
        else
            ++it;
    }
    return matchCount;
}

uint32_t
Orderbook::match_order(const Order& incoming)
{
    uint32_t matchCount = 0;
    Order order = incoming;

    if (order.side == Side::BUY) {
        matchCount = process_orders(order, sellOrders, std::less<>());
        if (order.quantity > 0)
            buyOrders[order.price].push_back(order);
    }
    else {
        matchCount = process_orders(order, buyOrders, std::greater<>());
        if (order.quantity > 0)
            sellOrders[order.price].push_back(order);
    }
    return matchCount;
}

template <typename OrderMap>
static bool
modify_order_in_map(OrderMap& ordersMap, IdType order_id, QuantityType new_quantity)
{
    for (auto it = ordersMap.begin(); it != ordersMap.end();) {
        auto& orderVec = it->second;
        for (auto orderIt = orderVec.begin(); orderIt != orderVec.end();) {
            if (orderIt->id == order_id) {
                if (new_quantity == 0)
                    orderIt = orderVec.erase(orderIt);
                else {
                    orderIt->quantity = new_quantity;
                    return true;
                }
            }
            else {
                ++orderIt;
            }
        }
        if (orderVec.empty())
            it = ordersMap.erase(it);
        else
            ++it;
    }
    return false;
}

void
Orderbook::modify_order_by_id(IdType order_id, QuantityType new_quantity)
{
    if (modify_order_in_map(buyOrders, order_id, new_quantity))
        return;
    if (modify_order_in_map(sellOrders, order_id, new_quantity))
        return;
}

template <typename OrderMap>
static std::optional<Order>
lookup_order_in_map(OrderMap& ordersMap, IdType order_id)
{
    for (const auto& [price, orderVec] : ordersMap) {
        for (const auto& order : orderVec) {
            if (order.id == order_id)
                return order;
        }
    }
    return std::nullopt;
}

uint32_t
Orderbook::get_volume_at_level(Side side, PriceType price)
{
    uint32_t total = 0;
    if (side == Side::BUY) {
        auto it = buyOrders.find(price);
        if (it == buyOrders.end())
            return 0;
        for (const auto& order : it->second)
            total += order.quantity;
    }
    else {
        auto it = sellOrders.find(price);
        if (it == sellOrders.end())
            return 0;
        for (const auto& order : it->second)
            total += order.quantity;
    }
    return total;
}

Order
Orderbook::lookup_order_by_id(IdType order_id)
{
    auto o1 = lookup_order_in_map(buyOrders, order_id);
    auto o2 = lookup_order_in_map(sellOrders, order_id);
    if (o1.has_value())
        return *o1;
    if (o2.has_value())
        return *o2;
    throw std::runtime_error("Order not found");
}

bool
Orderbook::order_exists(IdType order_id)
{
    return lookup_order_in_map(buyOrders, order_id).has_value()
           || lookup_order_in_map(sellOrders, order_id).has_value();
}
