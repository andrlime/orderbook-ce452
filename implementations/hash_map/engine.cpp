#include "engine.hpp"

#include <stdexcept>

// Match a single price level's resting orders against `order`.
// Stale IDs (left by modify_order_by_id) are cleaned up lazily here,
// same as in the flat and array implementations.
static void
match_level(Order& order, LevelVector<IdType>& vec, IDOrderMap& idMap,
            VolumeMap& vol, PriceType price, uint32_t& matchCount)
{
    for (auto it = vec.begin(); it != vec.end() && order.quantity > 0;) {
        auto& slot = idMap[*it];
        if (!slot.has_value()) {
            it = vec.erase(it); // clean up stale ID left by modify_order_by_id
            continue;
        }
        Order& resting = *slot;
        QuantityType trade = std::min(order.quantity, resting.quantity);
        order.quantity -= trade;
        resting.quantity -= trade;
        vol[price] -= trade;
        ++matchCount;
        if (resting.quantity == 0) {
            slot = std::nullopt;
            it = vec.erase(it);
        } else {
            ++it;
        }
    }
}

uint32_t
Orderbook::match_order(const Order& incoming)
{
    Order order = incoming;
    uint32_t matchCount = 0;

    if (order.side == Side::BUY) {
        // Scan sell levels from best_ask upward to order.price.
        // volumeMap is a flat array — O(1) liveness check without touching
        // the hash map.  The hash map is only consulted when volume > 0.
        PriceType p = best_ask;
        while (order.quantity > 0 && p <= order.price) {
            if (sellVolume[p] == 0) { ++p; continue; }
            auto it = sellOrders.find(p);
            if (it != sellOrders.end()) {
                match_level(order, it->second, idToOrdersMap, sellVolume, p, matchCount);
                if (it->second.empty())
                    sellOrders.erase(it);
            }
            if (sellVolume[p] == 0) ++p;
        }
        best_ask = p;

        if (order.quantity > 0) {
            buyOrders[order.price].push_back(order.id); // operator[] creates entry if new
            idToOrdersMap[order.id] = order;
            buyVolume[order.price] += order.quantity;
            if (order.price > best_bid)
                best_bid = order.price;
        }
    } else {
        int p = static_cast<int>(best_bid);
        const int limit = static_cast<int>(order.price);
        while (order.quantity > 0 && p >= limit) {
            if (buyVolume[p] == 0) { --p; continue; }
            auto it = buyOrders.find(p);
            if (it != buyOrders.end()) {
                match_level(order, it->second, idToOrdersMap, buyVolume,
                            static_cast<PriceType>(p), matchCount);
                if (it->second.empty())
                    buyOrders.erase(it);
            }
            if (buyVolume[p] == 0) --p;
        }
        best_bid = static_cast<PriceType>(p < 0 ? 0 : p);

        if (order.quantity > 0) {
            sellOrders[order.price].push_back(order.id);
            idToOrdersMap[order.id] = order;
            sellVolume[order.price] += order.quantity;
            if (order.price < best_ask)
                best_ask = order.price;
        }
    }

    return matchCount;
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
    // ID stays in the level vector; stale entries are cleaned up lazily
    // during the next traversal of that level in match_level().
    if (new_quantity == 0)
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
