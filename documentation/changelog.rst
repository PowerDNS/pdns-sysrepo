Changelog
=========
Release 0.3.3
-------------
Released 2020-04-03

This release only updates several dependencies:

- sysrepo to 1.4.30
- libyang to 1.0.151

Release 0.3.2
-------------
Released 2020-03-25

This release only updates several dependencies:

- sysrepo to 1.4.26
- libyang to 1.0.146

Release 0.3.1
-------------
Released 2020-03-13

Bug fixes
^^^^^^^^^
- Fix crashing bug that occurred when data nodes in sysrepo were not in an expected order

Release 0.3.0
-------------
Released 2020-02-13

New Features
^^^^^^^^^^^^
- Implement zone slaving, zone data is store in the operational datastore
- Wildcard records in zones work

YANG Model updates
^^^^^^^^^^^^^^^^^^
- Added a dnsname, zonename and hostname types
- Added an option to disallow service restarts

Release 0.2.1
-------------
Released 2020-01-22

Bug fixes
^^^^^^^^^
- Fix an issue when finding zones
- Fix a crash in getUpdatedMasters when no zones are defined
- Ensure domain-id creation is always incremented

YANG Model updates
^^^^^^^^^^^^^^^^^^
- Fix the zones-state container presence for the rrset-management feature
- Allow more than one piece in RDATA for relevant rrtypes

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
