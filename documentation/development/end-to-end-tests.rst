Running the end-to-end tests
============================
The end-to-end tests rely on having a PowerDNS Authoritative Server 4.3.x (or recent master build) with the `LMDB backend <https://doc.powerdns.com/authoritative/backends/lmdb.html>`__.
This PowerDNS service must be controlled by :program:`systemd` for the tests to succeed.

This document will describe how to set this up on your development machine.
It assumes several things, adjust files as needed:

   * PowerDNS is built in ``/home/lieter/src/PowerDNS/pdns``
   * The service is run by ``lieter:users``
   * The configuration is located in ``/home/lieter/src/PowerDNS/pdns-conf/netconf``

``pdns-sysrepo-test.service``
-----------------------------
Add a service file for the test PowerDNS as ``/etc/systemd/system/pdns-sysrepo-test.service`` with this content:

.. code-block:: ini

    [Unit]
    Description=PowerDNS Authoritative Server
    Documentation=man:pdns_server(1) man:pdns_control(1)
    Documentation=https://doc.powerdns.com
    Wants=network-online.target
    After=network-online.target mysqld.service postgresql.service slapd.service mariadb.service

    [Service]
    # Modify as needed
    ExecStart=/home/lieter/src/PowerDNS/pdns/pdns/pdns_server --config-dir=/home/lieter/src/PowerDNS/pdns-conf/netconf --socket-dir=/home/lieter/src/PowerDNS/pdns-conf/netconf --guardian=no --daemon=no --disable-syslog --log-timestamp=no --write-pid=no
    Type=notify
    Restart=on-failure
    RestartSec=1
    StartLimitInterval=0
    # Modify as needed
    User=lieter
    # Modify as needed
    Group=users

    # Sandboxing
    CapabilityBoundingSet=CAP_NET_BIND_SERVICE CAP_SETGID CAP_SETUID CAP_CHOWN CAP_SYS_CHROOT
    LockPersonality=true
    ProtectControlGroups=true
    ProtectKernelModules=true
    ProtectKernelTunables=true
    RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6
    RestrictNamespaces=true
    RestrictRealtime=true
    SystemCallArchitectures=native
    SystemCallFilter=~ @clock @debug @module @mount @raw-io @reboot @swap @cpu-emulation @obsolete

    [Install]
    WantedBy=multi-user.target

Build and run :program:`pdns-sysrepo`
-------------------------------------
Build :program:`pdns-sysrepo` in the :doc:`normal way <building>`.

Then create a configuration for it at ``/home/lieter/src/PowerDNS/pdns-conf/netconf/pdns-sysrepo.yaml``

.. code-block:: yaml

  pdns_conf: /home/lieter/src/PowerDNS/pdns-conf/netconf/pdns.conf

  pdns_service: "pdns-sysrepo-test.service"

Then start it on the foreground::

  ./pdns-sysrepo -c /home/lieter/src/PowerDNS/pdns-conf/netconf/pdns-sysrepo.yaml -l trace

Run the end-to-end tests
------------------------
In another terminal, go to the ``end-to-end-tests`` directory and run the tests::

  PDNS_DB_DIR="/home/lieter/src/PowerDNS/pdns-conf/netconf" ./runtests.sh