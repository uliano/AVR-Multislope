/* 
 * File:   utils.h
 * Author: uliano
 *
 * Created on April 8, 2024, 1:03 PM
 */

#pragma once

template <typename T>
constexpr bool is_powerof2(T v) {
    return v > 0 && ((v & (v - 1)) == 0);
}

// Minimal implementation of is_same (since type_traits is missing)
template <typename A, typename B>
struct is_same { static constexpr bool value = false; };

template <typename A>
struct is_same<A, A> { static constexpr bool value = true; };
