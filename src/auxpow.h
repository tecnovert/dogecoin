// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2016 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AUXPOW_H
#define BITCOIN_AUXPOW_H

#include <consensus/params.h>
#include <consensus/validation.h>
#include <primitives/pureheader.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

#include <vector>

class CBlock;
class CBlockHeader;
class CBlockIndex;

/** Header for merge-mining data in the coinbase.  */
static const unsigned char pchMergedMiningHeader[] = { 0xfa, 0xbe, 'm', 'm' };

/**
* Initialise the auxpow of the given block header.  This constructs
* a minimal CAuxPow object with a minimal parent block and sets
* it on the block header.  The auxpow is not necessarily valid, but
* can be "mined" to make it valid.
* @param header The header to set the auxpow on.
*/
void initAuxPow(CBlockHeader& header);

/**
* Check the auxpow, given the merge-mined block's hash and our chain ID.
* Note that this does not verify the actual PoW on the parent block!  It
* just confirms that all the merkle branches are valid.
* @param hashAuxBlock Hash of the merge-mined block.
* @param nChainId The auxpow chain ID of the block to check.
* @param params Consensus parameters.
* @return True if the auxpow is valid.
*/
bool checkAuxPow(CAuxPow &auxpow, const uint256& hashAuxBlock, int nChainId, const Consensus::Params& params);


#endif // BITCOIN_AUXPOW_H
