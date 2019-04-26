#ifndef MATCHCALCULATOR_HH
#define MATCHCALCULATOR_HH

// MatchCalculator

#include "Constants.hh"
#include "CandidateMatchMemory.hh"
#include "AllStubMemory.hh"
#include "AllProjectionMemory.hh"
#include "FullMatchMemory.hh"

//////////////////////////////////////////////////////////////

template<int layer, int part>
void merger(
     int i,
     // inputs
     CandidateMatch inA,
     bool validA,
     CandidateMatch inB,
     bool validB,
     bool inread,
     CandidateMatch * out,
     bool * vout,
     bool * readA,
     bool * readB
){

#pragma HLS inline 
#pragma HLS pipeline II=1
#pragma HLS interface ap_ctrl_none port=return 
#pragma HLS interface ap_ctrl_none port=inA,validA,inB,validB,out,vout,inread,

    // internal variables
    static CandidateMatch A = CandidateMatch();
    static CandidateMatch B = CandidateMatch();
    static CandidateMatch o = CandidateMatch();
    static bool vo = false;
    static bool vA = false;
    static bool vB = false;
    static bool sA = false;
    static bool sB = false; 

    // Set read enables for A and B
    *readA = (((inread || !*vout) && sA) || !vA) && validA;
    *readB = (((inread || !*vout) && sB) || !vB) && validB;

    // Setup state machine
    static enum {HOLD, PROC_A, PROC_B, START, DONE} state;
    if (i==0)                                              state = START;
    else if (sA && (inread || !*vout))                     state = PROC_A;
    else if (sB && (inread || !*vout))                     state = PROC_B; 
    else if ((!sA && !sB) && (validA || validB) && !*vout) state = START;
    else if (!sB && !sA && *vout && inread)                state = DONE;              
    else                                                   state = HOLD; 

    //------------- Explanation of the states -------------
    // START:  Either inA or inB is valid & there is no valid output & neither sA or sB is set
    //         No output yet, but set the next sA and sB and pipeline the inputs  
    // PROC_A: sA is set & either there is an inread from the next layer or not valid output
    //         Output is the pipelined A, and set the next sA and sB   
    // PROC_B: sB is set & either there is an inread from the next layer or not valid output
    //         Output is the pipelined B, and set the next sA and sB
    // DONE:   There is an valid output & there is no inread from the next layer and neither sA or sB is set
    //         No output, and set all reads to false
    // HOLD:   In all other cases, pipeline everthing 
    //-----------------------------------------------------

    // Case statements
    switch(state)
    {
    case PROC_A: // just readA and compare inA with pipelined B
        o  = A;      // output is A
        vo = vA;     // output valid is vA
    	A  = inA;    // pipeline inA
    	vA = validA; // pipeline inA valid
        sA = ((inA.getProjIndex() <= B.getProjIndex()) || !vB) && validA;  // sA=true if inA is valid and (inA <= B or B not valid)
    	sB = (!(inA.getProjIndex() <= B.getProjIndex()) || !validA) && vB; // sB=true if B is valid and (inA > B or inA not valid)
        break;
    case PROC_B: // just readB and compare inB with pipelined A
        o  = B;      // output is B
    	vo = vB;     // output valid is vB
    	B  = inB;    // pipeline inB
    	vB = validB; // pipeline inB valid
        sA = ((A.getProjIndex() <= inB.getProjIndex()) || !validB) && vA;  // sA=true if A is valid and (A <= inB or inB not valid) 
        sB = (!(A.getProjIndex() <= inB.getProjIndex()) || !vA) && validB; // sB=true if inB is valid and (A > inB or A not valid)
        break;
    case START: // both in at same time
        o  = CandidateMatch();
    	vo = false;
    	A  = inA;    // pipeline inA
    	vA = validA; // pipeline inA valid
    	B  = inB;    // pipeline inB
    	vB = validB; // pipeline inB valid
        sA = ((inA.getProjIndex() <= inB.getProjIndex()) || !validB) && validA;  // sA=true if inA is valid and (inA <= inB or inB not valid)
        sB = (!(inA.getProjIndex() <= inB.getProjIndex()) || !validA) && validB; // sB=true if inB is valid and (inA > inB or inA not valid)
        break;
    case DONE: // set everything to false 
        o  = CandidateMatch();
        vo = false;
        A  = CandidateMatch();
        vA = false;
        B  = CandidateMatch();
        vB = false;
        sA = false;
        sB = false;
        break;
    case HOLD: // pipeline all
        break;
    }

    *out  = o;
    *vout = vo; 


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
                     FullMatchMemory<FMTYPE>* fullmatch7
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
  const ap_uint<4> kNbitszprojL123 = 12; // nbitszprojL123 in emulation (defined in constants) 
  const ap_uint<4> kNbitszprojL456 = 8;  // nbitszprojL456 in emulation (defined in constants)
  const ap_uint<5> kNbitsdrinv = 19;     // idrinvbits     in emulation (defined in constants)
  const ap_uint<4> kShift_Rinv = 13;     // rinvbitshift   in emulation (defined in constants)
  const ap_uint<3> kShift_Phider = 7;    // phiderbitshift in emulation (defined in constants)
  const ap_uint<3> kNbitsrL123 = 7;      // nbitsrL123     in emulation (defined in constants)
  const ap_uint<3> kNbitsrL456 = 7;      // nbitsrL456     in emulation (defined in constants) 
  const ap_int<4>  kShift_PS_zderL = -7; // PS_zderL_shift in emulation (defined in constants)
  const ap_int<4>  kShift_2S_zderL = -7; // SS_zderL_shift in emulation (defined in constants)

  const auto kFact               = (1 <= LAYER <= 3)? 1 : (1<<(kNbitszprojL123-kNbitszprojL456)); // fact_ in emulation defined in MC
  const auto kPhi0_shift         = (1 <= LAYER <= 3)? 3 : 0;                                      // phi0shift_ in emulation defined in MC
  const auto kShift_phi0bit      = 1;                                                             // phi0bitshift in emulation defined in constants
  const ap_uint<10> kPhi_corr_shift_L123 = 7 + kNbitsdrinv + kShift_phi0bit - kShift_Rinv - kShift_Phider;                    // icorrshift for L123
  const ap_uint<10> kPhi_corr_shift_L456 = kPhi_corr_shift_L123 - 10 - kNbitsrL456;                                           // icorrshift for L456
  const auto kPhi_corr_shift     = (1 <= LAYER <= 3)? kPhi_corr_shift_L123 : kPhi_corr_shift_L456;                            // icorrshift_ in emulation
  const ap_uint<10> kZ_corr_shiftL123 = (-1-kShift_PS_zderL);                                                                 // icorzshift for L123 (6 in L3)
  const ap_uint<10> kZ_corr_shiftL456 = (-1-kShift_2S_zderL + kNbitszprojL123 - kNbitszprojL456 + kNbitsrL456 - kNbitsrL123); // icorzshift for L456
  const auto kZ_corr_shift       = (1 <= LAYER <= 3)? kZ_corr_shiftL123 : kZ_corr_shiftL456;                                  // icorzshift_ in emulation

  // Setup look up tables for match cuts
  ap_uint<17> LUT_matchcut_phi[7];
  readTable_Cuts<true,LAYER,17,7>(LUT_matchcut_phi);
  ap_uint<13> LUT_matchcut_z[7];
  readTable_Cuts<false,LAYER,13,7>(LUT_matchcut_z);

  // Initialize MC delta phi cut variables
  ap_uint<17> best_delta_phi;

  // Bool and ID needed for determining if processing a new tracklet
  CandidateMatch::CMProjIndex id;
  CandidateMatch::CMProjIndex id_next;

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
  //ap_uint<7> max   = kMaxProc;
  //ap_uint<7> ncm = (ncm1+ncm2+ncm3+ncm4+ncm5+ncm6+ncm7+ncm8 > max)? max : ncm1+ncm2+ncm3+ncm4+ncm5+ncm6+ncm7+ncm8; 
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
  bool read_L1_1 = false;
  bool read_L1_2 = false;
  bool read_L1_3 = false;
  bool read_L1_4 = false;
  CandidateMatch cm_L1_1 = CandidateMatch();
  CandidateMatch cm_L1_2 = CandidateMatch();
  CandidateMatch cm_L1_3 = CandidateMatch();
  CandidateMatch cm_L1_4 = CandidateMatch();
  bool valid_L1_1 = false; 
  bool valid_L1_2 = false; 
  bool valid_L1_3 = false; 
  bool valid_L1_4 = false; 

  // layer 2 variables
  bool read_L2_1 = false;
  bool read_L2_2 = false;
  CandidateMatch cm_L2_1 = CandidateMatch();
  CandidateMatch cm_L2_2 = CandidateMatch();
  bool valid_L2_1 = false; 
  bool valid_L2_2 = false; 

  // layer 3 variables
  bool valid_L3 = false;
  
  // Setup candidate match data stream that goes into match calculations
  CandidateMatch datastream = CandidateMatch();

  // Setup dummy index to be used in the comparison
  CandidateMatch::CMProjIndex dummy = -1; 


  // Full match shift register to store best match
  typename AllProjection<APTYPE>::AProjTCSEED projseed;
  FullMatch<FMTYPE> bestmatch      = FullMatch<FMTYPE>();
  bool goodmatch                   = false;
  //typename AllProjection<APTYPE>::AProjTCSEED projseed_next;
  //FullMatch<FMTYPE> bestmatch_next = FullMatch<FMTYPE>();
  //bool goodmatch_next              = false;

  // Processing starts
  MC_LOOP: for (ap_uint<kNBits_MemAddr> istep = 0; istep < kMaxProc+3; istep++)
  {

#pragma HLS PIPELINE II=1 enable_flush
#pragma HLS allocation instances=merger limit=6 function

    // pipeline variables
    bool read_L1_1_next = false;
    bool read_L1_2_next = false;
    bool read_L1_3_next = false;
    bool read_L1_4_next = false;
    CandidateMatch cm_L1_1_next = CandidateMatch();
    CandidateMatch cm_L1_2_next = CandidateMatch();
    CandidateMatch cm_L1_3_next = CandidateMatch();
    CandidateMatch cm_L1_4_next = CandidateMatch();
    bool valid_L1_1_next = false;
    bool valid_L1_2_next = false;
    bool valid_L1_3_next = false;
    bool valid_L1_4_next = false;
    bool read_L2_1_next = false;
    bool read_L2_2_next = false;
    CandidateMatch cm_L2_1_next = CandidateMatch();
    CandidateMatch cm_L2_2_next = CandidateMatch();
    bool valid_L2_1_next = false; 
    bool valid_L2_2_next = false; 
    CandidateMatch cm_L3_next;
    bool valid_L3_next = false;

    bool read1_next = false;
    bool read2_next = false;
    bool read3_next = false;
    bool read4_next = false;
    bool read5_next = false;
    bool read6_next = false;
    bool read7_next = false;
    bool read8_next = false;

    // Bool to signal last processing
    //bool last = (istep==(kMaxProc-1) || istep==(ncm-1))? true : false;

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
    merger<1,1>(
      istep, cm1, valid1, cm2, valid2,         // inputs: inA, validA, inB, validB
      read_L1_1,
      &cm_L1_1, &valid_L1_1,  // outputs: out, vout
      &read1_next, &read2_next          // outputs: read1, read2 
    );

    // merger Layer 1 Part 2
    merger<1,2>(
      istep, cm3, valid3, cm4, valid4,         // inputs: inA, validA, inB, validB
      read_L1_2,   // inputs: out, vout, inread from L2_1
      &cm_L1_2, &valid_L1_2,  // outputs: out, vout
      &read3_next, &read4_next          // outputs: read3, read4 
    );

    // merger Layer 1 Part 3 
    merger<1,3>(
      istep, cm5, valid5, cm6, valid6,         // inputs: inA, validA, inB, validB
      read_L1_3,   // inputs: out, vout, inread from L2_1
      &cm_L1_3, &valid_L1_3,  // outputs: out, vout
      &read5_next, &read6_next          // outputs: read5, read6 
    );

    // merger Layer 1 Part 4  
    merger<1,4>(
      istep, cm7, valid7, cm8, valid8,         // inputs: inA, validA, inB, validB
      read_L1_4,   // inputs: out, vout, inread from L2_1
      &cm_L1_4, &valid_L1_4,  // outputs: out, vout
      &read7_next, &read8_next          // outputs: read7, read8 
    );

    // merger Layer 2 Part 1
    merger<2,1>(
      istep, cm_L1_1, valid_L1_1,    // inputs: inA, validA
      cm_L1_2, valid_L1_2,    // inputs: inB, validB
      read_L2_1,   // inputs: out, vout, inread from L3_1
      &cm_L2_1, &valid_L2_1,  // outputs: out, vout
      &read_L1_1_next, &read_L1_2_next  // outputs: read_L1_1, read_L1_2
    );

    // merger Layer 2 Part 2
    merger<2,2>(
      istep, cm_L1_3, valid_L1_3,    // inputs: inA, validA 
      cm_L1_4, valid_L1_4,    // inputs: inB, validB
      read_L2_2,   // inputs: out, vout, inread from L3_1
      &cm_L2_2, &valid_L2_2,  // outputs: out, vout
      &read_L1_3_next, &read_L1_4_next  // outputs: read_L1_3, read_L1_4
    );

    // merger Layer 3 Part 1
    merger<3,1>(
      istep, cm_L2_1, valid_L2_1,      // inputs: inA, validA 
      cm_L2_2, valid_L2_2,      // inputs: inB, validB
      true,         // inputs: out, vout, true inread because last layer
      &datastream, &valid_L3,        // outputs: out, vout
      &read_L2_1_next, &read_L2_2_next    // outputs: read_L2_1, read_L2_2
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

    read_L1_1  = read_L1_1_next;
    read_L1_2  = read_L1_2_next;
    read_L1_3  = read_L1_3_next;
    read_L1_4  = read_L1_4_next;
    read_L2_1  = read_L2_1_next;
    read_L2_2  = read_L2_2_next;

    //cm_L1_1    = cm_L1_1_next;
    //cm_L1_2    = cm_L1_2_next;
    //cm_L1_3    = cm_L1_3_next;
    //cm_L1_4    = cm_L1_4_next;
    //cm_L2_1    = cm_L2_1_next;
    //cm_L2_2    = cm_L2_2_next;
    //datastream = cm_L3_next;

    //valid_L1_1 = valid_L1_1_next;
    //valid_L1_2 = valid_L1_2_next;
    //valid_L1_3 = valid_L1_3_next;
    //valid_L1_4 = valid_L1_4_next;
    //valid_L2_1 = valid_L2_1_next;
    //valid_L2_2 = valid_L2_2_next;
    //valid_L3   = valid_L3_next;

    //-----------------------------------------------------------------------------------------------------------
    //-------------------------------------- MATCH CALCULATION STEPS --------------------------------------------
    //-----------------------------------------------------------------------------------------------------------

    // Extract the stub and projection indices from the candidate match
    auto projid = datastream.getProjIndex();
    auto stubid = datastream.getStubIndex();
    // Use the stub and projection indices to pick up the stub and projection
    AllProjection<APTYPE> proj = allproj->read_mem(bx,projid);
    AllStub<ASTYPE>       stub = allstub->read_mem(bx,stubid);

    // Check if processing a new tracklet or not 
    // Later we only want to store the single best match per tracklet
    id      = id_next; // pipelined id
    id_next = projid;  
    bool newtracklet = (istep==0 || (id_next != id))? true : false;

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
 
    typename AllProjection<APTYPE>::AProjTCSEED projseed_next;
    FullMatch<FMTYPE> bestmatch_next = FullMatch<FMTYPE>();
    bool goodmatch_next              = false;

    // For first tracklet, pick up the phi cut value
    best_delta_phi = (newtracklet)? LUT_matchcut_phi[proj_seed] : best_delta_phi;

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
    fullmatch1->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==0)); // L1L2 seed
    fullmatch2->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==1)); // L3L4 seed
    fullmatch3->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==2)); // L5L6 seed
    fullmatch4->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==3)); // D1D2 seed
    fullmatch5->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==4)); // D3D4 seed
    fullmatch6->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==5)); // L1D1 seed
    fullmatch7->write_mem(bx,bestmatch,(newtracklet && goodmatch==true && projseed==6)); // L2D1 seed

    // pipeline the bestmatch registers 
    bestmatch      = bestmatch_next;
    goodmatch      = goodmatch_next;
    projseed       = projseed_next;

  }// end MC_LOOP 

  // Write out also the last match, based on the seeding
  fullmatch1->write_mem(bx,bestmatch,(goodmatch==true && projseed==0)); // L1L2 seed
  fullmatch2->write_mem(bx,bestmatch,(goodmatch==true && projseed==1)); // L3L4 seed
  fullmatch3->write_mem(bx,bestmatch,(goodmatch==true && projseed==2)); // L5L6 seed
  fullmatch4->write_mem(bx,bestmatch,(goodmatch==true && projseed==3)); // D1D2 seed
  fullmatch5->write_mem(bx,bestmatch,(goodmatch==true && projseed==4)); // D3D4 seed
  fullmatch6->write_mem(bx,bestmatch,(goodmatch==true && projseed==5)); // L1D1 seed
  fullmatch7->write_mem(bx,bestmatch,(goodmatch==true && projseed==6)); // L2D1 seed

}// end MatchCalculator

#endif
 
