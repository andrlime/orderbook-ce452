#include "engine.hpp"

#include <algorithm>
#include <stdexcept>

// Match a single price level against `order`.
// flat_eager guarantees every ID in a level vector is live (no stale IDs),
// so there is no liveness check here — every slot dereference is valid.
static void
match_level(Order& order, LevelVector<IdType>& vec, IDOrderMap& idMap,
            VolumeMap& vol, PriceType price, uint32_t& matchCount)
{
    for (auto it = vec.begin(); it != vec.end() && order.quantity > 0;) {
        Order& resting = *idMap[*it];
        QuantityType trade = std::min(order.quantity, resting.quantity);
        order.quantity -= trade;
        resting.quantity -= trade;
        vol[price] -= trade;
        ++matchCount;
        if (resting.quantity == 0) {
            idMap[*it] = std::nullopt;
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
        PriceType p = best_ask;
        while (order.quantity > 0 && p <= order.price) {
            if (sellVolume[p] == 0) { ++p; continue; }
            match_level(order, sellOrders[p], idToOrdersMap, sellVolume, p, matchCount);
            if (sellVolume[p] == 0) ++p;
        }
        best_ask = p;

        if (order.quantity > 0) {
            buyOrders[order.price].push_back(order.id);
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
            match_level(order, buyOrders[p], idToOrdersMap, buyVolume,
                        static_cast<PriceType>(p), matchCount);
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
        return; // already cancelled or fully matched (Test 34: noop)
    auto& order = *slot;
    auto& vol = order.side == Side::BUY ? buyVolume : sellVolume;
    vol[order.price] += new_quantity - order.quantity;
    order.quantity = new_quantity;
    if (new_quantity == 0) {
        // Eager erase: remove stale ID from the level vector immediately so
        // the match loop never encounters it.  Only full cancels trigger this;
        // partial qty reductions (new_quantity > 0) must NOT erase — FIFO
        // order must be preserved (Test 16).
        auto& vec = (order.side == Side::BUY ? buyOrders : sellOrders)[order.price];
        auto it = std::find(vec.begin(), vec.end(), order_id);
        if (it != vec.end())
            vec.erase(it);
        slot = std::nullopt;
    }
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
