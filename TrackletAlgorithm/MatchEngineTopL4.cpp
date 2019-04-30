#include "MatchEngineTopL4.h"

void MatchEngineTopL4(const BXType bx,
		      const VMStubMEMemory<BARREL2S>* const instubdata,
		      const VMProjectionMemory<BARREL>* const inprojdata,
		      CandidateMatchMemory* const outcandmatch){
  

  MatchEngine<4, BARREL2S>(bx,
			   instubdata,
			   inprojdata,
			   outcandmatch);
  
}

