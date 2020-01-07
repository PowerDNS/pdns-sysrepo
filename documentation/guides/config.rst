Configuring :program:`pdns-sysrepo`
===================================
This software is configured with sysrepo under the ``/pdns-server:pdns-sysrepo/`` tree.
As this is in the YANG datastore, these settings can be changed at runtime.

This tree has 2 containers, ``pdns-service`` and ``logging``.

Configuring logging
-------------------
There are two settings that can be configured, the loglevel and the whether or not to log timestamps.

The loglevel (at ``/pdns-server:pdns-sysrepo/logging/level``) is an enum with the following options:

:off: No logging whatsoever
:critical: Only critical errors are logged
:error: Only errors are logged, errors usually lead to program termination
:warning: Logs errors and warnings. A warning indicates something failed that does not terminate the program
:info: Logs all of the above plus some information about what :program:`pdns-sysrepo` is doing, this is the default
:debug: Also logs a lot of information could come in handy when bugs are encountered
:trace: The most verbose of all logs, mostly used for developers

:program:`pdns-sysrepo` can optionally log timestamps (handy when stdout is not sent to a logger) with the boolean at ``/pdns-server:pdns-sysrepo/logging/timestamp``.


Configuring the PowerDNS Service information
--------------------------------------------
To know where to write the PowerDNS configuration, :program:`pdns-sysrepo` reads ``/pdns-server:pdns-sysrepo/pdns-service/config-file``.
The default is ``/etc/powerdns/pdns.conf``, which should work for most Debian-based installations.

As :program:`pdns-sysrepo` might need to restart the PowerDNS Authoritative Server to apply certain settings, it reads the name of the systemd service from ``/pdns-server:pdns-sysrepo/pdns-service/name``.

dbus permissions
----------------
:program:`pdns-sysrepo` uses `D-Bus <https://www.freedesktop.org/wiki/Software/dbus/>`__ to communicate with systemd to restart the PowerDNS service.
In order to be able to restart the PowerDNS service, the user under which :program:`pdns-sysrepo` runs needs to have permission to do this.
Within D-Bus, `Polkit <https://en.wikipedia.org/wiki/Polkit>`__ is used for authorizing actions.

.. note::
  As of version 0.1.0, :program:`pdns-sysrepo` runs as root. This will change in a future version.

After installing, a polkit rules file is installed in ``/usr/share/polkit-1/rules.d/pdns.rules``:

.. literalinclude:: ../../polkit/10-pdns.rules
   :language: javascript

When using all the defaults, this file is enough to authorize :program:`pdns-sysrepo` to restart the PowerDNS service.

To change the user or the service name, copy this file to ``/etc/polkit-1/rules.d`` and edit as required.
