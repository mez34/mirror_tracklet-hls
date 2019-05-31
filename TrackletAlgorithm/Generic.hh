#ifndef GENERIC_HH
#define GENERIC_HH

// Generic Module 

#include "Constants.hh"
#include "hls_stream.h"
#include "FullMatchMemory.hh"

constexpr int tmux=10;

template<class DataType>
void stream2stream(hls::stream<DataType>* sin, hls::stream<DataType>* sout)
{
  DataType dout;
  S2S_LOOP: for (int i = 0; i < tmux; ++i) {
    #pragma HLS pipeline II=1 rewind 
    DataType din;
    sin->read_nb(din);
    sout->write_nb(dout);
    dout = din;
  }
}

template<class DataType, class MemType>
void stream2mem(BXType bx, hls::stream<DataType>* sin, MemType* mem)
{
  S2M_LOOP: for (int i = 0; i < tmux; ++i) {
    #pragma HLS pipeline II=1 rewind
    DataType din;
    if (sin->read_nb(din)) {
      mem->write_mem(bx,din);
    }
  } 
}

template<class DataType, class MemType>
void mem2stream(BXType bx, MemType & mem, hls::stream<DataType>* sout)
{
  M2S_LOOP: for (int i = 0; i < tmux; ++i) {
    #pragma HLS pipeline II=1 rewind
    if (i < mem.getEntries(bx)) {
      auto dout = mem.read_mem(bx,i);
      sout->write_nb(dout.raw());
    } 
  }
}

template<regionType FMTYPE, int i>
void StreamOut(
    BXType bx,
    const FullMatchMemory<FMTYPE>* in,
    BXType& bx_o,
    FullMatch<FMTYPE>* out,
    bool& valid
){

  // Initialization

  // Reset output memories
  //out->clear(bx);

  // Initialize read addresses for candidate matches
  ap_uint<kNBits_MemAddr> addr1 = 0;  

  FullMatch<FMTYPE> bestmatch = FullMatch<FMTYPE>();
  bool goodmatch = false;

  // Processing starts
  LOOP: for (ap_uint<kNBits_MemAddr> istep = 0; istep < tmux; istep++)
  {

#pragma HLS PIPELINE II=1 rewind

    // Pick up number of candidate matches for each CM memory
    ap_uint<kNBits_MemAddr> nin = in->getEntries(bx);

    // Read in
    const FullMatch<FMTYPE> &in1 = in->read_mem(bx,addr1);

    // Valid signal for the candidate match
    bool valid1 = (addr1 < nin) && (nin > 0);

    auto tid = in1.getTrackletIndex();
    auto sid = in1.getStubIndex();
    auto phi = in1.getPhiRes();
    auto z   = in1.getZRes();

    // Output  
    FullMatch<FMTYPE> bestmatch_next(i,tid,sid,phi,z); 
    bool goodmatch_next = valid1;

    // Write out
    //out->write_mem(bx,bestmatch,goodmatch);
    *out    = bestmatch;
    valid  = goodmatch;
 
    // Pipeline
    bestmatch = bestmatch_next;
    goodmatch = goodmatch_next;

    // Increment the read addresses for the candidate matches
    addr1++;

  }// end LOOP 

}// end StreamOut

template<int i>
void GenArray(
  BXType bx,
  const ap_uint<45> in[10][20],
  BXType& bx_o,
  ap_uint<45> out[10][20]
){

  int addr1 = 0;
  ap_uint<45> bestout = 0;
  bool goodout = false;
  
  // Process
  LOOP: for (int istep = 0; istep < tmux; istep++){

#pragma HLS pipeline II=1 rewind

    const ap_uint<45> input = in[bx][addr1];
    bool valid = (addr1 < tmux); 

    // Output  
    ap_uint<45> bestout_next = input; 
    bool goodout_next = valid;

    // Write out
    out[bx][istep] = bestout; 
 
    // Pipeline
    bestout = bestout_next;
    goodout = goodout_next;

    // Increment the read addresses for the candidate matches
    addr1++;


  }// end LOOP 

}// end GenArray

template<regionType FMTYPE, int i>
void Generic(
    BXType bx,
    const FullMatchMemory<FMTYPE>* in,
    BXType& bx_o,
    FullMatchMemory<FMTYPE>* out,
    FullMatchMemory<FMTYPE>* clone
){

  // Initialization

  // Reset output memories
  //out->clear(bx);

  // Initialize read addresses for candidate matches
  ap_uint<kNBits_MemAddr> addr1 = 0;  

  FullMatch<FMTYPE> bestmatch = FullMatch<FMTYPE>();
  bool goodmatch = false;

  // Processing starts
  LOOP: for (ap_uint<kNBits_MemAddr> istep = 0; istep < tmux; istep++)
  {

#pragma HLS PIPELINE II=1 rewind

    // Pick up number of candidate matches for each CM memory
    ap_uint<kNBits_MemAddr> nin = in->getEntries(bx);

    // Read in
    const FullMatch<FMTYPE> &in1 = in->read_mem(bx,addr1);

    // Valid signal for the candidate match
    bool valid1 = (addr1 < nin) && (nin > 0);

    auto tid = in1.getTrackletIndex();
    auto sid = in1.getStubIndex();
    auto phi = in1.getPhiRes();
    auto z   = in1.getZRes();

    // Output  
    FullMatch<FMTYPE> bestmatch_next(i,tid,sid,phi,z); 
    bool goodmatch_next = valid1;

    // Write out
    out->write_mem(bx,bestmatch,goodmatch);
    clone->write_mem(bx,bestmatch,goodmatch);
 
    // Pipeline
    bestmatch = bestmatch_next;
    goodmatch = goodmatch_next;

    // Increment the read addresses for the candidate matches
    addr1++;

  }// end LOOP 

}// end Generic 

#endif
 
