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

    def testImported(self):
        self.importConfig(
            {
                "pdns-server:pdns-server": {
                    "backend": [
                        {
                            "name": "backend1",
                            "backendtype": "lmdb",
                            "filename": "{}/pdns.lmdb".format(self.dbdir)
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
                        "port": self.webserver_port,
                        "api-key": self.api_key
                    },
                    "master": False,
                    "slave": True
                }
            }
        )
        output = subprocess.run(['sysrepocfg', '-X', '--format=json',
                                 '--module=pdns-server',
                                 '--datastore=running'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        print(output)
        data = json.loads(output.stdout)
        self.assertFalse(data["pdns-server:pdns-server"]["master"])
        self.assertTrue(data["pdns-server:pdns-server"]["slave"])

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

        output = subprocess.run(['sysrepocfg', '-X', '--format=json',
                                 '--module=pdns-server',
                                 '--datastore=operational'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        print(output)
        data = json.loads(output.stdout)
        self.assertEqual(len(data["pdns-server:zones-state"]["zones"]), 1)
        self.assertEqual(
            data["pdns-server:zones-state"]["zones"][0]["name"], zonename)
        self.assertEqual(
            data["pdns-server:zones-state"]["zones"][0]["zonetype"],
            zonetype)
