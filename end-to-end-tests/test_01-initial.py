import unittest
import subprocess
import json
import os
import tempfile


class PdnsSysrepoBasicTest(unittest.TestCase):
    webserver_port = 8081
    api_key = "foobar"
    dbdir = os.environ.get('PDNS_DB_DIR', '/var/lib/pdns')

    default_config = {
        "pdns-server:pdns-server": {
            "backend": [
                {
                    "name": "backend1",
                    "backendtype": "lmdb",
                    "filename": "{}/pdns.lmdb".format(dbdir)
                }
            ],
            "listen-addresses": [
                {
                    "name": "main IP",
                    "ip-address": "127.0.0.1",
                    "port": "5300"
                }
            ],
            "webserver": {
                "port": webserver_port,
                "api-key": api_key
            },
            "master": False,
            "slave": True
        }
    }

    @classmethod
    def importConfig(cls, config: dict):
        p = subprocess.Popen(['sysrepocfg', '--format', 'json', '--import',
                              '-m', 'pdns-server'], stdin=subprocess.PIPE)
        config_str = json.dumps(config)
        p.communicate(input=config_str.encode('utf-8'))

    @classmethod
    def editConfig(cls, config: dict):
        with tempfile.NamedTemporaryFile() as fp:
            fp.write(json.dumps(config).encode('utf-8'))
            fp.flush()
            subprocess.call(['sysrepocfg', '--format', 'json',
                             '--edit={}'.format(fp.name),
                             '-m', 'pdns-server'])

    @classmethod
    def getRunningDS(cls) -> dict:
        output = subprocess.run(['sysrepocfg', '-X', '--format=json',
                                 '--module=pdns-server',
                                 '--datastore=running'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        print(output)
        return json.loads(output.stdout)

    @classmethod
    def getOperationalDS(cls) -> dict:
        output = subprocess.run(['sysrepocfg', '-X', '--format=json',
                                 '--module=pdns-server',
                                 '--datastore=operational'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        print(output)
        return json.loads(output.stdout)

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
