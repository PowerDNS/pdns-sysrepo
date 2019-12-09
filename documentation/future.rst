Future plans
============
There are several plans on future work for :program:`pdns-sysrepo`.

0.1.0
-----
* End-to-end tests
* Unit tests

0.2.0
-----
* Configure :program:`pdns-sysrepo` using sysrepo
* Transparently handle zones with and without trailing dot consistently
* Configure RRSets on Native and Master domains
* Add AXFR ACLs

  * Per-zone
  * Globally

0.3.0
-----
* Refactor the code
   * Move all headers to one ``include`` directory
   * Move ``sr_wrapper`` and ``iputils`` to ``src``
   * Fix the ``include-dirs``
* Run the service as a non-priviled user
* Properly check if the PowerDNS service was restarted
   * Rollback on failure
   * Have a ``config-state`` node that shows the current config of the service
* ``Sync config`` RPC
* ``Sync zone config`` RPC

Long-term
---------
* Have a PowerDNS Backend read zone info from sysrepo directly
