import requests
import unittest

class TestRemoteBackend(unittest.TestCase):
    url = "http://127.0.0.1:9100/dns/"

    def test_lookup_example_com_SOA(self):
        data = requests.get(self.url + 'lookup/example.com./SOA').json()
        record = data['result'][0]
        self.assertEqual(record['content'], 'ns1.example.nl. hostmaster.example.nl. 2020011501 10800 3600 604800 3600')
        self.assertEqual(record['qname'], 'example.com.')
        self.assertEqual(record['qtype'], 'SOA')
        self.assertEqual(record['ttl'], 3600)

    def test_lookup_doesntexist_example_com_A(self):
        data = requests.get(self.url + 'lookup/doesntexist.example.com./A').json() # This is an NXD
        self.assertEqual(len(data['result']), 0)

    def test_lookup_example_com_A(self):
        data = requests.get(self.url + 'lookup/example.com./A').json()
        self.assertEqual(len(data['result']), 2)
        seen_dot_1 = False
        seen_dot_3 = False
        for record in data['result']:
            self.assertEqual(record['qname'], 'example.com.')
            self.assertEqual(record['qtype'], 'A')
            self.assertEqual(record['ttl'], 3600)
            if record['content'] == '192.0.2.1':
                seen_dot_1 = True
            if record['content'] == '192.0.2.3':
                seen_dot_3 = True

        self.assertTrue(seen_dot_1)
        self.assertTrue(seen_dot_3)

    def test_lookup_example_com_MX(self):
        data = requests.get(self.url + 'lookup/example.com./MX').json()
        print(data)
        self.assertEqual(len(data['result']), 2)
        rdatas = set()
        for record in data['result']:
            self.assertEqual(record['qname'], 'example.com.')
            self.assertEqual(record['qtype'], 'MX')
            self.assertEqual(record['ttl'], 3000)
            rdatas.add(record['content'])
        self.assertSetEqual(rdatas, {'10 mx1.example.net.', '20 mx2.example.net.'})

    def test_lookup_www_example_com_ANY(self):
        # This is what PowerDNS actually does :)
        data = requests.get(self.url + 'lookup/www.example.com./ANY').json()
        # 1 AAAA answer
        self.assertEqual(len(data['result']), 1)

    def test_lookup_wildcard(self):
        data = requests.get(self.url + 'lookup/*.wildcard.testdomain.example./AAAA').json()
        assert(len(data['result']) == 1)
        assert(data["result"][0]["qtype"] == "AAAA")

    def test_lookup_domain_does_not_exist(self):
        data = requests.get(self.url + 'lookup/nosuchdomain.example./A').json()
        self.assertEqual(len(data['result']), 0)
        self.assertEqual(len(data['log']), 1)

    def test_list_example_com(self):
        data = requests.get(self.url + 'list/-1/example.com.').json()
        self.assertEqual(len(data['result']), 8)

    def test_list_nonexist(self):
        data = requests.get(self.url + 'list/-1/doesnexist.example.').json()
        self.assertEqual(len(data['result']), 0)
        self.assertEqual(len(data['log']), 1)

    def test_list_broken_request(self):
        response = requests.get(self.url + 'list/doesnexist.example.') # Missing DomainId
        self.assertEqual(response.status_code, 404)

    def test_getAllDomains(self):
        data = requests.get(self.url + 'getAllDomains').json()
        self.assertEqual(len(data['result']), 4)
        self.assertEqual(len(set(x['id'] for x in data['result'])), 4, 'domain IDs are not unique')

    def test_getDomainMetadata_ALSONOTIFY(self):
        data = requests.get(self.url + 'getDomainMetadata/example.com./ALSO-NOTIFY').json()
        self.assertEqual(len(data['result']), 3)
        print(data)
        self.assertEqual(data['result'][0], '192.0.2.200:53')
        self.assertEqual(data['result'][1], '[2001:db8::53:1]:53')
        self.assertEqual(data['result'][2], '192.0.2.222:53')

    def test_getDomainMetadata_ALSONOTIFY_with_port(self):
        data = requests.get(self.url + 'getDomainMetadata/testdomain.example./ALSO-NOTIFY').json()
        self.assertEqual(len(data['result']), 2)
        self.assertEqual(data['result'][0], '192.0.2.3:1500')
        self.assertEqual(data['result'][1], '192.0.2.222:53')

    def test_getDomainMetadata_ALLOWAXFR(self):
        data = requests.get(self.url + 'getDomainMetadata/example.com./ALLOW-AXFR-FROM').json()
        self.assertEqual(len(data['result']), 4)
        self.assertEqual(data['result'][0], '::1/128')
        self.assertEqual(data['result'][1], '127.0.0.0/8')
        self.assertEqual(data['result'][2], '2001:db8:53::/64')
        self.assertEqual(data['result'][3], '192.0.2.128/25')

    def test_getDomainMetadata_unknown(self):
        response = requests.get(self.url + 'getDomainMetadata/example.com./UNKNOWN-METADATA')
        self.assertEqual(response.status_code, 500)
        data = response.json()
        self.assertEqual(len(data['log']), 1)

    def test_getUpdatedMasters_setNotified(self):
        data = requests.get(self.url + 'getUpdatedMasters').json()
        print(data)
        self.assertEqual(len(data['result']), 3)
        zone_id = data['result'][0]['id']
        serial = data['result'][0]['serial']

        # Tell the backend we notified
        data = requests.patch(self.url + 'setNotified/' + str(zone_id), data = {'serial': serial}).json()
        print(data)
        self.assertTrue(data['result'])

        # Check that the backend knows we did
        data = requests.get(self.url + 'getUpdatedMasters').json()
        print(data)
        self.assertEqual(len(data['result']), 2)
        other_zone_id = data['result'][0]['id']
        self.assertNotEqual(zone_id, other_zone_id)

    def test_getDomainInfo(self):
        data = requests.get(self.url + 'getDomainInfo/example.com.').json()
        print(data)
        self.assertListEqual(data['result']['allow-axfr'],
                             ['::1/128', '127.0.0.0/8'])
        self.assertEqual(data['result']['zone'], 'example.com.')
        self.assertEqual(data['result']['kind'], 'master')