#include "engine.hpp"

#include <stdexcept>

// Match a single price level against `order` using SoA arrays.
// valid_by_id is checked for liveness (not qty==0) because qty transiently
// reaches 0 mid-match before valid is cleared.
static void
match_level_soa(Order& order, LevelVector<IdType>& vec,
                std::array<QuantityType, SOA_ID_COUNT>& qty,
                std::array<uint8_t,      SOA_ID_COUNT>& valid,
                VolumeMap& vol, PriceType price, uint32_t& matchCount)
{
    for (auto it = vec.begin(); it != vec.end() && order.quantity > 0;) {
        IdType id = *it;
        if (!valid[id]) {
            it = vec.erase(it); // clean up stale ID left by modify_order_by_id
            continue;
        }
        QuantityType trade = std::min(order.quantity, qty[id]);
        order.quantity -= trade;
        qty[id] -= trade;
        vol[price] -= trade;
        ++matchCount;
        if (qty[id] == 0) {
            valid[id] = 0;
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
            match_level_soa(order, sellOrders[p], qty_by_id, valid_by_id,
                            sellVolume, p, matchCount);
            if (sellVolume[p] == 0) ++p;
        }
        best_ask = p;

        if (order.quantity > 0) {
            IdType id = order.id;
            buyOrders[order.price].push_back(id);
            qty_by_id[id]   = order.quantity;
            price_by_id[id] = order.price;
            side_by_id[id]  = static_cast<uint8_t>(order.side);
            valid_by_id[id] = 1;
            buyVolume[order.price] += order.quantity;
            if (order.price > best_bid)
                best_bid = order.price;
        }
    } else {
        int p = static_cast<int>(best_bid);
        const int limit = static_cast<int>(order.price);
        while (order.quantity > 0 && p >= limit) {
            if (buyVolume[p] == 0) { --p; continue; }
            match_level_soa(order, buyOrders[p], qty_by_id, valid_by_id,
                            buyVolume, static_cast<PriceType>(p), matchCount);
            if (buyVolume[p] == 0) --p;
        }
        best_bid = static_cast<PriceType>(p < 0 ? 0 : p);

        if (order.quantity > 0) {
            IdType id = order.id;
            sellOrders[order.price].push_back(id);
            qty_by_id[id]   = order.quantity;
            price_by_id[id] = order.price;
            side_by_id[id]  = static_cast<uint8_t>(order.side);
            valid_by_id[id] = 1;
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
    if (!valid_by_id[order_id])
        return;
    auto& vol = (static_cast<Side>(side_by_id[order_id]) == Side::BUY)
                    ? buyVolume : sellVolume;
    // Adjust volume: += (new - old), wraps correctly in unsigned arithmetic.
    vol[price_by_id[order_id]] += new_quantity - qty_by_id[order_id];
    qty_by_id[order_id] = new_quantity;
    // Only clear valid on full cancel; partial modify keeps FIFO position.
    if (new_quantity == 0)
        valid_by_id[order_id] = 0;
}

uint32_t
Orderbook::get_volume_at_level(Side side, PriceType price)
{
    return (side == Side::BUY ? buyVolume : sellVolume)[price];
}

Order
Orderbook::lookup_order_by_id(IdType order_id)
{
    if (!valid_by_id[order_id])
        throw std::runtime_error("Order not found");
    return Order{
        order_id,
        price_by_id[order_id],
        qty_by_id[order_id],
        static_cast<Side>(side_by_id[order_id])
    };
}

bool
Orderbook::order_exists(IdType order_id)
{
    return valid_by_id[order_id] != 0;
}
