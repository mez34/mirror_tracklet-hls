#ifndef MATCHCALCULATOR_HH
#define MATCHCALCULATOR_HH

// MatchCalculator

#include "Constants.hh"
#include "CandidateMatchMemory.hh"
#include "AllStubMemory.hh"
#include "AllProjectionMemory.hh"
#include "FullMatchMemory.hh"
//#include "merger.hh"

//////////////////////////////////////////////////////////////////////////
//                         DEBUG FUNCTIONS
//////////////////////////////////////////////////////////////////////////
#include <stdarg.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

typedef hls::stream<char> dbg_stream;
//void printDebug(const char * text, dbg_stream & streamOut);
//void printU32Debug(ap_uint<32> dataIn, dbg_stream & streamOut);

template<typename T>
void printDebug(const char * text, dbg_stream & streamOut) {
	ap_uint<32> index = 0;
	while (1) {
		char textIn = text[index];
		if (textIn == 0) {
			break;
		}
		streamOut.write(textIn);
		index++;
	}

};

template<typename T>
void printU32Debug(ap_uint<32> dataIn, dbg_stream & streamOut) {
	ap_uint<32> subValue = 1000000000;
	bool hasStarted = false;
	while (subValue > 0) {
//		printf("DataIn %d, subValue %d\n",(int) dataIn, (int) subValue);
#pragma HLS UNROLL
		ap_uint<4> count = 0;
		while (dataIn >= subValue) {
			dataIn -= subValue;
			count++;
		}
		if ((count != 0) || (hasStarted) || (subValue == 1)) {
			streamOut.write(char(0x30 + count));
			hasStarted = true;
		}
		subValue /= 10;
	}
};
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////

template<int layer, int part>
void cleanmerger(
     // inputs
     CandidateMatch inA,
     bool validA,
     CandidateMatch inB,
     bool validB,
     CandidateMatch out,
     bool vout,
     bool inread,
     CandidateMatch A,
     bool vA,
     bool sA,
     CandidateMatch B,
     bool vB,
     bool sB,
     // outputs
     CandidateMatch * Anext,
     bool * vAnext,
     bool * sAnext,
     CandidateMatch * Bnext,
     bool * vBnext,
     bool * sBnext,
     CandidateMatch * outnext,
     bool * voutnext,
     bool * readA,
     bool * readB
){

#pragma HLS inline 
#pragma HLS pipeline II=1
#pragma HLS interface ap_ctrl_none port=return 
#pragma HLS interface ap_ctrl_none port=inA,validA,inB,validB,out,vout,inread,A,vA,sA,B,vB,sB 
#pragma HLS interface ap_ctrl_none port=Anext,Bnext,vAnext,vBnext,sAnext,sBnext,voutnext 
//#pragma HLS dependence variable=Anext,Bnext,vAnext,vBnext,sAnext,sBnext,outnext,voutnext,readA,readB intra false

    // Set read enables for A and B
    *readA = (((inread || !vout) && sA) || !vA) && validA;
    *readB = (((inread || !vout) && sB) || !vB) && validB;

    // Setup state machine
    static enum {HOLD, PROC_A, PROC_B, START, DONE} state;
    if (sA && (inread || !vout))                          state = PROC_A;
    else if (sB && (inread || !vout))                     state = PROC_B; 
    else if ((!sA && !sB) && (validA || validB) && !vout) state = START;
    else if (!sB && !sA && vout && inread)                state = DONE;              
    else                                                  state = HOLD; 

    // Case statements
    switch(state)
    {
    case PROC_A: // just readA and compare inA with pipelined B
        *outnext  = A;      // output is A
    	*voutnext = vA;     // output valid is vA
    	*Anext    = inA;    // pipeline inA
    	*vAnext   = validA; // pipeline inA valid
        *Bnext    = B;      // pipeline B
        *vBnext   = vB;     // pipeline vB
        *sAnext   = ((inA.getProjIndex() <= B.getProjIndex()) || !vB) && validA;  // sA=true if inA is valid and (inA <= B or B not valid)
    	*sBnext   = (!(inA.getProjIndex() <= B.getProjIndex()) || !validA) && vB; // sB=true if B is valid and (inA > B or inA not valid)
    	break;
    case PROC_B: // just readB and compare inB with pipelined A
        *outnext  = B;      // output is B
    	*voutnext = vB;     // output valid is vB
    	*Bnext    = inB;    // pipeline inB
    	*vBnext   = validB; // pipeline inB valid
        *Anext    = A;      // pipeline A
        *vAnext   = vA;     // pipeline vA
        *sAnext   = ((A.getProjIndex() <= inB.getProjIndex()) || !validB) && vA;  // sA=true if A is valid and (A <= inB or inB not valid) 
        *sBnext   = (!(A.getProjIndex() <= inB.getProjIndex()) || !vA) && validB; // sB=true if inB is valid and (A > inB or A not valid)
    	break;
    case START: // both in at same time
        *outnext  = CandidateMatch();
    	*voutnext = false;
    	*Anext    = inA;    // pipeline inA
    	*vAnext   = validA; // pipeline inA valid
    	*Bnext    = inB;    // pipeline inB
    	*vBnext   = validB; // pipeline inB valid
        *sAnext   = ((inA.getProjIndex() <= inB.getProjIndex()) || !validB) && validA;  // sA=true if inA is valid and (inA <= inB or inB not valid)
        *sBnext   = (!(inA.getProjIndex() <= inB.getProjIndex()) || !validA) && validB; // sB=true if inB is valid and (inA > inB or inA not valid)
    	break;
    case DONE: // set everything to false 
        *outnext  = CandidateMatch();
        *voutnext = false;
        *Anext    = CandidateMatch();
        *vAnext   = false;
        *Bnext    = CandidateMatch();
        *vBnext   = false;
        *sAnext   = false;
        *sBnext   = false;
        break;
    case HOLD: // pipeline all 
        *outnext  = out;
        *voutnext = vout;
        *Anext    = A;
        *Bnext    = B;
        *vAnext   = vA;
        *vBnext   = vB;
        *sAnext   = sA;
        *sBnext   = sB; 
    	break;
    }

  //std::cout << "Layer: " << layer << " " << part << " , state: " << state << std::endl;
  //std::cout << "---Reads: (A,B,vout,inread) " << readA << " " << readB << " " << vout << " " << inread << std::endl; 
  //std::cout << "---In: " << inA.raw() << " " << validA << " " << inB.raw() << " " << validB << std::endl;
  //std::cout << "---Out: " << out.raw() << " " << *voutnext << std::endl;

}

//////////////////////////////////////////////////////////////

// Pipeline reads

template<typename T>
T pipeline_read(T in)
{

#pragma HLS inline off
//#pragma HLS interface register port=in
//#pragma HLS interface register port=return

  T out = in;
  return out;

};

//////////////////////////////////////////////////////////////

// Absolute value template

template< int width >
ap_uint<width> iabs( ap_int<width> value )
{
  ap_uint<width> absval;
  if (value < 0) absval = -value;
  else           absval = value;
  return absval;
};

//////////////////////////////////////////////////////////////

// Template to get look up tables

// Table for phi or z cuts
template<bool phi, int L, int width, int depth>
void readTable_Cuts(ap_uint<width> table[depth]){
  if (phi){ // phi cuts
    if (L==1){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L1PHIC_phicut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==2){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L2PHIC_phicut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==3){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L3PHIC_phicut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==4){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L4PHIC_phicut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==5){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L5PHIC_phicut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==6){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L6PHIC_phicut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else {
      static_assert(true, "Only LAYERS 1 to 6 are valid");
    }
  } // end phi cuts
  else { // z cuts
    if (L==1){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L1PHIC_zcut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==2){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L2PHIC_zcut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==3){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L3PHIC_zcut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==4){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L4PHIC_zcut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==5){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L5PHIC_zcut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else if (L==6){
      ap_uint<width> tmp[depth] =
#include "../emData/MC/MC_L3PHIC/MC_L6PHIC_zcut.tab"
      for (int i = 0; i < depth; i++) table[i] = tmp[i];
    }
    else {
      static_assert(true, "Only LAYERS 1 to 6 are valid");
    }
 
  }

} // end readTable_Cuts


//////////////////////////////////////////////////////////////

// MatchCalculator

template<regionType ASTYPE, regionType APTYPE, regionType FMTYPE, int LAYER=0, int DISK=0, int PHISEC=0>
void MatchCalculator(BXType bx,
                     const CandidateMatchMemory* match1,
                     const CandidateMatchMemory* match2,
                     const CandidateMatchMemory* match3,
                     const CandidateMatchMemory* match4,
                     const CandidateMatchMemory* match5,
                     const CandidateMatchMemory* match6,
                     const CandidateMatchMemory* match7,
                     const CandidateMatchMemory* match8,
                     const AllStubMemory<ASTYPE>* allstub,
                     const AllProjectionMemory<APTYPE>* allproj,
                     BXType& bx_o,
                     FullMatchMemory<FMTYPE>* fullmatch1,
                     FullMatchMemory<FMTYPE>* fullmatch2,
                     FullMatchMemory<FMTYPE>* fullmatch3,
                     FullMatchMemory<FMTYPE>* fullmatch4,
                     FullMatchMemory<FMTYPE>* fullmatch5,
                     FullMatchMemory<FMTYPE>* fullmatch6,
                     FullMatchMemory<FMTYPE>* fullmatch7,
                     hls::stream<char> & debug
){

//#pragma HLS pipeline II=108
//#pragma HLS latency max=140


  // Reset output memories
  fullmatch1->clear(bx);
  fullmatch2->clear(bx);
  fullmatch3->clear(bx);
  fullmatch4->clear(bx);
  fullmatch5->clear(bx);
  fullmatch6->clear(bx);
  fullmatch7->clear(bx);

  // Initialization
 
  // Setup constants depending on which layer/disk working on
  // probably should move these to constants file
  ap_uint<4> kNbitszprojL123 = 12; // nbitszprojL123 in emulation (defined in constants) 
  ap_uint<4> kNbitszprojL456 = 8;  // nbitszprojL456 in emulation (defined in constants)
  ap_uint<5> kNbitsdrinv = 19;     // idrinvbits     in emulation (defined in constants)
  ap_uint<4> kShift_Rinv = 13;     // rinvbitshift   in emulation (defined in constants)
  ap_uint<3> kShift_Phider = 7;    // phiderbitshift in emulation (defined in constants)
  ap_uint<3> kNbitsrL123 = 7;      // nbitsrL123     in emulation (defined in constants)
  ap_uint<3> kNbitsrL456 = 7;      // nbitsrL456     in emulation (defined in constants) 
  ap_int<4>  kShift_PS_zderL = -7; // PS_zderL_shift in emulation (defined in constants)
  ap_int<4>  kShift_2S_zderL = -7; // SS_zderL_shift in emulation (defined in constants)

  auto kFact               = (1 <= LAYER <= 3)? 1 : (1<<(kNbitszprojL123-kNbitszprojL456));                             // fact_ in emulation defined in MC
  auto kPhi0_shift         = (1 <= LAYER <= 3)? 3 : 0;                                                                  // phi0shift_ in emulation defined in MC
  auto kShift_phi0bit      = 1;                                                                                         // phi0bitshift in emulation defined in constants
  ap_uint<10> kPhi_corr_shift_L123 = 7 + kNbitsdrinv + kShift_phi0bit - kShift_Rinv - kShift_Phider;                    // icorrshift for L123
  ap_uint<10> kPhi_corr_shift_L456 = kPhi_corr_shift_L123 - 10 - kNbitsrL456;                                           // icorrshift for L456
  auto kPhi_corr_shift     = (1 <= LAYER <= 3)? kPhi_corr_shift_L123 : kPhi_corr_shift_L456;                            // icorrshift_ in emulation
  ap_uint<10> kZ_corr_shiftL123 = (-1-kShift_PS_zderL);                                                                 // icorzshift for L123 (6 in L3)
  ap_uint<10> kZ_corr_shiftL456 = (-1-kShift_2S_zderL + kNbitszprojL123 - kNbitszprojL456 + kNbitsrL456 - kNbitsrL123); // icorzshift for L456
  auto kZ_corr_shift       = (1 <= LAYER <= 3)? kZ_corr_shiftL123 : kZ_corr_shiftL456;                                  // icorzshift_ in emulation

  // Setup look up tables for match cuts
  ap_uint<17> LUT_matchcut_phi[7];
  readTable_Cuts<true,LAYER,17,7>(LUT_matchcut_phi);
  ap_uint<13> LUT_matchcut_z[7];
  readTable_Cuts<false,LAYER,13,7>(LUT_matchcut_z);

  // Pick up number of candidate matches for each CM memory
  ap_uint<kNBits_MemAddr> ncm1 = match1->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm2 = match2->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm3 = match3->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm4 = match4->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm5 = match5->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm6 = match6->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm7 = match7->getEntries(bx);
  ap_uint<kNBits_MemAddr> ncm8 = match8->getEntries(bx);

  // Count up total number of CMs *and protect incase of overflow)
  ap_uint<7> ncm;
  if (ncm1+ncm2+ncm3+ncm4+ncm5+ncm6+ncm7+ncm8 > kMaxProc) ncm = kMaxProc;
  else ncm = ncm1+ncm2+ncm3+ncm4+ncm5+ncm6+ncm7+ncm8;

  // Initialize read addresses for candidate matches
  ap_uint<kNBits_MemAddr> addr1 = 0;  
  ap_uint<kNBits_MemAddr> addr2 = 0;  
  ap_uint<kNBits_MemAddr> addr3 = 0;  
  ap_uint<kNBits_MemAddr> addr4 = 0;  
  ap_uint<kNBits_MemAddr> addr5 = 0;  
  ap_uint<kNBits_MemAddr> addr6 = 0;  
  ap_uint<kNBits_MemAddr> addr7 = 0;  
  ap_uint<kNBits_MemAddr> addr8 = 0; 

  // Read signals for the input candidate matches
  bool read1 = false;
  bool read2 = false;
  bool read3 = false;
  bool read4 = false;
  bool read5 = false;
  bool read6 = false;
  bool read7 = false;
  bool read8 = false;

  // Variables for the merger
  // layer 1 variables
  CandidateMatch cm_L1[4];
  CandidateMatch tmpA_L1[4];
  CandidateMatch tmpB_L1[4];
  bool valid_L1[4] = {false,false,false,false};
  bool read_L1[4] = {false,false,false,false};
  bool vA_L1[4] = {false,false,false,false};
  bool vB_L1[4] = {false,false,false,false};
  bool sA_L1[4] = {false,false,false,false};
  bool sB_L1[4] = {false,false,false,false};
  // layer 2 variables
  CandidateMatch cm_L2[2];
  CandidateMatch tmpA_L2[2];
  CandidateMatch tmpB_L2[2];
  bool valid_L2[2] = {false,false};
  bool read_L2[2] = {false,false};
  bool vA_L2[2] = {false,false};
  bool vB_L2[2] = {false,false};
  bool sA_L2[2] = {false,false};
  bool sB_L2[2] = {false,false};
  // layer 3 variables
  CandidateMatch tmpA_L3;
  CandidateMatch tmpB_L3;
  bool valid_L3 = false;
  bool vA_L3 = false;
  bool vB_L3 = false;
  bool sA_L3 = false;
  bool sB_L3 = false;
  
  // pipeline variables
  bool read_L1_next[4] = {false,false,false,false};
  bool read_L2_next[2] = {false,false};

  bool read_L1_1 = false;
  bool read_L1_2 = false;
  bool read_L1_3 = false;
  bool read_L1_4 = false;
  bool read_L1_1_next = false;
  bool read_L1_2_next = false;
  bool read_L1_3_next = false;
  bool read_L1_4_next = false;
  bool read_L2_1 = false;
  bool read_L2_2 = false;
  bool read_L2_1_next = false;
  bool read_L2_2_next = false;

  CandidateMatch cm_L1_next[4];
  CandidateMatch tmpA_L1_next[4];
  CandidateMatch tmpB_L1_next[4];
  bool valid_L1_next[4] = {false,false,false,false};
  bool vA_L1_next[4] = {false,false,false,false};
  bool vB_L1_next[4] = {false,false,false,false};
  bool sA_L1_next[4] = {false,false,false,false};
  bool sB_L1_next[4] = {false,false,false,false};
  CandidateMatch cm_L2_next[2];
  CandidateMatch tmpA_L2_next[2];
  CandidateMatch tmpB_L2_next[2];
  bool valid_L2_next[2] = {false,false};
  bool vA_L2_next[2] = {false,false};
  bool vB_L2_next[2] = {false,false};
  bool sA_L2_next[2] = {false,false};
  bool sB_L2_next[2] = {false,false};
  CandidateMatch cm_L3_next;
  CandidateMatch tmpA_L3_next;
  CandidateMatch tmpB_L3_next; 
  bool valid_L3_next = false;
  bool vA_L3_next = false;
  bool vB_L3_next = false;
  bool sA_L3_next = false;
  bool sB_L3_next = false;
  bool read1_next = false;
  bool read2_next = false;
  bool read3_next = false;
  bool read4_next = false;
  bool read5_next = false;
  bool read6_next = false;
  bool read7_next = false;
  bool read8_next = false;

#pragma HLS ARRAY_PARTITION variable=cm_L1,cm_L2,tmpA_L1,tmpB_L1,tmpA_L2,tmpB_L2 complete dim=0
#pragma HLS ARRAY_PARTITION variable=valid_L1,read_L1,vA_L1,vB_L1,sA_L1,sB_L1 complete dim=0
#pragma HLS ARRAY_PARTITION variable=valid_L2,read_L2,vA_L2,vB_L2,sA_L2,sB_L2 complete dim=0
#pragma HLS ARRAY_PARTITION variable=read_L1_next,read_L2_next complete dim=0
#pragma HLS ARRAY_PARTITION variable=tmpA_L1_next,tmpB_L1_next,tmpA_L2_next,tmpB_L2_next complete dim=0
#pragma HLS ARRAY_PARTITION variable=vA_L1_next,vB_L1_next,sA_L1_next,sB_L1_next complete dim=0
#pragma HLS ARRAY_PARTITION variable=vA_L2_next,vB_L2_next,sA_L2_next,sB_L2_next complete dim=0
#pragma HLS ARRAY_PARTITION variable=cm_L1_next,cm_L2_next,valid_L1_next,valid_L2_next complete dim=0

#pragma HLS resource variable=read_L1,read_L2,read_L1_next,read_L2_next core=register

//#pragma HLS resource variable=read_L1,read_L2 register


//#pragma HLS RESOURCE variable=cm_L1,cm_L2,tmpA_L1,tmpB_L1,tmpA_L2,tmpB_L2,valid_L1,read_L1,vA_L1,vB_L1,sA_L1,sB_L1,valid_L2,read_L2,vA_L2,vB_L2,sA_L2,sB_L2,read_L1_next,read_L2_next,tmpA_L1_next,tmpB_L1_next,tmpA_L2_next,tmpB_L2_next,vA_L1_next,vB_L1_next,sA_L1_next,sB_L1_next,vA_L2_next,vB_L2_next,sA_L2_next,sB_L2_next,valid_L1_next,valid_L2_next,cm_L1_next,cm_L2_next core=register

   // Setup candidate match data stream that goes into match calculations
  CandidateMatch datastream = CandidateMatch();

  // Setup dummy index to be used in the comparison
  CandidateMatch::CMProjIndex dummy = -1; 

  // Bool and ID needed for determining if processing a new tracklet
  bool newtracklet = true;
  CandidateMatch::CMProjIndex id[kMaxProc+1];

  // Initialize MC delta phi cut variables
  ap_uint<17> best_delta_phi;

  // Full match shift register to store best match
  FullMatch<FMTYPE> bestmatch      = FullMatch<FMTYPE>();
  FullMatch<FMTYPE> bestmatch_next = FullMatch<FMTYPE>();
  bool goodmatch                   = false;
  bool goodmatch_next              = false;
  typename AllProjection<APTYPE>::AProjTCSEED projseed;
  typename AllProjection<APTYPE>::AProjTCSEED projseed_next;

  // Bool to signal last processing
  bool last = false;

  //merge_top(bx, match1, match2, match3, match4, match5, match6, match7, match8);

  // Processing starts
  MC_LOOP: for (ap_uint<kNBits_MemAddr> istep = 0; istep < kMaxProc+3; istep++)
  {

#pragma HLS PIPELINE II=1
//#pragma HLS latency max=140

// no inter-loop dependencies between variables that are just written
#pragma HLS dependence variable=cm1,cm2,cm3,cm4,cm5,cm6,cm7,cm8 inter false
#pragma HLS dependence variable=cm_L1_next,tmpA_L1_next,tmpB_L1_next,vA_L1_next,vB_L1_next,sA_L1_next,sB_L1_next,valid_L1_next inter false
#pragma HLS dependence variable=cm_L2_next,tmpA_L2_next,tmpB_L2_next,vA_L2_next,vB_L2_next,sA_L2_next,sB_L2_next,valid_L2_next inter false
#pragma HLS dependence variable=cm_L3_next,tmpA_L3_next,tmpB_L3_next,vA_L3_next,vB_L3_next,sA_L3_next,sB_L3_next,valid_L3_next inter false
#pragma HLS dependence variable=read1_next,read2_next,read3_next,read4_next,read5_next,read6_next,read7_next,read8_next inter false
//#pragma HLS resource variable=bestmatch,projseed,goodmatch,bestmatch_next,projseed_next,goodmatch_next core=register

    if (istep==(kMaxProc-1) || istep==(ncm-1)) last = true;
    else last = false;

    if (istep < ncm+3){

      //-----------------------------------------------------------------------------------------------------------
      //-------------------------------- MERGE INPUT CANDIDATE MATCHES --------------------------------------------
      //-----------------------------------------------------------------------------------------------------------

      // Increment the read addresses for the candidate matches
      if (read1) addr1++;
      if (read2) addr2++;
      if (read3) addr3++;
      if (read4) addr4++;
      if (read5) addr5++;
      if (read6) addr6++;
      if (read7) addr7++;
      if (read8) addr8++;

      // Read in each candidate match
      const CandidateMatch &cm1 = match1->read_mem(bx,addr1);
      const CandidateMatch &cm2 = match2->read_mem(bx,addr2);
      const CandidateMatch &cm3 = match3->read_mem(bx,addr3); 
      const CandidateMatch &cm4 = match4->read_mem(bx,addr4); 
      const CandidateMatch &cm5 = match5->read_mem(bx,addr5); 
      const CandidateMatch &cm6 = match6->read_mem(bx,addr6); 
      const CandidateMatch &cm7 = match7->read_mem(bx,addr7); 
      const CandidateMatch &cm8 = match8->read_mem(bx,addr8); 

      // Valid signal for the candidate match
      bool valid1 = (addr1 < ncm1) && (ncm1 > 0);
      bool valid2 = (addr2 < ncm2) && (ncm2 > 0);
      bool valid3 = (addr3 < ncm3) && (ncm3 > 0);
      bool valid4 = (addr4 < ncm4) && (ncm4 > 0);
      bool valid5 = (addr5 < ncm5) && (ncm5 > 0);
      bool valid6 = (addr6 < ncm6) && (ncm6 > 0);
      bool valid7 = (addr7 < ncm7) && (ncm7 > 0);
      bool valid8 = (addr8 < ncm8) && (ncm8 > 0);

      // merger Layer 1 Part 1
      cleanmerger<1,1>(
        cm1, valid1, cm2, valid2,                         // inputs: inA, validA, inB, validB
        //cm_L1[0], valid_L1[0], read_L1[0],                // inputs: out, vout, inread from L2_1
        cm_L1[0], valid_L1[0], read_L1_1,                // inputs: out, vout, inread from L2_1
        tmpA_L1[0], vA_L1[0], sA_L1[0],                   // tmp variables internal to L1_1 merger
        tmpB_L1[0], vB_L1[0], sB_L1[0],                   // tmp variables internal to L1_1 merger
        &tmpA_L1_next[0], &vA_L1_next[0], &sA_L1_next[0], // tmp variables internal to L1_1 merger
        &tmpB_L1_next[0], &vB_L1_next[0], &sB_L1_next[0], // tmp variables internal to L1_1 merger
        &cm_L1_next[0], &valid_L1_next[0],                // outputs: out, vout
        &read1_next, &read2_next                          // outputs: read1, read2 
      );

      // merger Layer 1 Part 2
      cleanmerger<1,2>(
        cm3, valid3, cm4, valid4,                         // inputs: inA, validA, inB, validB
        //cm_L1[1], valid_L1[1], read_L1[1],                // inputs: out, vout, inread from L2_1
        cm_L1[1], valid_L1[1], read_L1_2,                // inputs: out, vout, inread from L2_1
        tmpA_L1[1], vA_L1[1], sA_L1[1],                   // tmp variables internal to L1_2 merger
        tmpB_L1[1], vB_L1[1], sB_L1[1],                   // tmp variables internal to L1_2 merger
        &tmpA_L1_next[1], &vA_L1_next[1], &sA_L1_next[1], // tmp variables internal to L1_2 merger
        &tmpB_L1_next[1], &vB_L1_next[1], &sB_L1_next[1], // tmp variables internal to L1_2 merger
        &cm_L1_next[1], &valid_L1_next[1],                // outputs: out, vout
        &read3_next, &read4_next                          // outputs: read3, read4 
      );

      // merger Layer 1 Part 3 
      cleanmerger<1,3>(
        cm5, valid5, cm6, valid6,                         // inputs: inA, validA, inB, validB
        //cm_L1[2], valid_L1[2], read_L1[2],                // inputs: out, vout, inread from L2_1
        cm_L1[2], valid_L1[2], read_L1_3,                // inputs: out, vout, inread from L2_1
        tmpA_L1[2], vA_L1[2], sA_L1[2],                   // tmp variables internal to L1_3 merger
        tmpB_L1[2], vB_L1[2], sB_L1[2],                   // tmp variables internal to L1_3 merger
        &tmpA_L1_next[2], &vA_L1_next[2], &sA_L1_next[2], // tmp variables internal to L1_3 merger
        &tmpB_L1_next[2], &vB_L1_next[2], &sB_L1_next[2], // tmp variables internal to L1_3 merger
        &cm_L1_next[2], &valid_L1_next[2],                // outputs: out, vout
        &read5_next, &read6_next                          // outputs: read5, read6 
      );

      // merger Layer 1 Part 4  
      cleanmerger<1,4>(
        cm7, valid7, cm8, valid8,                         // inputs: inA, validA, inB, validB
        //cm_L1[3], valid_L1[3], read_L1[3],                // inputs: out, vout, inread from L2_1
        cm_L1[3], valid_L1[3], read_L1_4,               // inputs: out, vout, inread from L2_1
        tmpA_L1[3], vA_L1[3], sA_L1[3],                   // tmp variables internal to L1_4 merger
        tmpB_L1[3], vB_L1[3], sB_L1[3],                   // tmp variables internal to L1_4 merger
        &tmpA_L1_next[3], &vA_L1_next[3], &sA_L1_next[3], // tmp variables internal to L1_4 merger
        &tmpB_L1_next[3], &vB_L1_next[3], &sB_L1_next[3], // tmp variables internal to L1_4 merger
        &cm_L1_next[3], &valid_L1_next[3],                // outputs: out, vout
        &read7_next, &read8_next                          // outputs: read7, read8 
      );

      // merger Layer 2 Part 1
      cleanmerger<2,1>(
        cm_L1_next[0], valid_L1_next[0], cm_L1_next[1], valid_L1_next[1], // inputs: inA, validA, inB, validB
        //cm_L2[0], valid_L2[0], read_L2[0],                // inputs: out, vout, inread from L3_1
        cm_L2[0], valid_L2[0], read_L2_1,                // inputs: out, vout, inread from L3_1
        tmpA_L2[0], vA_L2[0], sA_L2[0],                   // tmp variables internal to L2_1 merger
        tmpB_L2[0], vB_L2[0], sB_L2[0],                   // tmp variables internal to L2_1 merger
        &tmpA_L2_next[0], &vA_L2_next[0], &sA_L2_next[0], // tmp variables internal to L2_1 merger
        &tmpB_L2_next[0], &vB_L2_next[0], &sB_L2_next[0], // tmp variables internal to L2_1 merger
        &cm_L2_next[0], &valid_L2_next[0],                // outputs: out, vout
        //&read_L1_next[0], &read_L1_next[1]                // outputs: read_L1_1, read_L1_2
        &read_L1_1_next, &read_L1_2_next                // outputs: read_L1_1, read_L1_2
      );

      // merger Layer 2 Part 2
      cleanmerger<2,2>(
        cm_L1_next[2], valid_L1_next[2], cm_L1_next[3], valid_L1_next[3], // inputs: inA, validA, inB, validB
        //cm_L2[1], valid_L2[1], read_L2[1],                // inputs: out, vout, inread from L3_1
        cm_L2[1], valid_L2[1], read_L2_2,                // inputs: out, vout, inread from L3_1
        tmpA_L2[1], vA_L2[1], sA_L2[1],                   // tmp variables internal to L2_2 merger
        tmpB_L2[1], vB_L2[1], sB_L2[1],                   // tmp variables internal to L2_2 merger
        &tmpA_L2_next[1], &vA_L2_next[1], &sA_L2_next[1], // tmp variables internal to L2_2 merger
        &tmpB_L2_next[1], &vB_L2_next[1], &sB_L2_next[1], // tmp variables internal to L2_2 merger
        &cm_L2_next[1], &valid_L2_next[1],                // outputs: out, vout
        //&read_L1_next[2], &read_L1_next[3]                // outputs: read_L1_3, read_L1_4
        &read_L1_3_next, &read_L1_4_next                // outputs: read_L1_3, read_L1_4
      );

      // merger Layer 3 Part 1
      cleanmerger<3,1>(
        cm_L2_next[0], valid_L2_next[0], cm_L2_next[1], valid_L2_next[1], // inputs: inA, validA, inB, validB
        datastream, valid_L3, true,                       // inputs: out, vout, true inread because last layer
        tmpA_L3, vA_L3, sA_L3,                            // tmp variables internal to L3_1 merger
        tmpB_L3, vB_L3, sB_L3,                            // tmp variables internal to L3_1 merger
        &tmpA_L3_next, &vA_L3_next, &sA_L3_next,          // tmp variables internal to L3_1 merger
        &tmpB_L3_next, &vB_L3_next, &sB_L3_next,          // tmp variables internal to L3_1 merger
        &cm_L3_next, &valid_L3_next,                      // outputs: out, vout
        //&read_L2_next[0], &read_L2_next[1]                // outputs: read_L2_1, read_L2_2
        &read_L2_1_next, &read_L2_2_next                // outputs: read_L2_1, read_L2_2
      );

      // pipeline the variables
      // set up the inputs for the next iteration of the loop

      read1      = read1_next;
      read2      = read2_next;
      read3      = read3_next;
      read4      = read4_next;
      read5      = read5_next;
      read6      = read6_next;
      read7      = read7_next;
      read8      = read8_next;

      //read_L1[0] = read_L1_next[0];
      //read_L1[1] = read_L1_next[1];
      //read_L1[2] = read_L1_next[2];
      //read_L1[3] = read_L1_next[3];
      //read_L2[0] = read_L2_next[0];
      //read_L2[1] = read_L2_next[1];

      read_L1_1 = read_L1_1_next;
      read_L1_2 = read_L1_2_next;
      read_L1_3 = read_L1_3_next;
      read_L1_4 = read_L1_4_next;
      read_L2_1 = read_L2_1_next;
      read_L2_2 = read_L2_2_next;

      cm_L1[0]    = cm_L1_next[0];
      cm_L1[1]    = cm_L1_next[1];
      cm_L1[2]    = cm_L1_next[2];
      cm_L1[3]    = cm_L1_next[3];
      cm_L2[0]    = cm_L2_next[0];
      cm_L2[1]    = cm_L2_next[1];
      datastream  = cm_L3_next;

      valid_L1[0] = valid_L1_next[0];
      valid_L1[1] = valid_L1_next[1];
      valid_L1[2] = valid_L1_next[2];
      valid_L1[3] = valid_L1_next[3];
      valid_L2[0] = valid_L2_next[0];
      valid_L2[1] = valid_L2_next[1];
      valid_L3    = valid_L3_next;

      tmpA_L1[0] = tmpA_L1_next[0];
      tmpA_L1[1] = tmpA_L1_next[1];
      tmpA_L1[2] = tmpA_L1_next[2];
      tmpA_L1[3] = tmpA_L1_next[3];
      tmpB_L1[0] = tmpB_L1_next[0];
      tmpB_L1[1] = tmpB_L1_next[1];
      tmpB_L1[2] = tmpB_L1_next[2];
      tmpB_L1[3] = tmpB_L1_next[3];
      tmpA_L2[0] = tmpA_L2_next[0];
      tmpA_L2[1] = tmpA_L2_next[1];
      tmpB_L2[0] = tmpB_L2_next[0];
      tmpB_L2[1] = tmpB_L2_next[1];
      tmpA_L3    = tmpA_L3_next;
      tmpB_L3    = tmpB_L3_next;
     
      vA_L1[0]   = vA_L1_next[0]; 
      vA_L1[1]   = vA_L1_next[1]; 
      vA_L1[2]   = vA_L1_next[2]; 
      vA_L1[3]   = vA_L1_next[3]; 
      vB_L1[0]   = vB_L1_next[0]; 
      vB_L1[1]   = vB_L1_next[1]; 
      vB_L1[2]   = vB_L1_next[2]; 
      vB_L1[3]   = vB_L1_next[3]; 
      vA_L2[0]   = vA_L2_next[0]; 
      vA_L2[1]   = vA_L2_next[1]; 
      vB_L2[0]   = vB_L2_next[0]; 
      vB_L2[1]   = vB_L2_next[1];
      vA_L3      = vA_L3_next;
      vB_L3      = vB_L3_next; 

      sA_L1[0]   = sA_L1_next[0]; 
      sA_L1[1]   = sA_L1_next[1]; 
      sA_L1[2]   = sA_L1_next[2]; 
      sA_L1[3]   = sA_L1_next[3]; 
      sB_L1[0]   = sB_L1_next[0]; 
      sB_L1[1]   = sB_L1_next[1]; 
      sB_L1[2]   = sB_L1_next[2]; 
      sB_L1[3]   = sB_L1_next[3]; 
      sA_L2[0]   = sA_L2_next[0]; 
      sA_L2[1]   = sA_L2_next[1]; 
      sB_L2[0]   = sB_L2_next[0]; 
      sB_L2[1]   = sB_L2_next[1];
      sA_L3      = sA_L3_next;
      sB_L3      = sB_L3_next; 

      //-----------------------------------------------------------------------------------------------------------
      //-------------------------------------- MATCH CALCULATION STEPS --------------------------------------------
      //-----------------------------------------------------------------------------------------------------------

      printDebug<char>("Step: ",debug);
      printU32Debug<int>(istep,debug);
      printDebug<char>(" datastream: ",debug);
      printU32Debug<int>(datastream.raw(),debug);
      printDebug<char>("\n",debug); 

      // Extract the stub and projection indices from the candidate match
      auto projid = datastream.getProjIndex();
      auto stubid = datastream.getStubIndex();
      // Use the stub and projection indices to pick up the stub and projection
      AllProjection<APTYPE> proj = allproj->read_mem(bx,projid);
      AllStub<ASTYPE>       stub = allstub->read_mem(bx,stubid);

      // Check if processing a new tracklet or not 
      // Later we only want to store the single best match per tracklet
      id[istep] = projid;
      if (istep==0) newtracklet = true;
      else if (id[istep] != id[istep-1]) newtracklet = true;
      else newtracklet = false; 

      // Stub parameters
      typename AllStub<ASTYPE>::ASR    stub_r    = stub.getR();
      typename AllStub<ASTYPE>::ASZ    stub_z    = stub.getZ();
      typename AllStub<ASTYPE>::ASPHI  stub_phi  = stub.getPhi();
      typename AllStub<ASTYPE>::ASBEND stub_bend = stub.getBend();       

      // Projection parameters
      typename AllProjection<APTYPE>::AProjTCID          proj_tcid = proj.getTCID();
      typename AllProjection<APTYPE>::AProjTrackletIndex proj_tkid = proj.getTrackletIndex();
      typename AllProjection<APTYPE>::AProjTCSEED        proj_seed = proj.getSeed();
      typename AllProjection<APTYPE>::AProjPHI           proj_phi  = proj.getPhi();
      typename AllProjection<APTYPE>::AProjRZ            proj_z    = proj.getRZ();
      typename AllProjection<APTYPE>::AProjPHIDER        proj_phid = proj.getPhiDer();
      typename AllProjection<APTYPE>::AProjRZDER         proj_zd   = proj.getRZDer(); 

      // Calculate residuals
      // Get phi and z correction
      ap_int<22> full_phi_corr = stub_r * proj_phid; // full corr has enough bits for full multiplication
      ap_int<18> full_z_corr   = stub_r * proj_zd;   // full corr has enough bits for full multiplication
      ap_int<11> phi_corr      = full_phi_corr >> kPhi_corr_shift;                        // only keep needed bits
      ap_int<12> z_corr        = (full_z_corr + (1<<(kZ_corr_shift-1))) >> kZ_corr_shift; // only keep needed bits
       
      // Apply the corrections
      ap_int<15> proj_phi_corr = proj_phi + phi_corr;  // original proj phi plus phi correction
      ap_int<13> proj_z_corr   = proj_z + z_corr;      // original proj z plus z correction

      // Get phi and z difference between the projection and stub
      ap_int<9> delta_z         = stub_z - proj_z_corr;
      ap_int<13> delta_z_fact   = delta_z * kFact;
      ap_int<18> stub_phi_long  = stub_phi;         // make longer to allow for shifting
      ap_int<18> proj_phi_long  = proj_phi_corr;    // make longer to allow for shifting
      ap_int<18> shiftstubphi   = stub_phi_long << kPhi0_shift;                        // shift
      ap_int<18> shiftprojphi   = proj_phi_long << (kShift_phi0bit - 1 + kPhi0_shift); // shift
      ap_int<17> delta_phi      = shiftstubphi - shiftprojphi;
      ap_uint<13> abs_delta_z   = iabs<13>( delta_z_fact ); // absolute value of delta z
      ap_uint<17> abs_delta_phi = iabs<17>( delta_phi );    // absolute value of delta phi

      // Full match parameters
      typename FullMatch<FMTYPE>::FMTCID          fm_tcid  = proj_tcid;
      typename FullMatch<FMTYPE>::FMTrackletIndex fm_tkid  = proj_tkid;
      typename FullMatch<FMTYPE>::FMSTUBPHIID     fm_asphi = PHISEC;
      typename FullMatch<FMTYPE>::FMSTUBID        fm_asid  = stubid;
      typename FullMatch<FMTYPE>::FMPHIRES        fm_phi   = delta_phi;
      typename FullMatch<FMTYPE>::FMZRES          fm_z     = delta_z;

      // Full match  
      FullMatch<FMTYPE> fm(fm_tcid,fm_tkid,fm_asphi,fm_asid,fm_phi,fm_z);

      //-----------------------------------------------------------------------------------------------------------
      //-------------------------------------- BEST MATCH LOGIC BLOCK ---------------------------------------------
      //-----------------------------------------------------------------------------------------------------------
 
      // pipeline the bestmatch registers 
      bestmatch      = bestmatch_next;
      goodmatch      = goodmatch_next;
      projseed       = projseed_next;

      // For first tracklet, pick up the phi cut value
      if (newtracklet) best_delta_phi = LUT_matchcut_phi[proj_seed];

      // Check that matches fall within the selection window of the projection 
      if ((abs_delta_z <= LUT_matchcut_z[proj_seed]) && (abs_delta_phi <= best_delta_phi)){
        // Update values of best phi parameters, so that the next match
        // will be compared to this value instead of the original selection cut
        best_delta_phi = abs_delta_phi;

        // Store bestmatch
        bestmatch_next = fm;
        goodmatch_next = true;
        projseed_next  = proj_seed;
      }
      else if (newtracklet){ // if is a new tracklet, do not make a match because it didn't pass the cuts
        bestmatch_next = FullMatch<FMTYPE>();
        goodmatch_next = false;
        projseed_next  = 0;
      }
      else { // if current match did not pass, but it is not a new tracklet, keep the previous best match for that tracklet
        bestmatch_next = bestmatch;
        goodmatch_next = goodmatch;
        projseed_next  = projseed;
      }

      // Write out only the best match, based on the seeding
      if (newtracklet && goodmatch==true){ // if there is a new tracklet write out the best match for the previous tracklet
        if (projseed==0) fullmatch1->write_mem(bx,bestmatch); // L1L2 seed
        if (projseed==1) fullmatch2->write_mem(bx,bestmatch); // L3L4 seed
        if (projseed==2) fullmatch3->write_mem(bx,bestmatch); // L5L6 seed
        if (projseed==3) fullmatch4->write_mem(bx,bestmatch); // D1D2 seed
        if (projseed==4) fullmatch5->write_mem(bx,bestmatch); // D3D4 seed
        if (projseed==5) fullmatch6->write_mem(bx,bestmatch); // L1D1 seed
        if (projseed==6) fullmatch7->write_mem(bx,bestmatch); // L2D1 seed
      }

      /*
      if (last){ // if this is the last iteration of loop, write out the current best also
        if (goodmatch[istep]==true && projseed[istep]==0) fullmatch1->write_mem(bx,bestmatch[istep]); // L1L2 seed
        if (goodmatch[istep]==true && projseed[istep]==1) fullmatch2->write_mem(bx,bestmatch[istep]); // L3L4 seed
        if (goodmatch[istep]==true && projseed[istep]==2) fullmatch3->write_mem(bx,bestmatch[istep]); // L5L6 seed
        if (goodmatch[istep]==true && projseed[istep]==3) fullmatch4->write_mem(bx,bestmatch[istep]); // D1D2 seed
        if (goodmatch[istep]==true && projseed[istep]==4) fullmatch5->write_mem(bx,bestmatch[istep]); // D3D4 seed
        if (goodmatch[istep]==true && projseed[istep]==5) fullmatch6->write_mem(bx,bestmatch[istep]); // L1D1 seed
        if (goodmatch[istep]==true && projseed[istep]==6) fullmatch7->write_mem(bx,bestmatch[istep]); // L2D1 seed
      }
      */

     }
     else break; // end processing of CMs
  }// end MC_LOOP 

}// end MatchCalculator

#endif
 
