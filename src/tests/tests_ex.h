#pragma once

#include <openenclave/internal/tests.h>

#define ASSERT_THROW(exp, exc)                    \
    try                                           \
    {                                             \
        exp;                                      \
        OE_TEST(false && "did not throw");        \
    }                                             \
    catch (exc)                                   \
    {                                             \
    }                                             \
    catch (...)                                   \
    {                                             \
        OE_TEST(false && "threw different type"); \
    }

#define ASSERT_NO_THROW(exp)       \
    try                            \
    {                              \
        exp;                       \
    }                              \
    catch (...)                    \
    {                              \
        OE_TEST(false && "threw"); \
    }
