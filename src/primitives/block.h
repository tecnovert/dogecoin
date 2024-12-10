// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <primitives/pureheader.h>
#include <serialize.h>
#include <uint256.h>
//#include <auxpow.h>

class CBlock;
class CBlockHeader;
class CBlockIndex;

/* Because it is needed for auxpow, the definition of CMerkleTx is moved
   here from wallet.h.  */

class CMerkleTx
{
private:
  /** Constant used in hashBlock to indicate tx has been abandoned */
    static const uint256 ABANDON_HASH;

public:
    CTransactionRef tx;
    uint256 hashBlock;
    // Dogecoin TODO: Is this used? If not remove. If it is, I don't think it's actually set
    // anywhere. Check with Namecore
    std::vector<uint256> vMerkleBranch;

    /* An nIndex == -1 means that hashBlock (in nonzero) refers to the earliest
     * block in the chain we know this or any in-wallet dependency conflicts
     * with. Older clients interpret nIndex == -1 as unconfirmed for backward
     * compatibility.
     */
    int nIndex;

    CMerkleTx()
    {
        CMutableTransaction txNew;
        SetTx(MakeTransactionRef(std::move(txNew)));
        Init();
    }

    CMerkleTx(CTransactionRef arg)
    {
        SetTx(std::move(arg));
        Init();
    }

    /** Helper conversion operator to allow passing CMerkleTx where CTransaction is expected.
     *  TODO: adapt callers and remove this operator. */
    operator const CTransaction&() const { return *tx; }

    void Init()
    {
        hashBlock = uint256();
        nIndex = -1;
    }

    void SetTx(CTransactionRef arg)
    {
        tx = std::move(arg);
    }

    SERIALIZE_METHODS(CMerkleTx, obj)
    {
        READWRITE(obj.tx);
        READWRITE(obj.hashBlock);
        READWRITE(obj.vMerkleBranch);
        READWRITE(obj.nIndex);
    }

    /**
     * Actually compute the Merkle branch.  This is used for unit tests when
     * constructing an auxpow.  It is not needed for actual production, since
     * we do not care in the Namecoin client how the auxpow is constructed
     * by a miner.
     */
    void InitMerkleBranch(const CBlock& block, int posInBlock);

    void SetMerkleBranch(const CBlockIndex* pindex, int posInBlock);

    bool hashUnset() const { return (hashBlock.IsNull() || hashBlock == ABANDON_HASH); }
    bool isAbandoned() const { return (hashBlock == ABANDON_HASH); }
    void setAbandoned() { hashBlock = ABANDON_HASH; }

    const uint256& GetHash() const { return tx->GetHash(); }
    bool IsCoinBase() const { return tx->IsCoinBase(); }
};

class CAuxPow : public CMerkleTx
{

/* Public for the unit tests.  */
public:

  /** The merkle branch connecting the aux block to our coinbase.  */
  std::vector<uint256> vChainMerkleBranch;

  /** Merkle tree index of the aux block header in the coinbase.  */
  int nChainIndex;

  /** Parent block header (on which the real PoW is done).  */
  CPureBlockHeader parentBlock;

public:

  /* Prevent accidental conversion.  */
  inline explicit CAuxPow(CTransactionRef txIn)
    : CMerkleTx(txIn)
  {
  }

  inline CAuxPow()
    : CMerkleTx()
  {
  }

  SERIALIZE_METHODS(CAuxPow, obj)
  {
    READWRITEAS(CMerkleTx, obj);
    READWRITE(obj.vChainMerkleBranch);
    READWRITE(obj.nChainIndex);
    READWRITE(obj.parentBlock);
  }

  /**
   * Check the auxpow, given the merge-mined block's hash and our chain ID.
   * Note that this does not verify the actual PoW on the parent block!  It
   * just confirms that all the merkle branches are valid.
   * @param hashAuxBlock Hash of the merge-mined block.
   * @param nChainId The auxpow chain ID of the block to check.
   * @param params Consensus parameters.
   * @return True if the auxpow is valid.
   */
  //bool check(const uint256& hashAuxBlock, int nChainId, const Consensus::Params& params) const;

  /**
   * Get the parent block's hash.  This is used to verify that it
   * satisfies the PoW requirement.
   * @return The parent block hash.
   */
  inline uint256
  getParentBlockPoWHash() const
  {
    return parentBlock.GetPoWHash ();
  }

  /**
   * Return parent block.  This is only used for the temporary parentblock
   * auxpow version check.
   * @return The parent block.
   */
  /* FIXME: Remove after the hardfork.  */
  inline const CPureBlockHeader&
  getParentBlock() const
  {
    return parentBlock;
  }

  /**
   * Calculate the expected index in the merkle tree.  This is also used
   * for the test-suite.
   * @param nNonce The coinbase's nonce value.
   * @param nChainId The chain ID.
   * @param h The merkle block height.
   * @return The expected index for the aux hash.
   */
  static int getExpectedIndex(uint32_t nNonce, int nChainId, unsigned h);

  /**
   * Check a merkle branch.  This used to be in CBlock, but was removed
   * upstream.  Thus include it here now.
   */
  static uint256 CheckMerkleBranch(uint256 hash,
                                   const std::vector<uint256>& vMerkleBranch,
                                   int nIndex);
};


/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader : public CPureBlockHeader
{
public:
    // auxpow (if this is a merge-minded block)
    mutable std::shared_ptr<CAuxPow> auxpow;

    CBlockHeader()
    {
        SetNull();
    }

    SERIALIZE_METHODS(CBlockHeader, obj)
    {
        READWRITEAS(CPureBlockHeader, obj);

        if (obj.IsAuxpow()) {
            if (ser_action.ForRead()) {
                obj.auxpow.reset(new CAuxPow());
            }
            assert(obj.auxpow);
            READWRITE(*obj.auxpow);
        } else if (ser_action.ForRead()) {
            obj.auxpow.reset();
        }
    }

    void SetNull()
    {
        CPureBlockHeader::SetNull();
        auxpow.reset();
    }

    /**
     * Set the block's auxpow (or unset it).  This takes care of updating
     * the version accordingly.
     * @param apow Pointer to the auxpow to use or NULL.
     */
    void SetAuxpow(CAuxPow* apow);
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }

    SERIALIZE_METHODS(CBlock, obj)
    {
        READWRITEAS(CBlockHeader, obj);
        READWRITE(obj.vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        block.auxpow         = auxpow;
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    SERIALIZE_METHODS(CBlockLocator, obj)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(obj.vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
