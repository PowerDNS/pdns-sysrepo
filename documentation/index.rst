.. PowerDNS Syrepo Plugin documentation master file, created by
   sphinx-quickstart on Tue Nov 12 11:40:45 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

PowerDNS Sysrepo Plugin
=======================
:program:`pdns-sysrepo` is a stand-alone program that adds `YANG <https://en.wikipedia.org/wiki/YANG>`__ support to the `PowerDNS Authoritative Server <https://docs.powerdns.com/authoritative>`__ using `sysrepo <https://sysrepo.org>`__.

It acts as an "indirect" plugin to sysrepo to configure PowerDNS.
Writing the configuration file for the PowerDNS Authoritative Server and restarting the service to load this configuration if needed.
For correct startup configuration, the PowerDNS service is made to depend on :program:`pdns-sysrepo`, meaning that PowerDNS is only started after :program:`pdns-sysrepo` has successfully started up.

Conceptually, the whole system looks like this::

                           +--------- DNS Server --------------------+
                           |                                         |
  [NSO] <= NETCONF => [Netopeer] <=> [sysrepo] <=> [pdns-sysrepo]    |
                           |                        ^       |        |
                           |                  calls |       | writes |
                           |                        v       |        |
                           |           /----[systemd]       v        |
                           |          | restarts       <config file> |
                           |          |                  ^           |
                           |          |      /----------/            |
                           |          v     /   reads                |
                           +-----[pdns_server]-----------------------+

In pseudo code :program:`pdns-sysrepo` does the following:

* Read own configuration
* Read PowerDNS configuration from sysrepo's running datastore
* Write PowerDNS configuration file
* Register callbacks for configuration changes
* Notify systemd that it has started
* Wait for callbacks to be called (loop)
   * On configuration change
   * Write new PowerDNS configuration file
   * Send IPC to systemd to restart PowerDNS Service

.. toctree::
   :maxdepth: 2
   :hidden:

   install
   guides/config
   guides/config-changes
   development/index
   changelog