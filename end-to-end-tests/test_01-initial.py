import pdns_sysrepo_tests

class PdnsSysrepoBasicTest(pdns_sysrepo_tests.PdnsSysrepoTest):

    def testImported(self):
        self.importConfig(self.default_config)
        data = self.getRunningDS()
        self.assertFalse(data["pdns-server:pdns-server"]["master"])
        self.assertTrue(data["pdns-server:pdns-server"]["slave"])
        self.assertEqual(data["pdns-server:pdns-server"]["webserver"]["api-key"], self.api_key)
        self.assertEqual(data["pdns-server:pdns-server"]["webserver"]["port"], self.webserver_port)
        self.assertEqual(len(data["pdns-server:pdns-server"]["listen-addresses"]), 1)

    def testUpdateConfig(self):
        self.importConfig(self.default_config)
        self.editConfig({
            "pdns-server:pdns-server": {
                "slave": False
            }
        })
        data = self.getRunningDS()
        self.assertFalse(data["pdns-server:pdns-server"]["slave"])

    def testAddZone(self):
        self.importConfig(self.default_config)
        zonename = "foo.example.com."
        zonetype = "native"
        self.editConfig({
            "pdns-server:zones": [
                {
                    "name": zonename,
                    "class": "IN",
                    "zonetype": zonetype
                }
            ]
        })

        data = self.getOperationalDS()
        self.assertEqual(len(data["pdns-server:zones-state"]["zones"]), 1)
        self.assertEqual(
            data["pdns-server:zones-state"]["zones"][0]["name"], zonename)
        self.assertEqual(
            data["pdns-server:zones-state"]["zones"][0]["zonetype"],
            zonetype)

    def testChangeZoneType(self):
        self.importConfig(self.default_config)
        zonename = "edit.example.com."
        zonetype = "native"
        self.editConfig({
            "pdns-server:zones": [
                {
                    "name": zonename,
                    "class": "IN",
                    "zonetype": zonetype
                }
            ]
        })

        zonetype = "master"
        self.editConfig({
            "pdns-server:zones": [
                {
                    "name": zonename,
                    "class": "IN",
                    "zonetype": zonetype
                }
            ]
        })

        data = self.getOperationalDS()
        self.assertEqual(len(data["pdns-server:zones-state"]["zones"]), 1)
        self.assertEqual(
            data["pdns-server:zones-state"]["zones"][0]["name"], zonename)
        self.assertEqual(
            data["pdns-server:zones-state"]["zones"][0]["zonetype"],
            zonetype)
