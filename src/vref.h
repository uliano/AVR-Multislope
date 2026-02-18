#pragma once
#include <avr/io.h>

static inline void init_vref(void){
    VREF.ADC0REF = VREF_ALWAYSON_bm | VREF_REFSEL_VREFA_gc;
    VREF.ACREF = VREF_ALWAYSON_bm | VREF_REFSEL_VREFA_gc;
}