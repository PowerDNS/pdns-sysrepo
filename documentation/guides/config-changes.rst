Changing PowerDNS configuration
===============================
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

Adding and Removing Zone
^^^^^^^^^^^^^^^^^^^^^^^^
The Yang model has a leaf-list at ``/pdns-server:pdns-server/zones`` that can be manipulated to create and remove zones, e.g:

.. code-block:: json

  {
    "pdns-server:pdns-server": {
      "zones": [
        {
          "name": "foo.example.net.",
          "class": "IN",
          "zonetype": "master"
        }
      ],
    }
  }

Would create a zone named "foo.example.net".

.. note::
  :program:`pdns-sysrepo` will reject changes to zones if the API is disabled.
