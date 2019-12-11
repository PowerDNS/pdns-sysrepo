import json
import os
import subprocess
import tempfile
import unittest

class PdnsSysrepoTest(unittest.TestCase):
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