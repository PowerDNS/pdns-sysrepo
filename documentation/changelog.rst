Changelog
=========

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
