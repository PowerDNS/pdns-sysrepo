import requests

class TestRemoteBackend:
    url = "http://127.0.0.1:9100/dns/"

    def test_lookup_example_com_SOA(self):
        data = requests.get(self.url + 'lookup/example.com./SOA').json()
        record = data['result'][0]
        assert(record['content'] == 'ns1.example.nl. hostmaster.example.nl. 2020011501 10800 3600 604800 3600')
        assert(record['qname'] == 'example.com.')
        assert(record['qtype'] == 'SOA')
        assert(record['ttl'] == 3600)

    def test_lookup_doesntexist_example_com_A(self):
        data = requests.get(self.url + 'lookup/doesntexist.example.com./A').json() # This is an NXD
        assert(len(data['result']) == 0)

    def test_lookup_example_com_A(self):
        data = requests.get(self.url + 'lookup/example.com./A').json()
        assert(len(data['result']) == 2)
        seen_dot_1 = False
        seen_dot_3 = False
        for record in data['result']:
            assert(record['qname'] == 'example.com.')
            assert(record['qtype'] == 'A')
            assert(record['ttl'] == 3600)
            if record['content'] == '192.0.2.1':
                seen_dot_1 = True
            if record['content'] == '192.0.2.3':
                seen_dot_3 = True

        assert(seen_dot_1)
        assert(seen_dot_3)

    def test_lookup_domain_does_not_exist(self):
        data = requests.get(self.url + 'lookup/nosuchdomain.example./A').json()
        assert(len(data['result']) == 0)
        assert(len(data['log']) == 1)

    def test_list_example_com(self):
        data = requests.get(self.url + 'list/-1/example.com.').json()
        assert(len(data['result']) == 6)

    def test_list_nonexist(self):
        data = requests.get(self.url + 'list/-1/doesnexist.example.').json()
        assert(len(data['result']) == 0)
        assert(len(data['log']) == 1)

    def test_list_broken_request(self):
        response = requests.get(self.url + 'list/doesnexist.example.') # Missing DomainId
        assert(response.status_code == 404)

    def test_getAllDomains(self):
        data = requests.get(self.url + 'getAllDomains').json()
        assert(len(data['result']) == 3)
        assert len(set(x['id'] for x in data['result'])) == 3, 'domain IDs are not unique'

    def test_getDomainMetadata_ALSONOTIFY(self):
        data = requests.get(self.url + 'getDomainMetadata/example.com./ALSO-NOTIFY').json()
        assert(len(data['result']) == 2)
        print(data)
        assert(data['result'][0] == '192.0.2.200:53')
        assert(data['result'][1] == '[2001:db8::53:1]:53')

    def test_getDomainMetadata_ALSONOTIFY_with_port(self):
        data = requests.get(self.url + 'getDomainMetadata/testdomain.example./ALSO-NOTIFY').json()
        assert(len(data['result']) == 1)
        assert(data['result'][0] == '192.0.2.3:1500')

    def test_getDomainMetadata_ALLOWAXFR(self):
        data = requests.get(self.url + 'getDomainMetadata/example.com./ALLOW-AXFR-FROM').json()
        assert(len(data['result']) == 2)

    def test_getDomainMetadata_unknown(self):
        response = requests.get(self.url + 'getDomainMetadata/example.com./UNKNOWN-METADATA')
        assert(response.status_code == 500)
        data = response.json()
        assert(len(data['log']) == 1)

    def test_getUpdatedMasters_setNotified(self):
        data = requests.get(self.url + 'getUpdatedMasters').json()
        print(data)
        assert(len(data['result']) == 3)
        zone_id = data['result'][0]['id']
        serial = data['result'][0]['serial']

        # Tell the backend we notified
        data = requests.patch(self.url + 'setNotified/' + str(zone_id), data = {'serial': serial}).json()
        print(data)
        assert(data['result'] is True)

        # Check that the backend knows we did
        data = requests.get(self.url + 'getUpdatedMasters').json()
        print(data)
        assert(len(data['result']) == 2)
        other_zone_id = data['result'][0]['id']
        assert(zone_id != other_zone_id)