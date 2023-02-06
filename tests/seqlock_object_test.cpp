// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

//
// Created by Timur Doumler on 06/02/2023.
//

#include <crill/seqlock_object.h>
#include <doctest/doctest.h>
#include <thread>

TEST_CASE("crill::seqlock_object")
{
    struct coeffs
    {
        std::size_t a;
        bool b;
        std::size_t c;
    };

    static_assert(std::is_trivially_copyable_v<coeffs>);
    static_assert(!std::atomic<coeffs>::is_always_lock_free);

    SUBCASE("load default-constructed instance")
    {
        crill::seqlock_object<coeffs> obj;

        coeffs c = obj.load();
        REQUIRE(c.a == 0);
        REQUIRE(c.b == 0);
        REQUIRE(c.c == false);
    }

    SUBCASE("try_load default-constructed instance")
    {
        crill::seqlock_object<coeffs> obj;

        coeffs c;
        REQUIRE(obj.try_load(c));
        REQUIRE(c.a == 0);
        REQUIRE(c.b == 0);
        REQUIRE(c.c == false);
    }

    SUBCASE("load")
    {
        crill::seqlock_object<coeffs> obj(coeffs{1, true, 2});

        auto coeffs = obj.load();
        REQUIRE(coeffs.a == 1);
        REQUIRE(coeffs.b == true);
        REQUIRE(coeffs.c == 2);
    }

    SUBCASE("try_load")
    {
        crill::seqlock_object<coeffs> obj(coeffs{1, true, 2});

        coeffs c;
        REQUIRE(obj.try_load(c));
        REQUIRE(c.a == 1);
        REQUIRE(c.b == true);
        REQUIRE(c.c == 2);
    }

    SUBCASE("store")
    {
        crill::seqlock_object<coeffs> obj;
        obj.store(coeffs{1, true, 2});

        coeffs c;
        REQUIRE(obj.try_load(c));
        REQUIRE(c.a == 1);
        REQUIRE(c.b == true);
        REQUIRE(c.c == 2);
    }

    SUBCASE("Concurrent load/store")
    {
        crill::seqlock_object<coeffs> obj;
        std::atomic<bool> writer_started = false;
        std::atomic<bool> stop = false;

        std::thread writer([&] {
            writer_started = true;
            std::size_t i = 0;
            while (!stop) {
                obj.store(coeffs{i, true, i});
                ++i;
            }
        });

        while (!writer_started)
            /* wait */;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        coeffs c = {};
        for (std::size_t i = 0; i < 1000; ++i)
            c = obj.load();

        REQUIRE(c.a > 0);
        REQUIRE(c.b == true);
        REQUIRE(c.c == c.a); // no torn writes

        stop = true;
        writer.join();
    }
}
