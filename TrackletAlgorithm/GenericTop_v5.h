#ifndef MATCHCALCULATORTOP_H
#define MATCHCALCULATORTOP_H

#include "Constants.hh"
#include "FullMatchMemory.hh"
#include "Generic.hh"
#include "hls_stream.h"

typedef ap_uint<FullMatchBase<BARREL>::kFullMatchSize> FullMatchData;

void GenericTop(
  BXType,
  const FullMatchMemory<BARREL>*,
  BXType &,
  FullMatchMemory<BARREL>*
);

#endif
