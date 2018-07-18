#ifndef SSSS_H
#define SSSS_H

#include <gf256.h>

#ifndef SSSS_EEPROM_START
#define SSSS_EEPROM_START 0
#endif

#define SSSS_EEPROM_MODE_IDX (SSSS_EEPROM_START)
#define SSSS_EEPROM_PAYLOAD_IDX (SSSS_EEPROM_START + 1)
#define SSSS_EEPROM_FAMILY_IDX  (SSSS_EEPROM_START + 2)
#define SSSS_EEPROM_SHARE_IDX   (SSSS_EEPROM_START + 3)

#ifndef SSSS_EEPROM_DATA_START  
#define SSSS_EEPROM_DATA_START  (SSSS_EEPROM_START + 4)
#endif

// On an ATmega328 there is 1KB of EEPROM. This
// gives us room for about 31 32-byte shares.
#ifndef SSSS_MAX_SHARES
#define SSSS_MAX_SHARES 16
#endif


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

class Ssss {
  private:
  void setMode(uint8_t mode);  
  void setShares(uint8_t collected);  
  void saveShare(uint8_t shareIndex, uint8_t *share);

  // the family id packs the family and the
  // threshold into a single byte.
  uint8_t getFamilyId() const;
  
  // operational state variables
  uint8_t ent_index;
  uint8_t ent_sweeps;
  uint8_t family;
  uint8_t threshold;
  uint8_t payload;
  uint8_t shares;
  uint8_t mode;

  // working memory to encode/decode polynomial data
  gf256 powers[SSS_MAX_SHARES];
  gf256 lagrange_value[SSSS_MAX_SHARES];

  public:
  uint8_t getMode() const {return mode;}

  // Load the state of the sharing scheme
  // from data in EEPROM
  void LoadState();
  
  // Return either the number of shares
  // dealt or collected so far.
  uint8_t getShares() const {return shares;}


  void clear(); // wipe the device and return to NONE mode
  
  void setPayload(uint8_t payload);  
  void setFamily(uint8_t fam);  
  void setThreshold(uint8_t thresh);  

  // common status functions
  uint8_t getFamily() const {return family;}
  uint8_t getThreshold() const {return threshold;}
  uint8_t getPayloadBytes() const {return payload;}

  bool hasFamily() const;

  // Shuffle mode
  void beginShuffle();
  bool addEntropy(uint8_t entropy);
  void addSecret(uint8_t *secret);

  // Deal mode
  void beginDeal();
  void dealNextShare(int8_t *buffer); // writes share bytes to buffer - does not include checksum

  // Collect Mode
  void beginCollect();
  void setSharePayloadLength(uint16_t payloadLength) {setPayload( payloadLength - 2);}  

  bool hasShare(uint8_t *share) const; // check if the share we are about to add is already in the set
  bool checkFamily(uint8_t *share) const; // check if the share id we are about to add is the right family

  void addShare(uint8_t *share);  // this payload includes the family and share id...

  bool secretAvailable();

  // Reveal Mode
  void beginReveal();
  void getSecret(uint8_t *buffer); // copy secret into buffer
};

#endif
