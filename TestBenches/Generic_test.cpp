// Test bench for the MatchCalculator 
//#include "GenericTop.h"
//#include "GenericTop_v2.h"
//#include "GenericTop_v3.h"
//#include "GenericTop_v4.h"
#include "GenericTop_v5.h"

#include "FileReadUtility.hh"
#include "Constants.hh"

#include "Streamer.hh"
#include "hls_stream.h"
#include "hls_math.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>


const int nevents = 10; // number of events to run
bool truncation = true; // compare results to truncated emulation

using namespace std;

  typedef ap_uint<FullMatchBase<BARREL>::kFullMatchSize> FullMatchData;

int main() {
  // error counter
  int err_count = 0;

  // declare input memory arrays to be read from the emulation files
  static FullMatchMemory<BARREL> in;

  // declare output memory array to be filled by hls simulation
  static FullMatchMemory<BARREL> out;

  // read in input files
  ifstream fin;
  if (not openDataFile(fin,"emData/MC/MC_L3PHIC/FullMatches_FM_L1L2_L3PHIC_04.dat")) return -1;

  // open file(s) with reference results
  ifstream fout;
  if (not openDataFile(fout,"emData/MC/MC_L3PHIC/FullMatches_FM_L1L2_L3PHIC_04.dat")) return -1;
  
  hls::stream<FullMatchData> streamin;
  hls::stream<BXType> bx;

  ap_uint<45> inarray[nevents][20]; 
  ap_uint<45> outarray[nevents][20]; 

  // loop over events
  for (int ievt = 0; ievt < nevents; ++ievt) {

    // make memories from the input text files
    writeMemFromFile<FullMatchMemory<BARREL> >(in, fin, ievt);

    // convert to stream
    //static Streamer instreamer;
    //instreamer.mem2stream<FullMatchMemory<BARREL>,FullMatchData>(ievt, in, streamin); 
    M2S_LOOP: for (int i=0; i < kMaxProc; i++){
      if (i < in.getEntries(ievt)){
        FullMatch<BARREL> fm = in.read_mem(ievt, i);
        bool success = streamin.write_nb(fm.raw());
        assert(success);
        bx.write_nb(ievt);
      }
    }

    // convert to flat array
    M2A_LOOP: for (int i=0; i < kMaxProc; i++){
      if (i < in.getEntries(ievt)){
        FullMatch<BARREL> fm = in.read_mem(ievt, i);
        inarray[ievt][i] = fm.raw(); 
      } 
    }

  }
  
  // output stream
  //hls::stream<FullMatchData> streamout;
  //for (int ievt = 0; ievt < nevents; ++ievt) {
    
    // Unit Under Test
    //GenericTop(&bx, &streamin, &streamout);
    
    //BXType i = ievt;
    //GenericTop(i, &streamin, &streamout);

    //for (int entry = 0; entry < in.getEntries(ievt); ++entry){ 
    //  // Pickup individual entries
    //  FullMatch<BARREL> inmem = in.read_mem(ievt,entry);
    //  FullMatchData input = inmem.raw();
    //  // Unit under test
    //  GenericTop(ievt, &inmem, &out); 
    //  //GenericTop(ievt, &input, &out); 
    //}

  //// Unit Under Test
  //GenericTop(&in, &out);

  
  // loop over events
  for (int ievt = 0; ievt < nevents; ++ievt) {

    //// make memories from the input text files
    //writeMemFromFile<FullMatchMemory<BARREL> >(in, fin, ievt);

    //set bunch crossing
    BXType bx = ievt;
    BXType bx_out;

    //// Unit Under Test
    //GenericTop( bx, inarray, bx_out, outarray);

    // Unit Under Test
    GenericTop( bx, &in, bx_out, &out);

    // compare the computed outputs with the expected ones 
    err_count += compareMemWithFile<FullMatchMemory<BARREL> >(out, fout, ievt, "FullMatch", truncation);

  }  // end of event loop
  
  //for (int ievt=0; ievt<nevents; ++ievt){
  //  std::cout << "------ Event: " << ievt << std::endl;
  //  for (int nen=0; nen<20; ++nen){
  //    std::cout << outarray[ievt][nen] << std::endl;
  //  }
  //} 
 
  // close files
  fin.close();
  fout.close();

  return 0; //err_count;
}
