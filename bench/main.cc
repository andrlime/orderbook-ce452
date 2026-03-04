#include <array>
#include <cstdint>
#include <cstdio>

#include "engine.hpp"

#ifdef USE_SNIPER_ROI
#  include "sim_api.h"
#endif

static constexpr PriceType PRICE_LO = 1000;
static constexpr int PRICE_LEVELS = 2000;
static constexpr PriceType PRICE_MID = PRICE_LO + PRICE_LEVELS / 2;

// IDs cycle in [1, MAX_ID]. Sized for the array impl's uint16_t-indexed table.
static constexpr IdType MAX_ID = 65534;

// Seed: 2000 levels × 16/side × 2 sides = 64 000 orders, just under MAX_ID.
static constexpr int SEED_PER_LEVEL = 16;
static constexpr int PRINT_EVERY_N = 10000;

static constexpr int BENCH_ROUNDS = 1'000'000'000;

// Ring buffer for tracking live passive order IDs to modify/cancel later.
static constexpr int RING_SIZE = 1 << 16; // 65536, power of 2
static constexpr int POOL = 1 << 14;      // 16 384 entries, ~96 KB total

struct Pools {
    std::array<PriceType, POOL> buy_px;     // passive buy prices  [LO, MID)
    std::array<PriceType, POOL> sell_px;    // passive sell prices [MID, HI)
    std::array<PriceType, POOL> query_px;   // query prices        [LO, HI)
    std::array<QuantityType, POOL> qty;     // order quantities
    std::array<QuantityType, POOL> mod_qty; // modify quantities (0 = cancel)
    std::array<int, POOL> spread;           // aggressive spread 1..3
};

static Pools
build_pools(uint64_t seed)
{
    Pools p;
    uint64_t s = seed;
    auto lcg = [&]() -> uint32_t {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<uint32_t>(s >> 33);
    };
    for (int i = 0; i < POOL; ++i) {
        p.buy_px[i] = static_cast<PriceType>(PRICE_LO + lcg() % (PRICE_LEVELS / 2));
        p.sell_px[i] = static_cast<PriceType>(PRICE_MID + lcg() % (PRICE_LEVELS / 2));
        p.query_px[i] = static_cast<PriceType>(PRICE_LO + lcg() % PRICE_LEVELS);
        p.qty[i] = static_cast<QuantityType>(1 + lcg() % 199);
        p.mod_qty[i] = static_cast<QuantityType>(lcg() % 50);
        p.spread[i] = static_cast<int>(1 + lcg() % 3);
    }
    return p;
}

template <OrderbookConcept OB>
[[gnu::noinline]] void
run_bench()
{
    OB ob;

    const Pools pools = build_pools(0xdeadbeefcafe1234ULL);

    static IdType ring[RING_SIZE];
    int ring_head = 0, ring_tail = 0;
    auto ring_push = [&](IdType id) {
        ring[ring_head & (RING_SIZE - 1)] = id;
        ++ring_head;
    };
    auto ring_pop = [&]() -> IdType {
        if (ring_tail == ring_head)
            return 0;
        return ring[ring_tail++ & (RING_SIZE - 1)];
    };

    // Allocate the next ID, cycling [1, MAX_ID].
    IdType next_id = 1;
    auto alloc_id = [&]() -> IdType {
        IdType id = next_id;
        next_id = next_id % MAX_ID + 1;
        return id;
    };

    for (int lvl = 0; lvl < PRICE_LEVELS; ++lvl) {
        PriceType p = static_cast<PriceType>(PRICE_LO + lvl);
        for (int k = 0; k < SEED_PER_LEVEL; ++k) {
            int ki = (lvl * SEED_PER_LEVEL + k) & (POOL - 1);
            if (p < PRICE_MID) {
                IdType id = alloc_id();
                ob.match_order(Order{id, p, pools.qty[ki], Side::BUY});
                ring_push(id);
            }
            if (p >= PRICE_MID) {
                IdType id = alloc_id();
                ob.match_order(Order{id, p, pools.qty[ki], Side::SELL});
                ring_push(id);
            }
        }
    }

    std::printf("Seeded. Starting %d rounds...\n", BENCH_ROUNDS);
    std::fflush(stdout);

#ifdef USE_SNIPER_ROI
    SimRoiStart();
#endif

    int pi = 0, qi = 0, mi = 0, si = 0, vi = 0;

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        if (round % PRINT_EVERY_N == 0) {
            std::printf("Reached round %u/%u\r", round, BENCH_ROUNDS);
            std::fflush(stdout);
        }

        int sp = pools.spread[si++ & (POOL - 1)];

        // Aggressive BUY — push so it gets cancelled if it rests
        {
            IdType id = alloc_id();
            ob.match_order(Order{id, static_cast<PriceType>(PRICE_MID + sp), pools.qty[qi++ & (POOL - 1)], Side::BUY});
            ring_push(id);
        }

        // Aggressive SELL
        {
            IdType id = alloc_id();
            ob.match_order(Order{id, static_cast<PriceType>(PRICE_MID - sp), pools.qty[qi++ & (POOL - 1)], Side::SELL});
            ring_push(id);
        }

        // Passive BUY restore
        {
            IdType id = alloc_id();
            ob.match_order(Order{id, pools.buy_px[pi++ & (POOL - 1)], pools.qty[qi++ & (POOL - 1)], Side::BUY});
            ring_push(id);
        }

        // Passive SELL restore
        {
            IdType id = alloc_id();
            ob.match_order(Order{id, pools.sell_px[pi++ & (POOL - 1)], pools.qty[qi++ & (POOL - 1)], Side::SELL});
            ring_push(id);
        }

        // Modify two old orders (one reduce, one cancel) — 4 pushes, 4 pops: net zero
        {
            IdType id = ring_pop();
            if (id)
                ob.modify_order_by_id(id, pools.mod_qty[mi++ & (POOL - 1)]);
        }
        {
            IdType id = ring_pop();
            if (id)
                ob.modify_order_by_id(id, pools.mod_qty[mi++ & (POOL - 1)]);
        }
        {
            IdType id = ring_pop();
            if (id)
                ob.modify_order_by_id(id, 0);
        }
        {
            IdType id = ring_pop();
            if (id)
                ob.modify_order_by_id(id, 0);
        }

        // Two volume queries
        [[maybe_unused]] volatile uint32_t v1 = ob.get_volume_at_level(Side::BUY, pools.query_px[vi++ & (POOL - 1)]);
        [[maybe_unused]] volatile uint32_t v2 = ob.get_volume_at_level(Side::SELL, pools.query_px[vi++ & (POOL - 1)]);
    }

#ifdef USE_SNIPER_ROI
    SimRoiEnd();
#endif

    std::printf("\rDone.\n");
}

int
main()
{
    run_bench<Orderbook>();
    return 0;
}
