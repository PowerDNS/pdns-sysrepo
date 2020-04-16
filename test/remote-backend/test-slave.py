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

    def test_901_transaction_several_records(self):
        txId = random.randint(1, 10000)
        data = requests.get(self.url + 'getDomainInfo/slavedomain.example.').json()
        zoneId = data['result']['id']
        data = requests.post('{}startTransaction/{}/{}/{}'.format(self.url, zoneId, self.zoneName, txId)).json()
        self.assertIs(data['result'], True)
        patches = [
          b'rr[qname]=slavedomain.example.&rr[qtype]=SOA&rr[content]=ns1.example.net. hostmaster.example.net. 43 3600 3601 3602 3603&rr[ttl]=1200',
          b'rr[qname]=_xmpp-server._tcp.slavedomain.example.&rr[qtype]=SRV&rr[content]=0 0 5222 sipserver.example.com.&rr[ttl]=1800',
          b'rr[qname]=slavedomain.example.&rr[qtype]=A&rr[content]=192.0.2.42&rr[ttl]=3600',
          b'rr[qname]=slavedomain.example.&rr[qtype]=MX&rr[content]=10 mx1.example.net.&rr[ttl]=3600',
          b'rr[qname]=slavedomain.example.&rr[qtype]=AAAA&rr[content]=2001:DB8::42:42&rr[ttl]=3600',
        ]
        for p in patches:
            data = requests.patch('{}feedRecord/{}'.format(self.url, txId), data=p).json()
            self.assertIs(data['result'], True)

        data = requests.post('{}commitTransaction/{}'.format(self.url, txId)).json()
        self.assertIs(data['result'], True)
        data = requests.patch('{}setFresh/{}'.format(self.url, zoneId)).json()
        self.assertIs(data['result'], True)

        data = requests.get(self.url + 'lookup/slavedomain.example./ANY').json()
        # 1 SOA, 1 MX, 1 A, 1 AAAA
        self.assertEqual(len(data['result']), 4)

        data = requests.get(self.url + 'lookup/_xmpp-server._tcp.slavedomain.example./ANY').json()
        # 1 SRV
        self.assertEqual(len(data['result']), 1)