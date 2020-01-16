Zone data in sysrepo
====================
When the ``rrset-management`` feature is enabled, zone data can be stored in sysrepo.

.. code-block:: bash

  sysrepoctl -c pdns-server -e rrset-management

Each zone is in the YANG tree at ``/pdns-server:zones/zones[name=...]``.

Zone requirements
-----------------
In order for the PowerDNS Authoritative server to be able
to serve the zone, **at least** an SOA record with an owner
name equal to the zone name is required.

Per-zone AXFR ACLs
------------------
The AXFR ACLs at ``/pdns-server:axfr-access-control-list`` can be used
as per-zone ACLs instead of global AXFR ACLs.
These can be set using a leaf-ref at
``/pdns-server:zones/zones[name=...]/allow-axfr``:

.. code-block:: json

  {
    "pdns-server:axfr-access-control-list": [
      {
        "name": "host-one",
        "network": [
          {
            "name": "v6",
            "ip-prefix": "2001:DB8::/32"
          },
          {
            "name": "v4",
            "ip-prefix": "192.0.2.0/24"
          }
        ]
      }
    ],
    "pdns-server:zones": {
      "zones": [
        {
          "name": "example.com.",
          "allow-axfr": [
              "host-one"
          ]
        }
      ]
    }
  }

Per-zone notifications
----------------------
Some zones need to send NOTIFY messages on updates to nameservers
other than the ones configured at ``/pdns-server:pdns-server/also-notify`` and
the NS records in the zone.

Per-zone notifications can be set with the leaf-ref at
``/pdns-server:zones/zones[name=...]/also-notify``:

.. code-block:: json

  {
    "pdns-server:notify-endpoint": [
      {
        "name": "example-3",
        "address": [
          {
            "name": "host 3",
            "ip-address": "192.0.2.3",
            "port": 1500
          }
        ]
      }
    ],
    "pdns-server:zones": {
      "zones": [
        {
          "name": "example.com.",
          "also-notify": [
              "example-3"
          ]
        }
      ]
    }
  }


Example zone
------------

.. code-block:: json

  {
    "pdns-server:zones": {
      "zones": [
        {
          "name": "testdomain.example.",
          "zonetype": "master",
          "rrset": [
            {
              "owner": "ipv6.testdomain.example.",
              "type": "AAAA",
              "ttl": 600,
              "rdata": {
                "AAAA": {
                  "address": "2001:db8::12:AB:1"
                }
              }
            },
            {
              "owner": "testdomain.example.",
              "type": "NS",
              "ttl": 3600,
              "rdata": {
                "NS": {
                  "nsdname": [
                    "ns5.delegation.example.",
                    "ns6.delegation.example."
                  ]
                }
              }
            },
            {
              "owner": "testdomain.example.",
              "type": "A",
              "ttl": 3600,
              "rdata": {
                "A": {
                  "address": [
                    "192.0.2.1",
                    "192.0.2.3"
                  ]
                }
              }
            },
            {
              "owner": "testdomain.example.",
              "type": "SOA",
              "ttl": 3600,
              "rdata": {
                "SOA": {
                  "mname": "ns5.delegation.example.",
                  "rname": "hostmaster.example.nl.",
                  "serial": 2020011501,
                  "refresh": 10800,
                  "retry": 3600,
                  "expire": 604800,
                  "minimum": 3600
                }
              }
            }
          ]
        }
      ]
    }
  }
