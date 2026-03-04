#include "engine.hpp"

#include <stdexcept>

template <typename OrderMap, typename OtherOrderMap, typename VolMap, typename Condition>
[[gnu::always_inline]] inline uint32_t
process_orders(
    Order order, OrderMap& ordersMap, OtherOrderMap& otherSideMap, VolMap& volumeMap, VolMap& otherSideVol,
    IDOrderMap& idToOrdersMap, Condition cond
)
{
    uint32_t matchCount = 0;

    auto it = ordersMap.begin();
    while (it != ordersMap.end()) {
        auto& [price, vec] = *it;
        if (!(order.quantity > 0 && (price == order.price || cond(price, order.price)))) [[unlikely]]
            break;

        auto idIt = vec.begin();
        while (idIt != vec.end() && order.quantity > 0) {
            auto& slot = idToOrdersMap[*idIt];
            if (slot.has_value()) [[likely]] {
                Order& resting = *slot;
                QuantityType trade = std::min(order.quantity, resting.quantity);
                resting.quantity -= trade;
                order.quantity -= trade;
                volumeMap[price] -= trade;
                ++matchCount;

                if (resting.quantity == 0) [[unlikely]] {
                    slot = std::nullopt;
                    idIt = vec.erase(idIt);
                    continue;
                }
            }
            ++idIt;
        }

        if (vec.empty()) [[unlikely]] {
            it = ordersMap.erase(it);
            continue;
        }
        ++it;
    }

    if (order.quantity > 0) [[likely]] {
        otherSideMap[order.price].push_back(order.id);
        idToOrdersMap[order.id] = order;
        otherSideVol[order.price] += order.quantity;
    }

    return matchCount;
}

uint32_t
Orderbook::match_order(const Order& incoming)
{
    if (incoming.side == Side::BUY) {
        return process_orders(incoming, sellOrders, buyOrders, sellVolume, buyVolume, idToOrdersMap, std::less<>());
    }
    else {
        return process_orders(incoming, buyOrders, sellOrders, buyVolume, sellVolume, idToOrdersMap, std::greater<>());
    }
}

void
Orderbook::modify_order_by_id(IdType order_id, QuantityType new_quantity)
{
    auto& slot = idToOrdersMap[order_id];
    if (!slot.has_value())
        return;
    auto& order = *slot;
    auto& vol = order.side == Side::BUY ? buyVolume : sellVolume;
    vol[order.price] += new_quantity - order.quantity;
    order.quantity = new_quantity;
    if (new_quantity == 0) [[unlikely]]
        slot = std::nullopt;
}

uint32_t
Orderbook::get_volume_at_level(Side side, PriceType price)
{
    return (side == Side::BUY ? buyVolume : sellVolume)[price];
}

Order
Orderbook::lookup_order_by_id(IdType order_id)
{
    auto& slot = idToOrdersMap[order_id];
    if (!slot.has_value())
        throw std::runtime_error("Order not found");
    return *slot;
}

bool
Orderbook::order_exists(IdType order_id)
{
    return idToOrdersMap[order_id].has_value();
}
