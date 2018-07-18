#ifndef SSSS_H
#define SSSS_H

#include <gf256.h>


#define SSSS_EEPROM_MODE_IDX    0
#define SSSS_EEPROM_PAYLOAD_IDX 1
#define SSSS_EEPROM_FAMILY_IDX  2
#define SSSS_EEPROM_SHARE_IDX   3

#define SSSS_EEPROM_DATA_START  8

// NONE mode is when we have not started on a particular
// operation yet. From NONE we can go to DEAL mode by
// entering in the detals of the share configuration and
// by entering in the secret to be shared.
// From NONE mode, we can also enter COLLECT mode by entering
// the first of a set of shares.
#define SSSS_MODE_NONE    0

// DEAL mode is where we have a secret stored in EEPROM
// and the inderface will pass out successive shares.
// When done dealing shares, the device gets wiped back to 
// NONE mode
#define SSSS_MODE_DEAL    1

// In COLLECT mode we can see details of the share family,
// required shares to recover, and the number of shares 
// collected so far. We can also enter additional shares
#define SSSS_MODE_COLLECT 2

// When we have enough shares to reover the secret we go into
// recover mode, which allows us to view the secret. Once the
// secret has been revealed, the device can be wiped back to
// NONE mode.
#define SSSS_MODE_RECOVER 3


enum SsssMode: uint8_t {
  NoMode=0, Shuffle, Deal, Collect, Reveal
};

// Shares add two bytes to the beginning of the payload
// representing the family id, the number of shares needed to rebuild,
// and the share id.

// All shares in the share set have a common familyId byte.
// All shares in the share set have a unique id
// No share should ever be created with the id 0

struct FamilyId {
  unsigned int family : 5;  // all shares for a given secret will be of the same family
  unsigned int req    : 3;  // number of shares needed to build the secret
};

// EEPROM layout
// [0] mode  
// [1] payloadBytes 
// [2] familyId
// [3] current shareId (deal) / next share slot (collect)

// deal mode
// [4 --> (payloadBytes) +3 ] secret 
// [4+payloadBytes -> 4+n*payloadBytes +3] entropy

// collect mode
// [4] oth shareid 
// [5 ---> payloadBytes -+ 4 ] share payload

#define SSSS_MAX_SHARES 16

class Ssss {
  private:
  void setMode(uint8_t mode);  
  void setShares(uint8_t collected);  
  void saveShare(uint8_t shareIndex, uint8_t *share);
  uint8_t getFamilyId() const;
  
  uint8_t ent_index;
  uint8_t ent_sweeps;
  uint8_t family;
  uint8_t threshold;
  uint8_t payload;
  uint8_t shares;

  gf256 powers[16];
  gf256 lagrange_value[16];

  public:
  uint8_t mode;
  
  void LoadState();
  uint8_t getShares() const;

  // General Mode function
  uint8_t getMode() const;  // get the mode
  void clear(); // wipe the device and return to NONE mode
  void setPayload(uint8_t payload);  
  void setFamily(uint8_t fam);  
  void setThreshold(uint8_t thresh);  

  // common status functions
  uint8_t getFamily() const {return family;}
  uint8_t getThreshold() const {return threshold;}
  uint8_t getPayloadBytes() const {return payload;}
  uint8_t getPayloadWords() const {return (3*payload+2)/4;}

  bool hasFamily() const;

  // Deal Setup
  void beginShuffle();
  void setPayloadLength(uint8_t payloadLength);
  bool addEntropy(uint8_t entropy);
  void addSecret(uint8_t *secret);

  // Deal mode
  void beginDeal();
  void dealNextShare(int8_t *buffer); // writes share bytes to buffer - does not include checksum
  uint8_t getCurrentShare() const {return shares;}

  // Collect Mode Setup
  void beginCollect();
  uint8_t collectedSharesCount() const;
  void setSharePayloadLength(uint16_t payloadLength);  

  void addShare(uint8_t *share);  // this payload includes the family and share id...
  bool hasShare(uint8_t *share) const; // check if the share we are about to add is already in the set
  bool checkFamily(uint8_t *share) const; // check if the share id we are about to add is the right family

  // Recover Mode
  bool secretAvailable();
  void beginReveal();
  void getSecret(uint8_t *buffer); // copy secret into buffer
};

#endif
