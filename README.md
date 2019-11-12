# PDNS Sysrepo Configurator
This is a stand-alone program that adds [YANG](https://en.wikipedia.org/wiki/YANG) support to the [PowerDNS Authoritative Server](https://docs.powerdns.com/authoritative) using [sysrepo](https://sysrepo.org).

It acts as an "indirect" plugin to sysrepo to configure PowerDNS.
Writing the configuration file for the PowerDNS Authoritative Server and restarting the service to load this configuration if needed.

## Installing
```bash
meson build
cd build
ninja install
```

Please read the [documentation](documentation/index.rst) for requirements, dependencies and gotchas.