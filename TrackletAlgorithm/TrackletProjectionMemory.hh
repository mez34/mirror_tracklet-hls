#ifndef TRACKLETPROJECTION_HH
#define TRACKLETPROJECTION_HH

#include "Constants.hh"
#include "MemoryTemplate.hh"

// Bit size for TrackletProjectionMemory fields
constexpr unsigned int kTProjZDSize = 10;
constexpr unsigned int kTProjPhiDSize = 11;
constexpr unsigned int kTProjZSize = 12;
constexpr unsigned int kTProjPhiSize = 14;
constexpr unsigned int kTProjTCIndexSize = 13;
// Bit size for full TrackletProjectionMemory
constexpr unsigned int kTrackletProjectionSize = 1 + 1 +kTProjTCIndexSize + kTProjPhiSize + kTProjZSize + kTProjPhiDSize + kTProjZDSize;

// The location of the least significant bit (LSB) and most significant bit (MSB) in the TrackletProjectionMemory word for different fields
constexpr unsigned int kTProjZDLSB = 0;
constexpr unsigned int kTProjZDMSB = kTProjZDLSB + kTProjZDSize - 1;
constexpr unsigned int kTProjPhiDLSB = kTProjZDMSB + 1;
constexpr unsigned int kTProjPhiDMSB = kTProjPhiDLSB + kTProjPhiDSize - 1;
constexpr unsigned int kTProjZLSB = kTProjPhiDMSB + 1;
constexpr unsigned int kTProjZMSB = kTProjZLSB + kTProjZSize - 1;
constexpr unsigned int kTProjPhiLSB = kTProjZMSB + 1;
constexpr unsigned int kTProjPhiMSB = kTProjPhiLSB + kTProjPhiSize - 1;
constexpr unsigned int kTProjTCIndexLSB = kTProjPhiMSB + 1;
constexpr unsigned int kTProjTCIndexMSB = kTProjTCIndexLSB + kTProjTCIndexSize - 1;
constexpr unsigned int kTProjIsMinusNeighborLSB = kTProjTCIndexMSB + 1;
constexpr unsigned int kTProjIsPlusNeighborLSB = kTProjIsMinusNeighborLSB + 1;

// Data object definition
class TrackletProjection
{
public:

  typedef ap_uint<kTProjTCIndexSize> TProjTCID;
  typedef ap_uint<kTProjPhiSize> TProjPHI;
  typedef ap_int<kTProjZSize> TProjZ;
  typedef ap_int<kTProjPhiDSize> TProjPHIDER;
  typedef ap_int<kTProjZDSize> TProjZDER;
  
  typedef ap_uint<kTrackletProjectionSize> TrackletProjectionData;

  // Constructors
  TrackletProjection(const TrackletProjectionData& newdata):
    data_(newdata)
  {}

  TrackletProjection(const bool plusneighbor, const bool minusneighbor, const TProjTCID tcid, const TProjPHI phi, const TProjZ z, const TProjPHIDER phider, const TProjZDER zder):
    data_( ((((((plusneighbor,minusneighbor),tcid),phi),z),phider),zder) )
  {}
  
  TrackletProjection():
    data_(0)
  {}

  #ifndef __SYNTHESIS__
  TrackletProjection(const char* datastr, int base=16)
  {
    TrackletProjectionData newdata(datastr, base);
    data_ = newdata;
  }
  #endif

  // Getter
  TrackletProjectionData raw() const {return data_;}

  bool getIsPlusNeighbor() const {
    return data_.range(kTProjIsPlusNeighborLSB,kTProjIsPlusNeighborLSB);
  }
  
  bool getIsMinusNeighbor() const {
    return data_.range(kTProjIsMinusNeighborLSB,kTProjIsMinusNeighborLSB);
  }
  
  TProjTCID getTrackletIndex() const {
    return data_.range(kTProjTCIndexMSB,kTProjTCIndexLSB);
  }

  TProjPHI getPhi() const {
    return data_.range(kTProjPhiMSB,kTProjPhiLSB);
  }

  TProjZ getZ() const {
    return data_.range(kTProjZMSB,kTProjZLSB);
  }

  TProjPHIDER getPhiDer() {
    return data_.range(kTProjPhiDMSB,kTProjPhiDLSB);
  }

  TProjZDER getZDer() {
    return data_.range(kTProjZDMSB,kTProjZDLSB);
  }

  // Setter
  void setIsPlusNeighbor(const bool isplusneighbor) {
    data_.range(kTProjIsPlusNeighborLSB,kTProjIsPlusNeighborLSB) = isplusneighbor;
  }

  void setIsMinusNeighbor(const bool isminusneighbor) {
    data_.range(kTProjIsMinusNeighborLSB,kTProjIsMinusNeighborLSB) = isminusneighbor;
  }

  void setTrackletIndex(const TProjTCID id) {
    data_.range(kTProjTCIndexMSB,kTProjTCIndexLSB) = id;
  }

  void setPhi(const TProjPHI phi) {
    data_.range(kTProjPhiMSB,kTProjPhiLSB) = phi;
  }

  void setZ(const TProjZ z) {
    data_.range(kTProjZMSB,kTProjZLSB) = z;
  }

  void setPhiDer(const TProjPHIDER phider) {
    data_.range(kTProjPhiDMSB,kTProjPhiDLSB) = phider;
  }

  void setZDer(const TProjZDER zder) {
    data_.range(kTProjZDMSB,kTProjZDLSB) = zder;
  }
  
private:
  
  TrackletProjectionData data_;

};

// Memory definition
typedef MemoryTemplate<TrackletProjection, 1, kNBits_MemAddr> TrackletProjectionMemory;
// FIXME: double check number of bits for bx and for memory address

#endif
