/* 
 * File:   utils.cpp
 * Author: uliano
 *
 * Created on March 7, 2025, 08:23 AM
 */


// those two functions are needed (on AVR) to avoid linking errors
// that arise with virtual destructors in abstract classes
void operator delete(void*) noexcept {}
void operator delete(void*, unsigned int) noexcept {}