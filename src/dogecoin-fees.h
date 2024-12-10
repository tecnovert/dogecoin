// Copyright (c) 2021 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DOGECOIN_FEES_H
#define BITCOIN_DOGECOIN_FEES_H

#include <chain.h>
#include <chainparams.h>
#include <policy/feerate.h>
#include <sync.h>
#include <txmempool.h>

#ifdef ENABLE_WALLET

struct bilingual_str;

enum FeeRatePreset
{
    MINIMUM,
    MORE,
    WOW,
    AMAZE,
    MANY_GENEROUS,
    SUCH_EXPENSIVE
};

/** Estimate fee rate needed to get into the next nBlocks */
CFeeRate GetDogecoinFeeRate(int priority);
const std::string GetDogecoinPriorityLabel(int priority);
#endif // ENABLE_WALLET
CAmount GetDogecoinMinRelayFee(const CTransaction& tx, unsigned int nBytes, bool fAllowFree, CTxMemPool *mempool) EXCLUSIVE_LOCKS_REQUIRED(mempool->cs);
CAmount GetDogecoinDustFee(const std::vector<CTxOut> &vout, const CAmount dustLimit);

bool DogecoinParameterInteraction();

#endif // BITCOIN_DOGECOIN_FEES_H
