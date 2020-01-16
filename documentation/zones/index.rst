Managing zones
==============
In :program:`pdns-sysrepo` 2 methods for zone-management exist.
The method is chosen based on whether or not the
``rrset-management`` feature is enabled in the YANG datastore.

The first mode allows defining only the zones themselves,
without being able to modify the content.
This does allow, however, to configure the backends to the
PowerDNS Authoritative Server as the operator requires.
To change zone data, tools like :program:`pdnsutil`, or
database replication can be used.

In the second mode, :program:`pdns-sysrepo` exposes an HTTP
endpoint that is configured as the connection for the
PowerDNS's `remote-backend <https://doc.powerdns.com/authoritative/backends/remote.html>`__.
The operator can change zone data that is store in sysrepo,
the Remote Backend serves out this data.
Enabling this mode also allows per-zone settings like
AXFR ACLs and also-NOTIFY.

.. toctree::
   :maxdepth: 2

   rrset-management-off
   rrset-management-on
