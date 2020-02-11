import requests
import unittest
import random
import json

class TestRemoteBackendSlave(unittest.TestCase):
    url = "http://127.0.0.1:9100/dns/"
    zoneName = 'slavedomain.example.'

    # yes, this is the weird format the remote backend uses
    soaRecordPatch = b'rr[qname]=slavedomain.example.&rr[qtype]=SOA&rr[content]=ns1.example.net. hostmaster.example.net. 42 3600 3601 3602 3603&rr[ttl]=1200'

    def test_000_getUnfreshSlaveInfos(self):
        data = requests.get(self.url + 'getUnfreshSlaveInfos').json()
        self.assertEqual(len(data["result"]), 1)
        zone = data['result'][0]
        self.assertEqual(zone['zone'], self.zoneName)
        self.assertEqual(zone['kind'], 'slave')
        self.assertListEqual(zone['masters'], ['127.0.0.1:53'])

    def test_001_setFresh(self):
        data = requests.get(self.url + 'getUnfreshSlaveInfos').json()
        zoneId = data['result'][0]['id']
        data = requests.patch('{}setFresh/{}'.format(self.url, zoneId)).json()
        self.assertIs(data['result'], True)
        data = requests.get(self.url + 'getUnfreshSlaveInfos').json()
        self.assertEqual(len(data["result"]), 1) # as there is not soa with a refresh, it technically ain't fresh :)

    def test_002_transaction_does_not_exist(self):
        txId = random.randint(1, 10000)
        data = requests.get(self.url + 'getUnfreshSlaveInfos').json()
        zoneId = data['result'][0]['id']
        data = requests.post('{}startTransaction/{}/{}/{}'.format(self.url, zoneId, self.zoneName, txId)).json()
        self.assertIs(data['result'], True)

        data = requests.patch('{}feedRecord/{}'.format(self.url, txId + 1), data={}).json()
        self.assertIs(data['result'], False)

    # as this actively modifies things, we should test it last
    def test_900_transaction_SOA_only(self):
        txId = random.randint(1, 10000)
        data = requests.get(self.url + 'getUnfreshSlaveInfos').json()
        zoneId = data['result'][0]['id']
        data = requests.post('{}startTransaction/{}/{}/{}'.format(self.url, zoneId, self.zoneName, txId)).json()
        self.assertIs(data['result'], True)
        data = requests.patch('{}feedRecord/{}'.format(self.url, txId), data=self.soaRecordPatch).json()
        self.assertIs(data['result'], True)
        data = requests.post('{}commitTransaction/{}'.format(self.url, txId)).json()
        self.assertIs(data['result'], True)

        data = requests.patch('{}setFresh/{}'.format(self.url, zoneId)).json()
        self.assertIs(data['result'], True)
        data = requests.get(self.url + 'getUnfreshSlaveInfos').json()
        self.assertEqual(len(data["result"]), 0)

        data = requests.get(self.url + 'lookup/slavedomain.example./ANY').json()
        # 1 SOA answer
        self.assertEqual(len(data['result']), 1)

        data = requests.get(self.url + 'lookup/slavedomain.example./SOA').json()
        self.assertEqual(len(data['result']), 1)