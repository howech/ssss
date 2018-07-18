# ssss

This Arduino library implements a 
[Shamir Secret Sharing Scheme](https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing)
using GF256 as the
underlying field. (an implementation of GF256 for aruino is available 
[here](https://github.com/howech/gf256) ). As both processes of dealing out shares
and collecting them to recover the secret can be long, the library stores
various pieces of information in the arduino's EEPROM. (Note that this
library was originally written to be used with an arduino pro-mini with
an ATmega328, but will probably work on other hardware.)

## Modes

There are 4 different modes that the library can be in at any given time,
two for distributing shares, and two for using shares to recover the secret.

### Shuffle

To set up a secret share, select a family identifer, a threshold and a payload, and
then call beginShuffle(). The family identifier is just a number between 0 and 31
that will be used to help identify a share as being part of the same set of shares
as others. The threshold is the number of shares needed to recover the secret. In this
implementation, it is a number between 2 and 9.

In shuffle mode, once the payload size is set, the library will accept successive
bytes of entropy that it uses to fill out the higher order coefficients for the
sharing polynomial. When it has a sufficient quantity of entropy, the addEntropy()
method will return true. Also in shuffle mode, the library will accept a buffer 
to a payload sized array of bytes containing the secret to be shared. It will copy
the secret to EEPROM storage and erase the original location. Entropy bytes are
also stored in EEPROM.

The fact that the library is in shuffle mode will not be written to EEPROM. 

### Deal

When the library enters deal mode (via the beginDeal()) method, it recoreds the
fact in EEPROM. In deal mode the library will write (payload+2) bytes to a buffer
to represent a share. The share is 2 bytes longer than the original payload because
it containes some metadata about the sharing (family id, recovery threshold) as
well as the share index (used as the parameter to the share polynomial). Successive
calls to dealNextShare will produce additional shares. You can call this as many time
as you like to produce the number of shares that you need for your application.

In Deal mode, the current deal index, the secret and the share polynomial coefficients
are all stored in EEPROM, which means that the arduino will retain this information
even if power is removed. This allows the deal process to take place over an extended
period of time.

In deal mode, the getShares() method will return the number of shares dealt so far.

When you are done dealing shares, calling the clear() method will write over all
of the sensitve bytes with zeros, and return the device to the non-mode NoMode.

## Collect

In collect mode, the library will accept shares and store them in EEPROM. To enter
share mode, set the payload size and then call beginCollect(). Add more shares
using the addShare() method. The library will extract the family and threshold information
from the first share added. If you add two shares with the same share id,
the second one will overwrite the first one. You can check to see if a share has been
added already by calling the hasShare() method. The secretAvailable() method will
return true when the library has collected at least as many shares as indicacted 
by the threshold setting.

Shares and collection state is stored in EEPROM, so you can collect shares over an
extended period of time.

## Reveal

In reveal mode, you can call getSecret() to reveal the original secret from the shares
stored in EEPROM. Calling clear() will erase all of the sensitive information held by
this library and the EEPROM data, but will not erase the location where the secret was
deposited by getSecret(), so you will want to make sure to clean that up for yourself.


