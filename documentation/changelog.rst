Changelog
=========

Release 0.2.0
-------------
Released 2020-01-16

New Features
^^^^^^^^^^^^
- :program:`pdns-sysrepo` is now configured using sysrepo, dropping the yaml configuration and commandline switches
- Zone data in master and natives zones can be read from sysrepo
  - Per-zone AXFR ACLs
  - Per-zone ALSO-NOTIFY settings
- Global also-notify setting

Release 0.1.1
-------------
Released 2019-12-13

New Features
^^^^^^^^^^^^
- Add global AXFR ACL

Bug Fixes
^^^^^^^^^
- "/pdns-server:zones/class" was marked mandatory and was given a default without testing, making it un-importable. This is fixed.

Release 0.1.0
-------------
Released 2019-12-09

Initial release.

New Features
^^^^^^^^^^^^
- Several configuration options added:
   - Backends
   - Listen addresses
   - Master operation
   - Slave operation
   - Webserver configuration and API
- Configuration file writing
- Service restarting
- Zone status are available as operational data
- Zones can be created/deleted and the kind (zonetype) can be modified
