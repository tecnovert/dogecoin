#!/usr/bin/env python3
# Copyright (c) 2021 The Dogecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""PayTxFee QA test.

# Tests wallet behavior of -paytxfee in relation to -mintxfee
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.messages import (
    tx_from_hex,
)
from decimal import Decimal

class PayTxFeeTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.is_network_split = False

        self.extra_args = [
            # node 0 has txindex to track txs
            ["-debug", "-txindex"],
            # node 1 pays 0.1 DOGE on all txs due to implicit mintxfee = paytxfee
            ["-debug", "-paytxfee=0.1"],
            # node 2 will always pay 1 DOGE on all txs because of explicit mintxfee
            ["-debug", "-paytxfee=0.1", "-mintxfee=1"],
            # node 3 will always pay 0.1 DOGE on all txs despite explicit mintxfee of 0.01
            ["-debug", "-paytxfee=0.1", "-mintxfee=0.01"],
        ]

    def setup_network(self):
        super().setup_network()
        for s in range(0, self.num_nodes):
            for t in range(s+1, self.num_nodes):
                self.connect_nodes(s, t)
                self.connect_nodes(t, s)
        self.sync_all()

    def run_test(self):

        node_addrs = []
        for i in range(self.num_nodes):
            self.nodes[i].createwallet(wallet_name="test")
            node_addrs.append(self.nodes[i].getnewaddress())

        seed = 1000 # the amount to seed wallets with
        amount = 995 # the amount to send back
        targetAddress = self.nodes[0].getnewaddress()

        # mine some blocks and prepare some coins
        self.generatetoaddress(self.nodes[0], 102, node_addrs[0])
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), seed)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), seed)
        self.nodes[0].sendtoaddress(self.nodes[3].getnewaddress(), seed)
        self.generatetoaddress(self.nodes[0], 1, node_addrs[0])

        # create transactions
        txid1 = self.nodes[1].sendtoaddress(targetAddress, amount)
        txid2 = self.nodes[2].sendtoaddress(targetAddress, amount)
        txid3 = self.nodes[3].sendtoaddress(targetAddress, amount)
        self.sync_all()

        # make sure correct fees were paid
        tx1 = self.nodes[0].getrawtransaction(txid1, True)
        tx2 = self.nodes[0].getrawtransaction(txid2, True)
        tx3 = self.nodes[0].getrawtransaction(txid3, True)

        tx = tx_from_hex(tx1["hex"])
        prevout = tx1["vin"][0]
        tx_prev = tx_from_hex(self.nodes[0].getrawtransaction(prevout["txid"], True)["hex"])
        value_out = tx.vout[0].nValue + tx.vout[1].nValue
        value_in = tx_prev.vout[prevout["vout"]].nValue
        fee_paid = value_in - value_out
        fee_rate = fee_paid * 1000 // tx1["size"]
        assert (fee_rate == 10000000)  # 0.1

        tx = tx_from_hex(tx2["hex"])
        prevout = tx2["vin"][0]
        tx_prev = tx_from_hex(self.nodes[0].getrawtransaction(prevout["txid"], True)["hex"])
        value_out = tx.vout[0].nValue + tx.vout[1].nValue
        value_in = tx_prev.vout[prevout["vout"]].nValue
        fee_paid = value_in - value_out
        fee_rate = fee_paid * 1000 // tx2["size"]
        assert (fee_rate == 100000000)  # 1

        tx = tx_from_hex(tx3["hex"])
        prevout = tx3["vin"][0]
        tx_prev = tx_from_hex(self.nodes[0].getrawtransaction(prevout["txid"], True)["hex"])
        value_out = tx.vout[0].nValue + tx.vout[1].nValue
        value_in = tx_prev.vout[prevout["vout"]].nValue
        fee_paid = value_in - value_out
        fee_rate = fee_paid * 1000 // tx3["size"]
        assert (fee_rate == 10000000)  # 0.1

        # Value is off by one from 1.14.9, but feerate matches
        assert_equal(tx1['vout'][0]['value'] + tx1['vout'][1]['value'], Decimal("999.9775"))
        assert_equal(tx2['vout'][0]['value'] + tx2['vout'][1]['value'], Decimal("999.775"))
        assert_equal(tx3['vout'][0]['value'] + tx3['vout'][1]['value'], Decimal("999.9775"))

        # mine a block
        self.generatetoaddress(self.nodes[0], 1, node_addrs[0])

        # make sure all fees were mined
        block = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        coinbaseTx = self.nodes[0].getrawtransaction(block['tx'][0], True)

        assert_equal(coinbaseTx['vout'][0]['value'], Decimal("500000.27"))

        # Stop node2 early with expected_stderr
        self.nodes[2].stop_node(expected_stderr="Warning: -mintxfee is set very high! This is the minimum transaction fee you pay on every transaction.")


if __name__ == '__main__':
    PayTxFeeTest().main()
