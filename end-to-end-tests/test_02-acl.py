import pdns_sysrepo_tests
import requests
import time


class PdnsSysrepoAclTest(pdns_sysrepo_tests.PdnsSysrepoTest):
    def testAllowAxfr(self):
        aclHostName = "coolHost"
        aclAddress1 = "192.0.2.1/32"
        aclAddress2 = "2001:DB8:22::/64"

        config = self.default_config
        config["pdns-server:axfr-access-control-list"] = [
            {
                "name": aclHostName,
                "network": [
                    {
                        "name": "1",
                        "ip-prefix": aclAddress1
                    },
                    {
                        "name": "2",
                        "ip-prefix": aclAddress2
                    }
                ]
            }
        ]

        self.importConfig(config)
        axfr_add = {"pdns-server:pdns-server": {
            "allow-axfr": [aclHostName]
        }}
        self.editConfig(axfr_add)
        time.sleep(0.5)  # Ensure we wait for the restart

        data = requests.get(
            "http://127.0.0.1:8081/api/v1/servers/localhost/config",
            headers={"X-API-Key": self.api_key}).json()

        for e in data:
            if e["name"] == "allow-axfr-ips":
                self.assertEqual(e["value"],
                                 "{}, {}".format(aclAddress1,
                                                 aclAddress2).lower())
