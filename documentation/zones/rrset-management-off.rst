RRSet management disabled
=========================
:program:`pdns-sysrepo` can manage zones in the PowerDNS Authoritative server.
For this to work, the `API <https://doc.powerdns.com/authoritative/http-api/>`__ must be enabled.
Edit the PowerDNS configuration (using :program:`sysrepocfg`) to enable it:

.. code-block:: json

  {
    "pdns-server:pdns-server":
      "webserver": {
        "api-key": "mySecretKey"
      }
  }

Adding and Removing Zones
-------------------------
The Yang model has a leaf-list at ``/pdns-server:zones`` that can be manipulated to create and remove zones, e.g:

.. code-block:: json

  {
    "pdns-server:zones": [
      {
        "name": "foo.example.net.",
        "class": "IN",
        "zonetype": "native"
      }
    ]
  }

Would create a zone named "foo.example.net".

.. warning::
  Zone names **MUST** be terminated with a dot (".") or they will be rejected by PowerDNS on creation.
  In a future version, dot-less domains will be rejected by the YANG model, or handled transparently by the application.

Removing a zone is as simple as not including the zone in the edit.

Zone state
^^^^^^^^^^
Zones state information is exposed at the ``/pdns-server:zones-state`` in the Operational datastore of sysrepo.