#include "GenericTop_v5.h"

void GenericTop(
  BXType bx,
  const FullMatchMemory<BARREL>* in,
  BXType& bx_o,
  FullMatchMemory<BARREL>* out
){

#pragma HLS dataflow

  BXType bx_1 = bx-1;
  BXType bx_2 = bx-2;
  static BXType bx_o_1; 
  static BXType bx_o_2;
  static FullMatchMemory<BARREL> tmp1;
  static FullMatchMemory<BARREL> tmp2;
  static FullMatchMemory<BARREL> tmp3;
  static FullMatchMemory<BARREL> tmp4;
  static FullMatchMemory<BARREL> tmp5;
 
  TOP_1: Generic<BARREL,0>
  (
    bx,
    in,
    bx_o_1,
    out,      // &tmp1
    &tmp1     // &tmp2
  );

  /*
  TOP_2: Generic<BARREL,1>
  (
    bx_1,
    &tmp1,
    bx_o_2,
    &tmp3,
    &tmp4
  );

  TOP_3: Generic<BARREL,2>
  (
    bx_2,
    &tmp2,
    bx_o,
    out,
    &tmp5
  );
  */
 
} 
