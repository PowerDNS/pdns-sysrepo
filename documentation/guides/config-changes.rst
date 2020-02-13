PowerDNS configuration
======================
The sysrepo datastore can be manipulated in several ways.
In a production environment, there would be a NSO using the `NETCONF <https://en.wikipedia.org/wiki/NETCONF>`__ or `RESTCONF <https://tools.ietf.org/html/rfc8040>`__ protocol to manipulate the YANG datastore on the target device.
For :program:`pdns-sysrepo` a program like `Netopeer2 <https://github.com/CESNET/Netopeer2>`__ could act as a NETCONF server and manipulate the sysrepo datastores.

Installing and configuration Netopeer2 is beyond the scope of this document.

Fortunately, sysrepo comes with a tool to manipulate its datastores.
The :program:`sysrepocfg` tool can be used to change the config.

Working with :program:`sysrepocfg`
----------------------------------
See the Sysrepo documentation and the output of ``sysrepocfg --help`` for more information.

It is advicable to use the JSON format when working on the commandline with configuration changes.
To edit the configuration, the following command will spawn an editor with the config from the running datastore::

  sysrepocfg --edit -f json -m pdns-server

When no configuration exists, the editor shows an empty JSON object.
If configuration exists, the editor shows this.

A configuration in JSON format for the PowerDNS Authoritative Server could look like this:

.. code-block:: json

  {
    "pdns-server:pdns-server": {
      "listen-addresses": [
        {
          "name": "main",
          "ip-address": "127.0.0.1",
          "port": 5300
        }
      ],
      "backend": [
        {
          "name": "main",
          "backendtype": "gsqlite3",
          "database": "/var/lib/powerdns/pdns.sqlite3"
        }
      ],
      "master": true,
      "slave": false,
      "webserver": {
        "api-key": "mySecretKey"
      }
    }
  }

This can be edited, saved after which sysrepo will apply the changes.
Or it won't apply the changes if the configuration does not conform to the YANG model.

Setting AXFR ACLs
-----------------
The `allow-axfr-ips <https://doc.powerdns.com/authoritative/settings.html#allow-axfr-ips>`__ setting of the PowerDNS Authoritative Server that has the netmasks that may AXFR zones from the zones is located in the YANG model at ``/pdns-server:pdns-server/allow-axfr``.
This leaf-list contains leafrefs to ACL entries stored at ``/pdns-server:axfr-access-control-list``.
That list has a ``name`` and one or more networks.
To, for instance, add the loopback IP addresses as allowed addresses, apply the following edit:

.. code-block:: json

  {
    "pdns-server:axfr-access-control-list": [
      {
        "name": "Loopback",
        "network": [
          {
            "name": "v4",
            "ip-prefix": "127.0.0.0/8"
          },
          {
            "name": "v6",
            "ip-prefix": "::1/128"
          }
        ]
      }
    ],
    "pdns-server:pdns-server": {
      "allow-axfr": [
        "Loopback"
      ]
    }
  }

Setting also-notify endpoints
-----------------------------
When sending a `NOTIFY to a slave <https://doc.powerdns.com/authoritative/modes-of-operation.html#master-operation>`__
after the zone is updated, PowerDNS can notify additional IP addresses.

The endpoints can be defined at ``/pdns-server:notify-endpoint`` and can be referenced by name
for all zones in the ``/pdns-server:pdns-server/also-notify`` leaf-list or on a per-zone basis
at ``/pdns-server:zones/zones[name='NNN']/also-notify``.

e.g. the following configuration would notify 192.0.2.3:1500 for all zones and
would notify 192.0.2.200 and 2001:db8::53:1 additionally for the example.com zone:

.. code-block:: json

  {
    "pdns-server:notify-endpoint": [
      {
        "name": "example-200",
        "address": [
          {
            "name": "host 200",
            "ip-address": "192.0.2.200"
          },
          {
            "name": "host 200 on v6",
            "ip-address": "2001:db8::53:1"
          }
        ]
      },
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
    "pdns-server:pdns-server": {
      "also-notify": [
        "example-3"
      ]
    },
    "pdns-server:zones": {
      "zones": [
        {
          "name": "example.com.",
          "zonetype": "master",
          "also-notify": [
            "example-200"
          ],
          "rrset": []
        }
      ]
    }
  }
