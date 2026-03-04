#include <cassert>
#include <iostream>

#include "engine.hpp"

// Templated tests — each test function is parameterized on an OrderbookConcept
// type so the same suite can validate any implementation.

// Test 0: Simple order lookup
template <OrderbookConcept OB>
void
test_lookup_order()
{
    std::cout << "Test 0: Simple order lookup" << std::endl;
    OB ob;
    Order sellOrder{1, 100, 10, Side::SELL};
    assert(!ob.order_exists(1));

    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 0);

    assert(ob.order_exists(1));
    Order order_lookup = ob.lookup_order_by_id(1);
    assert(order_lookup.id == 1);
    assert(order_lookup.price == 100);
    assert(order_lookup.quantity == 10);
    assert(order_lookup.side == Side::SELL);
    std::cout << "Test 0 passed." << std::endl;
}

// Test 1: Simple match and modify
template <OrderbookConcept OB>
void
test_simple_match_and_modify()
{
    std::cout << "Test 1: Simple match and modify" << std::endl;
    OB ob;
    Order sellOrder{1, 100, 10, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 0);

    Order buyOrder{2, 100, 5, Side::BUY};
    matches = ob.match_order(buyOrder);
    assert(matches == 1);

    assert(ob.order_exists(1));
    Order order_lookup = ob.lookup_order_by_id(1);
    assert(order_lookup.quantity == 5);

    ob.modify_order_by_id(1, 0);
    assert(!ob.order_exists(1));
    std::cout << "Test 1 passed." << std::endl;
}

// Test 2: Multiple matches across price levels
template <OrderbookConcept OB>
void
test_multiple_matches()
{
    std::cout << "Test 2: Multiple matches across price levels" << std::endl;
    OB ob;
    Order sellOrder1{3, 90, 5, Side::SELL};
    Order sellOrder2{4, 95, 5, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder1);
    assert(matches == 0);
    matches = ob.match_order(sellOrder2);
    assert(matches == 0);

    Order buyOrder{5, 100, 8, Side::BUY};
    matches = ob.match_order(buyOrder);
    assert(matches == 2);

    assert(ob.order_exists(4));
    Order order_lookup = ob.lookup_order_by_id(4);
    assert(order_lookup.quantity == 2);

    ob.modify_order_by_id(4, 1);
    assert(ob.order_exists(4));
    order_lookup = ob.lookup_order_by_id(4);
    assert(order_lookup.quantity == 1);

    ob.modify_order_by_id(4, 0);
    assert(!ob.order_exists(4));
    std::cout << "Test 2 passed." << std::endl;
}

// Test 3: Sell order matching buy orders
template <OrderbookConcept OB>
void
test_sell_order_matching_buy()
{
    std::cout << "Test 3: Sell order matching buy orders" << std::endl;
    OB ob;
    Order buyOrder{6, 100, 10, Side::BUY};
    uint32_t matches = ob.match_order(buyOrder);
    assert(matches == 0);

    Order sellOrder{7, 100, 4, Side::SELL};
    matches = ob.match_order(sellOrder);
    assert(matches == 1);

    assert(ob.order_exists(6));
    Order order_lookup = ob.lookup_order_by_id(6);
    assert(order_lookup.quantity == 6);

    Order sellOrder2{8, 90, 7, Side::SELL};
    matches = ob.match_order(sellOrder2);
    assert(matches == 1);

    assert(!ob.order_exists(6));
    assert(ob.order_exists(8));
    order_lookup = ob.lookup_order_by_id(8);
    assert(order_lookup.quantity == 1);
    std::cout << "Test 3 passed." << std::endl;
}

// Test 4: Full fill buy order exact match
template <OrderbookConcept OB>
void
test_full_fill_buy_order_exact_match()
{
    std::cout << "Test 4: Full fill buy order exact match" << std::endl;
    OB ob;
    Order sellOrder{20, 100, 10, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 0);

    Order buyOrder{21, 100, 10, Side::BUY};
    matches = ob.match_order(buyOrder);
    assert(matches == 1);

    assert(!ob.order_exists(20));
    std::cout << "Test 4 passed." << std::endl;
}

// Test 5: Partial fill buy order across multiple sell levels
template <OrderbookConcept OB>
void
test_partial_fill_buy_order_across_multiple_sell_levels()
{
    std::cout << "Test 5: Partial fill buy order across multiple sell levels" << std::endl;
    OB ob;
    Order sellOrder1{22, 95, 4, Side::SELL};
    Order sellOrder2{23, 100, 6, Side::SELL};
    ob.match_order(sellOrder1);
    ob.match_order(sellOrder2);

    Order buyOrder{24, 100, 8, Side::BUY};
    uint32_t matches = ob.match_order(buyOrder);
    assert(matches == 2);

    assert(ob.order_exists(23));
    Order order_lookup = ob.lookup_order_by_id(23);
    assert(order_lookup.quantity == 2);
    std::cout << "Test 5 passed." << std::endl;
}

// Test 6: Modify nonexistent order
template <OrderbookConcept OB>
void
test_modify_nonexistent_order()
{
    std::cout << "Test 6: Modify nonexistent order" << std::endl;
    OB ob;
    Order buyOrder{25, 100, 10, Side::BUY};
    ob.match_order(buyOrder);

    ob.modify_order_by_id(999, 0);

    assert(ob.order_exists(25));
    Order order_lookup = ob.lookup_order_by_id(25);
    assert(order_lookup.id == 25);
    std::cout << "Test 6 passed." << std::endl;
}

// Test 7: Partial modification of a resting order
template <OrderbookConcept OB>
void
test_partial_modification()
{
    std::cout << "Test 7: Partial modification of a resting order" << std::endl;
    OB ob;
    Order sellOrder{26, 100, 10, Side::SELL};
    ob.match_order(sellOrder);

    assert(ob.order_exists(26));
    Order order_lookup = ob.lookup_order_by_id(26);
    assert(order_lookup.quantity == 10);

    ob.modify_order_by_id(26, 1);
    assert(ob.order_exists(26));
    order_lookup = ob.lookup_order_by_id(26);
    assert(order_lookup.quantity == 1);

    ob.modify_order_by_id(26, 0);
    assert(!ob.order_exists(26));
    std::cout << "Test 7 passed." << std::endl;
}

// Test 8: Partial fill sell order across multiple buy levels
template <OrderbookConcept OB>
void
test_partial_fill_sell_order_across_multiple_buy_levels()
{
    std::cout << "Test 8: Partial fill sell order across multiple buy levels" << std::endl;
    OB ob;
    Order buyOrder1{27, 100, 5, Side::BUY};
    Order buyOrder2{28, 95, 5, Side::BUY};
    ob.match_order(buyOrder1);
    ob.match_order(buyOrder2);

    Order sellOrder{29, 90, 7, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 2);

    assert(ob.order_exists(28));
    Order order_lookup = ob.lookup_order_by_id(28);
    assert(order_lookup.quantity == 3);
    assert(!ob.order_exists(27));
    std::cout << "Test 8 passed." << std::endl;
}

// Test 9: No match due to exact price mismatch
template <OrderbookConcept OB>
void
test_exact_price_mismatch_no_fill()
{
    std::cout << "Test 9: No match due to exact price mismatch" << std::endl;
    OB ob;
    Order sellOrder{30, 105, 5, Side::SELL};
    ob.match_order(sellOrder);

    Order buyOrder{31, 100, 5, Side::BUY};
    ob.match_order(buyOrder);

    assert(ob.order_exists(30));
    assert(ob.order_exists(31));
    std::cout << "Test 9 passed." << std::endl;
}

// Test 10: Multiple partial fills on same price level
template <OrderbookConcept OB>
void
test_multiple_partial_fills_same_level()
{
    std::cout << "Test 10: Multiple partial fills on same price level" << std::endl;
    OB ob;
    Order sellOrder1{32, 100, 4, Side::SELL};
    Order sellOrder2{33, 100, 6, Side::SELL};
    ob.match_order(sellOrder1);
    ob.match_order(sellOrder2);

    Order buyOrder{34, 100, 8, Side::BUY};
    uint32_t matches = ob.match_order(buyOrder);
    assert(matches == 2);

    assert(!ob.order_exists(32));
    assert(ob.order_exists(33));
    Order order_lookup = ob.lookup_order_by_id(33);
    assert(order_lookup.quantity == 2);
    std::cout << "Test 10 passed." << std::endl;
}

// Test 11: Order book integrity after multiple operations
template <OrderbookConcept OB>
void
test_order_book_integrity_after_multiple_operations()
{
    std::cout << "Test 11: Order book integrity after multiple operations" << std::endl;
    OB ob;
    Order buyOrder{35, 100, 10, Side::BUY};
    ob.match_order(buyOrder);

    Order sellOrder1{36, 100, 5, Side::SELL};
    ob.match_order(sellOrder1);
    assert(ob.order_exists(35));
    Order order_lookup = ob.lookup_order_by_id(35);
    assert(order_lookup.quantity == 5);

    Order sellOrder2{37, 95, 3, Side::SELL};
    ob.match_order(sellOrder2);
    assert(ob.order_exists(35));
    order_lookup = ob.lookup_order_by_id(35);
    assert(order_lookup.quantity == 2);

    ob.modify_order_by_id(35, 0);
    assert(!ob.order_exists(35));

    Order sellOrder3{38, 100, 2, Side::SELL};
    ob.match_order(sellOrder3);
    assert(ob.order_exists(38));
    std::cout << "Test 11 passed." << std::endl;
}

// Test 12: Multiple orders on the same side maintain FIFO order
template <OrderbookConcept OB>
void
test_multiple_orders_same_side_ordering()
{
    std::cout << "Test 12: Multiple orders on the same side ordering" << std::endl;
    OB ob;
    Order buyOrder1{39, 100, 5, Side::BUY};
    Order buyOrder2{40, 100, 5, Side::BUY};
    ob.match_order(buyOrder1);
    ob.match_order(buyOrder2);

    Order sellOrder{41, 95, 3, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 1);

    assert(ob.order_exists(39));
    Order order_lookup = ob.lookup_order_by_id(39);
    assert(order_lookup.quantity == 2);
    std::cout << "Test 12 passed." << std::endl;
}

// Test 13: Full match sell order exact match
template <OrderbookConcept OB>
void
test_full_match_sell_order_exact_match()
{
    std::cout << "Test 13: Full match sell order exact match" << std::endl;
    OB ob;
    Order buyOrder{42, 100, 10, Side::BUY};
    ob.match_order(buyOrder);

    Order sellOrder{43, 100, 10, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 1);

    assert(!ob.order_exists(42));
    std::cout << "Test 13 passed." << std::endl;
}

// Test 14: Modify with no change
template <OrderbookConcept OB>
void
test_modify_no_change()
{
    std::cout << "Test 14: Modify with no change" << std::endl;
    OB ob;
    Order sellOrder{50, 100, 10, Side::SELL};
    ob.match_order(sellOrder);
    ob.modify_order_by_id(50, 10);
    assert(ob.order_exists(50));
    Order order_lookup = ob.lookup_order_by_id(50);
    assert(order_lookup.quantity == 10);
    std::cout << "Test 14 passed." << std::endl;
}

// Test 15: Modify order after partial fill
template <OrderbookConcept OB>
void
test_modify_after_partial_fill()
{
    std::cout << "Test 15: Modify order after partial fill" << std::endl;
    OB ob;
    Order buyOrder{51, 100, 10, Side::BUY};
    ob.match_order(buyOrder);
    Order sellOrder{52, 100, 4, Side::SELL};
    ob.match_order(sellOrder);
    assert(ob.order_exists(51));
    Order order_lookup = ob.lookup_order_by_id(51);
    assert(order_lookup.quantity == 6);

    ob.modify_order_by_id(51, 3);
    assert(ob.order_exists(51));
    order_lookup = ob.lookup_order_by_id(51);
    assert(order_lookup.quantity == 3);

    Order sellOrder2{53, 90, 3, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder2);
    assert(matches == 1);
    assert(!ob.order_exists(51));
    std::cout << "Test 15 passed." << std::endl;
}

// Test 16: Modify preserves FIFO order
template <OrderbookConcept OB>
void
test_modify_preserves_fifo()
{
    std::cout << "Test 16: Modify preserves FIFO order" << std::endl;
    OB ob;
    Order sellOrderA{54, 100, 5, Side::SELL};
    Order sellOrderB{55, 100, 5, Side::SELL};
    ob.match_order(sellOrderA);
    ob.match_order(sellOrderB);
    ob.modify_order_by_id(54, 3);

    Order buyOrder{56, 100, 4, Side::BUY};
    uint32_t matches = ob.match_order(buyOrder);
    assert(matches == 2);

    assert(!ob.order_exists(54));
    assert(ob.order_exists(55));
    Order order_lookup = ob.lookup_order_by_id(55);
    assert(order_lookup.quantity == 4);
    std::cout << "Test 16 passed." << std::endl;
}

// Test 17: Multiple modifications on the same order
template <OrderbookConcept OB>
void
test_multiple_modifications()
{
    std::cout << "Test 17: Multiple modifications on the same order" << std::endl;
    OB ob;
    Order buyOrder{57, 100, 12, Side::BUY};
    ob.match_order(buyOrder);

    ob.modify_order_by_id(57, 8);
    assert(ob.order_exists(57));
    Order order_lookup = ob.lookup_order_by_id(57);
    assert(order_lookup.quantity == 8);

    ob.modify_order_by_id(57, 5);
    assert(ob.order_exists(57));
    order_lookup = ob.lookup_order_by_id(57);
    assert(order_lookup.quantity == 5);

    Order sellOrder{58, 100, 5, Side::SELL};
    uint32_t matches = ob.match_order(sellOrder);
    assert(matches == 1);
    assert(!ob.order_exists(57));
    std::cout << "Test 17 passed." << std::endl;
}

// Test 18: Modify with quantity=0 removes the order entirely
template <OrderbookConcept OB>
void
test_modify_with_zero_removes_order()
{
    std::cout << "Test 18: Modify with quantity=0 removes the order entirely" << std::endl;
    OB ob;
    Order buyOrder{60, 100, 10, Side::BUY};
    ob.match_order(buyOrder);
    ob.modify_order_by_id(60, 0);
    assert(!ob.order_exists(60));
    std::cout << "Test 18 passed." << std::endl;
}

// Test 19: Get volume at level with no orders
template <OrderbookConcept OB>
void
test_get_volume_no_orders()
{
    std::cout << "Test 19: Get volume at level with no orders" << std::endl;
    OB ob;
    uint32_t volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    uint32_t volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_buy == 0);
    assert(volume_sell == 0);
    std::cout << "Test 19 passed." << std::endl;
}

// Test 20: Get volume at level with a single order
template <OrderbookConcept OB>
void
test_get_volume_single_order()
{
    std::cout << "Test 20: Get volume at level with a single order" << std::endl;
    OB ob;
    Order sellOrder{100, 100, 10, Side::SELL};
    ob.match_order(sellOrder);
    uint32_t volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 10);
    uint32_t volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    assert(volume_buy == 0);
    std::cout << "Test 20 passed." << std::endl;
}

// Test 21: Get volume at level with multiple orders at the same price
template <OrderbookConcept OB>
void
test_get_volume_multiple_orders_same_level()
{
    std::cout << "Test 21: Get volume at level with multiple orders at the same price" << std::endl;
    OB ob;
    Order sellOrder1{101, 100, 5, Side::SELL};
    Order sellOrder2{102, 100, 7, Side::SELL};
    ob.match_order(sellOrder1);
    ob.match_order(sellOrder2);
    uint32_t volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 12);
    std::cout << "Test 21 passed." << std::endl;
}

// Test 22: Get volume at level with orders at different price levels
template <OrderbookConcept OB>
void
test_get_volume_orders_different_levels()
{
    std::cout << "Test 22: Get volume at level with orders at different price levels" << std::endl;
    OB ob;
    Order buyOrder1{103, 100, 10, Side::BUY};
    Order buyOrder2{104, 101, 5, Side::BUY};
    ob.match_order(buyOrder1);
    ob.match_order(buyOrder2);
    uint32_t volume_buy_100 = ob.get_volume_at_level(Side::BUY, 100);
    uint32_t volume_buy_101 = ob.get_volume_at_level(Side::BUY, 101);
    assert(volume_buy_100 == 10);
    assert(volume_buy_101 == 5);
    std::cout << "Test 22 passed." << std::endl;
}

// Test 23: Get volume at level after partial fill
template <OrderbookConcept OB>
void
test_get_volume_after_partial_fill()
{
    std::cout << "Test 23: Get volume at level after partial fill" << std::endl;
    OB ob;
    Order sellOrder{105, 100, 10, Side::SELL};
    ob.match_order(sellOrder);
    Order buyOrder{106, 100, 4, Side::BUY};
    ob.match_order(buyOrder);
    uint32_t volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 6);
    std::cout << "Test 23 passed." << std::endl;
}

// Test 24: Get volume at level after order cancellation
template <OrderbookConcept OB>
void
test_get_volume_after_cancellation()
{
    std::cout << "Test 24: Get volume at level after order cancellation" << std::endl;
    OB ob;
    Order buyOrder{107, 100, 10, Side::BUY};
    ob.match_order(buyOrder);
    uint32_t volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    assert(volume_buy == 10);
    ob.modify_order_by_id(107, 0);
    volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    assert(volume_buy == 0);
    std::cout << "Test 24 passed." << std::endl;
}

// Test 25: Complex scenario with multiple SELL orders and modifications
template <OrderbookConcept OB>
void
test_get_volume_complex1()
{
    std::cout << "Test 25: Complex scenario with multiple SELL orders and modifications" << std::endl;
    OB ob;
    Order sellOrder1{200, 100, 10, Side::SELL};
    Order sellOrder2{201, 100, 20, Side::SELL};
    Order sellOrder3{202, 101, 15, Side::SELL};
    ob.match_order(sellOrder1);
    ob.match_order(sellOrder2);
    ob.match_order(sellOrder3);
    ob.modify_order_by_id(200, 5);
    uint32_t volume_100 = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_100 == 25);
    uint32_t volume_101 = ob.get_volume_at_level(Side::SELL, 101);
    assert(volume_101 == 15);
    std::cout << "Test 25 passed." << std::endl;
}

// Test 26: Complex scenario with interleaved BUY order insertions, matches,
// and modifications
template <OrderbookConcept OB>
void
test_get_volume_complex2()
{
    std::cout << "Test 26: Complex scenario with interleaved BUY order "
                 "insertions, matches, and modifications"
              << std::endl;
    OB ob;
    Order buyOrder1{300, 100, 20, Side::BUY};
    Order buyOrder2{301, 100, 10, Side::BUY};
    ob.match_order(buyOrder1);
    ob.match_order(buyOrder2);
    uint32_t volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    assert(volume_buy == 30);

    Order sellOrder{302, 100, 15, Side::SELL};
    ob.match_order(sellOrder);
    volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    assert(volume_buy == 15);

    ob.modify_order_by_id(301, 5);
    volume_buy = ob.get_volume_at_level(Side::BUY, 100);
    assert(volume_buy == 10);
    std::cout << "Test 26 passed." << std::endl;
}

// Test 27: Complex scenario with cancellations and new insertions on SELL side
template <OrderbookConcept OB>
void
test_get_volume_complex3()
{
    std::cout << "Test 27: Complex scenario with cancellations and new "
                 "insertions on SELL side"
              << std::endl;
    OB ob;
    Order sellOrder1{400, 100, 30, Side::SELL};
    Order sellOrder2{401, 100, 20, Side::SELL};
    ob.match_order(sellOrder1);
    ob.match_order(sellOrder2);
    uint32_t volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 50);

    ob.modify_order_by_id(400, 0);
    volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 20);

    Order sellOrder3{402, 100, 15, Side::SELL};
    ob.match_order(sellOrder3);
    volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 35);

    Order sellOrder4{403, 101, 10, Side::SELL};
    ob.match_order(sellOrder4);
    uint32_t volume_sell_101 = ob.get_volume_at_level(Side::SELL, 101);
    assert(volume_sell_101 == 10);

    ob.modify_order_by_id(401, 10);
    volume_sell = ob.get_volume_at_level(Side::SELL, 100);
    assert(volume_sell == 25);
    std::cout << "Test 27 passed." << std::endl;
}

// Test 28: Comprehensive scenario encompassing both BUY and SELL sides
template <OrderbookConcept OB>
void
test_get_volume_all_encompassing()
{
    std::cout << "Test 28: Comprehensive scenario encompassing both BUY and SELL sides" << std::endl;
    OB ob;
    Order buyOrder1{500, 100, 20, Side::BUY};
    Order buyOrder2{501, 100, 15, Side::BUY};
    Order buyOrder3{502, 99, 10, Side::BUY};
    ob.match_order(buyOrder1);
    ob.match_order(buyOrder2);
    ob.match_order(buyOrder3);

    Order sellOrder1{503, 100, 25, Side::SELL};
    ob.match_order(sellOrder1);

    Order sellOrder2{504, 102, 30, Side::SELL};
    Order sellOrder3{505, 101, 10, Side::SELL};
    ob.match_order(sellOrder2);
    ob.match_order(sellOrder3);

    ob.modify_order_by_id(502, 5);
    ob.modify_order_by_id(504, 20);

    Order buyOrder4{506, 102, 15, Side::BUY};
    ob.match_order(buyOrder4);

    uint32_t volume_buy_100 = ob.get_volume_at_level(Side::BUY, 100);
    uint32_t volume_buy_99 = ob.get_volume_at_level(Side::BUY, 99);
    uint32_t volume_sell_102 = ob.get_volume_at_level(Side::SELL, 102);
    uint32_t volume_sell_101 = ob.get_volume_at_level(Side::SELL, 101);

    assert(volume_buy_100 == 10);
    assert(volume_buy_99 == 5);
    assert(volume_sell_102 == 15);
    assert(volume_sell_101 == 0);
    std::cout << "Test 28 passed." << std::endl;
}

// Test 29: Fully matched aggressive order does not rest
template <OrderbookConcept OB>
void
test_fully_matched_aggressor_does_not_rest()
{
    std::cout << "Test 29: Fully matched aggressive order does not rest" << std::endl;
    OB ob;
    // Two resting sells totalling 10 units.
    ob.match_order(Order{1, 100, 6, Side::SELL});
    ob.match_order(Order{2, 100, 4, Side::SELL});

    // Aggressive buy consumes exactly 10 — should not rest.
    uint32_t m = ob.match_order(Order{3, 100, 10, Side::BUY});
    assert(m == 2);
    assert(!ob.order_exists(1));
    assert(!ob.order_exists(2));
    assert(!ob.order_exists(3));
    assert(ob.get_volume_at_level(Side::BUY, 100) == 0);
    assert(ob.get_volume_at_level(Side::SELL, 100) == 0);
    std::cout << "Test 29 passed." << std::endl;
}

// Test 30: get_volume_at_level is side-specific
template <OrderbookConcept OB>
void
test_volume_query_is_side_specific()
{
    std::cout << "Test 30: get_volume_at_level is side-specific" << std::endl;
    OB ob;
    ob.match_order(Order{1, 100, 15, Side::BUY});
    ob.match_order(Order{2, 101, 20, Side::SELL});

    // Each order must only show on its own side.
    assert(ob.get_volume_at_level(Side::BUY, 100) == 15);
    assert(ob.get_volume_at_level(Side::SELL, 100) == 0);
    assert(ob.get_volume_at_level(Side::SELL, 101) == 20);
    assert(ob.get_volume_at_level(Side::BUY, 101) == 0);
    std::cout << "Test 30 passed." << std::endl;
}

// Test 31: Aggressive order sweeps all resting levels and rests its remainder
template <OrderbookConcept OB>
void
test_aggressor_sweeps_all_levels_and_rests()
{
    std::cout << "Test 31: Aggressive order sweeps all levels and rests remainder" << std::endl;
    OB ob;
    ob.match_order(Order{1, 98, 5, Side::SELL});
    ob.match_order(Order{2, 99, 5, Side::SELL});
    ob.match_order(Order{3, 100, 5, Side::SELL});

    // Buy 20 @ 100 — consumes all 15 resting units, 5 left over should rest.
    uint32_t m = ob.match_order(Order{4, 100, 20, Side::BUY});
    assert(m == 3);
    assert(!ob.order_exists(1));
    assert(!ob.order_exists(2));
    assert(!ob.order_exists(3));
    assert(ob.order_exists(4));
    assert(ob.lookup_order_by_id(4).quantity == 5);
    assert(ob.get_volume_at_level(Side::BUY, 100) == 5);
    std::cout << "Test 31 passed." << std::endl;
}

// Test 32: Cancel then reuse the same order id
template <OrderbookConcept OB>
void
test_cancel_then_resubmit_same_id()
{
    std::cout << "Test 32: Cancel then resubmit the same order id" << std::endl;
    OB ob;
    ob.match_order(Order{1, 100, 10, Side::BUY});
    assert(ob.order_exists(1));

    ob.modify_order_by_id(1, 0);
    assert(!ob.order_exists(1));

    // Reuse id=1 — should be treated as a fresh order.
    ob.match_order(Order{1, 100, 7, Side::SELL});
    assert(ob.order_exists(1));
    assert(ob.lookup_order_by_id(1).quantity == 7);
    assert(ob.lookup_order_by_id(1).side == Side::SELL);
    std::cout << "Test 32 passed." << std::endl;
}

// Test 33: FIFO drain across many orders at the same price level
template <OrderbookConcept OB>
void
test_fifo_drain_many_orders_same_level()
{
    std::cout << "Test 33: FIFO drain across many orders at same level" << std::endl;
    OB ob;
    // Five sell orders at the same price, submitted in order 1..5.
    for (int i = 1; i <= 5; ++i)
        ob.match_order(Order{static_cast<IdType>(i), 100, 10, Side::SELL});

    // Buy them off one at a time; each buy should match the earliest remaining.
    for (int i = 1; i <= 5; ++i) {
        uint32_t m = ob.match_order(Order{static_cast<IdType>(100 + i), 100, 10, Side::BUY});
        assert(m == 1);
        // The order we just consumed must be gone.
        assert(!ob.order_exists(static_cast<IdType>(i)));
        // All later orders must still exist.
        for (int j = i + 1; j <= 5; ++j)
            assert(ob.order_exists(static_cast<IdType>(j)));
    }
    assert(ob.get_volume_at_level(Side::SELL, 100) == 0);
    std::cout << "Test 33 passed." << std::endl;
}

// Test 34: modify_order_by_id on a fully matched (non-resting) id is a no-op
template <OrderbookConcept OB>
void
test_modify_fully_matched_order_is_noop()
{
    std::cout << "Test 34: Modify a fully matched order is a no-op" << std::endl;
    OB ob;
    ob.match_order(Order{1, 100, 5, Side::SELL});
    // Fully consume order 1.
    ob.match_order(Order{2, 100, 5, Side::BUY});
    assert(!ob.order_exists(1));

    // Modifying the gone id should not crash or create a ghost order.
    ob.modify_order_by_id(1, 10);
    assert(!ob.order_exists(1));
    assert(ob.get_volume_at_level(Side::SELL, 100) == 0);
    std::cout << "Test 34 passed." << std::endl;
}

int
main()
{
    test_lookup_order<Orderbook>();
    test_simple_match_and_modify<Orderbook>();
    test_multiple_matches<Orderbook>();
    test_sell_order_matching_buy<Orderbook>();
    test_full_fill_buy_order_exact_match<Orderbook>();
    test_partial_fill_buy_order_across_multiple_sell_levels<Orderbook>();
    test_modify_nonexistent_order<Orderbook>();
    test_partial_modification<Orderbook>();
    test_partial_fill_sell_order_across_multiple_buy_levels<Orderbook>();
    test_exact_price_mismatch_no_fill<Orderbook>();
    test_multiple_partial_fills_same_level<Orderbook>();
    test_order_book_integrity_after_multiple_operations<Orderbook>();
    test_multiple_orders_same_side_ordering<Orderbook>();
    test_full_match_sell_order_exact_match<Orderbook>();
    test_modify_no_change<Orderbook>();
    test_modify_after_partial_fill<Orderbook>();
    test_modify_preserves_fifo<Orderbook>();
    test_multiple_modifications<Orderbook>();
    test_modify_with_zero_removes_order<Orderbook>();
    test_get_volume_no_orders<Orderbook>();
    test_get_volume_single_order<Orderbook>();
    test_get_volume_multiple_orders_same_level<Orderbook>();
    test_get_volume_orders_different_levels<Orderbook>();
    test_get_volume_after_partial_fill<Orderbook>();
    test_get_volume_after_cancellation<Orderbook>();
    test_get_volume_complex1<Orderbook>();
    test_get_volume_complex2<Orderbook>();
    test_get_volume_complex3<Orderbook>();
    test_get_volume_all_encompassing<Orderbook>();
    test_fully_matched_aggressor_does_not_rest<Orderbook>();
    test_volume_query_is_side_specific<Orderbook>();
    test_aggressor_sweeps_all_levels_and_rests<Orderbook>();
    test_cancel_then_resubmit_same_id<Orderbook>();
    test_fifo_drain_many_orders_same_level<Orderbook>();
    test_modify_fully_matched_order_is_noop<Orderbook>();
    std::cout << "All tests passed." << std::endl;
    return 0;
}
