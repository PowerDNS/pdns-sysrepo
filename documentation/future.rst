Future plans
============
There are several plans on future work for :program:`pdns-sysrepo`.

0.4.0
-----
* Refactor the code
   * Move all headers to one ``include`` directory
   * Move ``sr_wrapper`` and ``iputils`` to ``src``
   * Fix the ``include-dirs``
* Run the service as a non-priviled user
* Properly check if the PowerDNS service was restarted
   * Rollback on failure
   * Have a ``config-state`` node that shows the current config of the service

0.5.0
-----
* ``Sync config`` RPC
* ``Sync zone config`` RPC
