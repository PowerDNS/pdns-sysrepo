Configuring :program:`pdns-sysrepo`
===================================
The software reads a YAML configuration file on startup.
By default, this file is ``/etc/pdns-sysrepo/pdns-sysrepo.yaml``, but this can be changed using the ``-c`` command line argument.

These are the default settings and their descriptions:

.. literalinclude:: ../../pdns-sysrepo.example.yaml
   :language: yaml

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
