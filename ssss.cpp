#include "ssss.h"
#include <Arduino.h>
#include <EEPROM.h>


/////////////////////////////////////////////////////////////////////////////////////////////////
// Lagrange Polynomial
/////////////////////////////////////////////////////////////////////////////////////////////////
gf256
lagrange(int size, gf256 x[], int i, gf256 y) {
  // evaluates the ith lagrange polynomial at y
  // lagrange polynomial
  // going through (x[i],1) and through each of the
  // (x[j!=i],0) points.
  
  gf256 result = 1;

  for(int j=0;j<size;++j) {
    if(j != i) {
      result = result * (x[j] - y) / (x[i] - x[j]);
    }
  }

  return result;
}

void 
Ssss::clear() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  for(uint8_t i=0; i< SSSS_MAX_SHARES; i++ ) {
    lagrange_value[i] = 0;
    powers[i] = 0;
  }
  LoadState();
}

void Ssss::LoadState() {
  mode = EEPROM.read(SSSS_EEPROM_MODE_IDX);
  uint8_t f = EEPROM.read(SSSS_EEPROM_FAMILY_IDX);
  family = f >> 3;
  threshold = (f & 0x7) + 2;
  shares = EEPROM.read(SSSS_EEPROM_SHARE_IDX);
  payload = EEPROM.read(SSSS_EEPROM_PAYLOAD_IDX);
}

bool
Ssss::hasFamily() const {
  return shares > 0 || mode == Deal || mode == Reveal || mode == Shuffle;
}

uint8_t
Ssss::getMode() const {
  return mode;
}

uint8_t
Ssss::getFamilyId() const {
  return family << 3 | ((threshold - 2) & 0x7);
}

void 
Ssss::setMode(uint8_t m) {
  mode = m;
  if(m != Shuffle) {
    EEPROM.write(SSSS_EEPROM_MODE_IDX, mode);
  }
}

void 
Ssss::setFamily(uint8_t fam) {
  family = fam;
  EEPROM.write( SSSS_EEPROM_FAMILY_IDX, getFamilyId());
}

void 
Ssss::setThreshold(uint8_t thresh) {
  threshold = thresh;
  EEPROM.put( SSSS_EEPROM_FAMILY_IDX, getFamilyId());
  // change in threshold means that we need to reset our
  // entropy counters.
  ent_sweeps = 0;
  ent_index = SSSS_EEPROM_DATA_START + payload;
}

void 
Ssss::setPayload(uint8_t payl) {
  payload = payl;
  EEPROM.write( SSSS_EEPROM_PAYLOAD_IDX, payload);
  // change in payload means that we need to reset our
  // entropy counters
  ent_sweeps = 0;
  ent_index = SSSS_EEPROM_DATA_START + payload;
}

  // Deal Setup
void 
Ssss::beginShuffle() {
  mode = Shuffle;
  ent_index = SSSS_EEPROM_DATA_START + payload;
  ent_sweeps = 0;
}

bool 
Ssss::addEntropy(uint8_t entropy) {
  // We only need to collect entropy when we are in shuffle mode.
  if(mode == Shuffle) {
    // EEPROM has a limited lifetime, so stop writing to it if we have
    // more than sufficient entropy
    if(ent_sweeps < 8) {
      // xor the current byte with what is already there
      EEPROM.write(ent_index, entropy ^ EEPROM.read(ent_index));
      ent_index += 1;

      if(ent_index > SSSS_EEPROM_DATA_START + threshold * payload) {
        ent_index = SSSS_EEPROM_DATA_START + payload;
        ent_sweeps += 1;
      }

      // At a minimim, we need to have 
      return ent_sweeps >= 2;
    }

    return false;
  }

  return false;
}

void 
Ssss::addSecret(uint8_t *secret) {
  uint16_t index = SSSS_EEPROM_DATA_START;
  for(uint8_t i = 0; i<payload; ++i) {
    EEPROM.write(index++, secret[i]);
    secret[i] = 0;  // dont leave extra copies of the secret lying around
  }

  for(uint16_t i=0,k=SSSS_EEPROM_DATA_START; i<threshold*(payload+1);i++,k++ ) {
  }
}


void 
Ssss::beginDeal() {
  mode = Deal;

  EEPROM.write(SSSS_EEPROM_MODE_IDX, mode);
  EEPROM.write(SSSS_EEPROM_FAMILY_IDX, getFamilyId());
  EEPROM.write(SSSS_EEPROM_PAYLOAD_IDX, payload);
  shares = 0;
  EEPROM.write(SSSS_EEPROM_SHARE_IDX, shares);
}

void 
Ssss::dealNextShare(int8_t *buffer) { // writes share bytes to buffer - does not include checksum
  shares++;

  uint8_t i = 0;

  buffer[i++] = getFamilyId();
  buffer[i++] = shares;
  gf256 gid = shares;

  for(uint8_t j = 0; j<threshold; j++ ) {
    powers[j] = gid.power(j);
  }

  for(uint8_t k =0; k<payload; k++) {
    gf256 sum = 0;
    uint16_t l = SSSS_EEPROM_DATA_START + k;
    for(uint8_t j = 0; j<threshold; j++, l+=payload) {
      sum = sum + powers[j] * EEPROM.read(l);
    }
    buffer[i++] = sum;
  }

  EEPROM.write( SSSS_EEPROM_SHARE_IDX, shares);
}

void 
Ssss::beginCollect() {
  mode = Collect;
  EEPROM.write(SSSS_EEPROM_MODE_IDX, mode);
  shares = 0;
  EEPROM.write(SSSS_EEPROM_SHARE_IDX, shares);
}


bool
Ssss::hasShare(uint8_t *share) const {
  uint16_t p=SSSS_EEPROM_DATA_START;
  for(uint8_t i=0; i<shares; i++, p+=payload+1) {
    if(share[1] == EEPROM.read(p)) {
      return true;
    }
  }
  return false;
}

bool
Ssss::checkFamily(uint8_t *share) const {
  return shares==0 || share[0] == getFamilyId();
}

void
Ssss::addShare(uint8_t *share) {
  uint8_t idx = shares;
  
  if(shares == 0) {
    setFamily( share[0] >> 3);
    setThreshold( (share[0] & 0x7) + 2);
  }
  

  // check for overwriting an existing share
  uint16_t k = SSSS_EEPROM_DATA_START;
  for(uint8_t j=0; j<shares; j++, k += payload + 1) {
    if(EEPROM.read(k) == share[1]) {
      idx = j;
      break;
    }
  }

  uint16_t p = payload + 1;
  uint16_t j = SSSS_EEPROM_DATA_START + idx * p;
  for(uint16_t k = 0; k<p; ++k ) {
    EEPROM.write(j++, share[k+1]);
  }

  if(idx == shares) {
    shares++;
    EEPROM.write( SSSS_EEPROM_SHARE_IDX, shares);
  }
}
  
bool 
Ssss::secretAvailable() {
  return shares >= threshold;
}

void
Ssss::beginReveal() {
  mode = Reveal;
}

void 
Ssss::getSecret(uint8_t *buffer) {
  uint8_t p = payload + 1;
  uint16_t k = SSSS_EEPROM_DATA_START;
  for(uint8_t i=0; i<shares; ++i, k+=p) {
    powers[i] = EEPROM.read(k);
  }
  
  for(uint8_t i=0; i<shares; ++i) {
    lagrange_value[i] = lagrange(shares, powers, i, 0);
  }

  for(uint8_t i =0; i<payload; i++) {
    gf256 sum = 0;
    k=SSSS_EEPROM_DATA_START+i;
    for(uint8_t j = 0; j<shares; ++j, k+=p) {
      sum = sum + lagrange_value[j] * EEPROM.read( k );
    }
    buffer[i] = sum;
  } 
}

