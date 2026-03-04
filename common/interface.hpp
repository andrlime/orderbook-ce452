#pragma once

#include <concepts>
#include <cstdint>

enum class Side : uint8_t { BUY, SELL };

using IdType = uint32_t;
using PriceType = uint16_t;
using QuantityType = uint16_t;

struct Order {
    IdType id;
    PriceType price;
    QuantityType quantity;
    Side side;
};

template <typename OB>
concept OrderbookConcept = std::default_initializable<OB>
                           && requires(OB& ob, const Order& o, IdType id, QuantityType qty, Side s, PriceType p) {
                                  { ob.match_order(o) } -> std::same_as<uint32_t>;
                                  { ob.modify_order_by_id(id, qty) } -> std::same_as<void>;
                                  { ob.get_volume_at_level(s, p) } -> std::same_as<uint32_t>;
                                  { ob.lookup_order_by_id(id) } -> std::same_as<Order>;
                                  { ob.order_exists(id) } -> std::same_as<bool>;
                              };
