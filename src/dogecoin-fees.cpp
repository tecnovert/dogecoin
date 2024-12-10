// Copyright (c) 2021 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/random/uniform_int.hpp>


#include <policy/policy.h>
#include <arith_uint256.h>
#include <dogecoin.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <validation.h>
#include <dogecoin-fees.h>
#include <consensus/amount.h>
#include <node/ui_interface.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

CFeeRate min_fee{wallet::DEFAULT_TRANSACTION_MINFEE};

bool DogecoinParameterInteraction()
{
    if (gArgs.IsArgSet("-mintxfee")) {
        std::optional<CAmount> min_tx_fee = ParseMoney(gArgs.GetArg("-mintxfee", ""));
        if (!min_tx_fee || min_tx_fee.value() == 0) {
            return InitError(AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", "")));
        } else if (min_tx_fee.value() > wallet::HIGH_TX_FEE_PER_KB) {
            InitWarning(AmountHighWarn("-mintxfee") + Untranslated(" ") +
                        _("This is the minimum transaction fee you pay on every transaction."));
        }

        min_fee = CFeeRate{min_tx_fee.value()};
    }

    return true;
}

#ifdef ENABLE_WALLET

CFeeRate GetDogecoinFeeRate(int priority)
{
    switch(priority)
    {
    case SUCH_EXPENSIVE:
        return CFeeRate(COIN / 100 * 521); // 5.21 DOGE, but very carefully avoiding floating point maths
    case MANY_GENEROUS:
        return CFeeRate(min_fee.GetFeePerK() * 100);
    case AMAZE:
        return CFeeRate(min_fee.GetFeePerK() * 10);
    case WOW:
        return CFeeRate(min_fee.GetFeePerK() * 5);
    case MORE:
        return CFeeRate(min_fee.GetFeePerK() * 2);
    case MINIMUM:
    default:
        break;
    }
    return min_fee;
}

const std::string GetDogecoinPriorityLabel(int priority)
{
    switch(priority)
    {
    case SUCH_EXPENSIVE:
        return _("Such expensive").translated;
    case MANY_GENEROUS:
        return _("Many generous").translated;
    case AMAZE:
        return _("Amaze").translated;
    case WOW:
        return _("Wow").translated;
    case MORE:
        return _("More").translated;
    case MINIMUM:
        return _("Minimum").translated;
    default:
        break;
    }
    return _("Default").translated;
}

#endif

CAmount GetDogecoinMinRelayFee(const CTransaction& tx, unsigned int nBytes, bool fAllowFree, CTxMemPool *mempool)
{
    {
        assert(mempool);
        //LOCK(mempool.cs);
        uint256 hash = tx.GetHash();
        CAmount nFeeDelta = 0;
        mempool->ApplyDelta(hash, nFeeDelta);
        if (nFeeDelta > 0)
            return 0;
    }

    CAmount nMinFee = ::minRelayTxFee.GetFee(nBytes);
    nMinFee += GetDogecoinDustFee(tx.vout, nDustLimit);

    if (fAllowFree)
    {
        // There is a free transaction area in blocks created by most miners,
        // * If we are relaying we allow transactions up to DEFAULT_BLOCK_PRIORITY_SIZE - 1000
        //   to be considered to fall into this category. We don't want to encourage sending
        //   multiple transactions instead of one big transaction to avoid fees.
        if (nBytes < (DEFAULT_BLOCK_PRIORITY_SIZE - 1000))
            nMinFee = 0;
    }

    if (!MoneyRange(nMinFee))
        nMinFee = MAX_MONEY;
    return nMinFee;
}

CAmount GetDogecoinDustFee(const std::vector<CTxOut> &vout, const CAmount dustLimit) {
    CAmount nFee = 0;

    // To limit dust spam, add the dust limit for each output
    // less than the (soft) dustlimit
    for (const CTxOut& txout : vout)
        if (txout.IsDust(dustLimit))
            nFee += dustLimit;

    return nFee;
}
