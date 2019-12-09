# PDNS Sysrepo Configurator
This is a stand-alone program that adds [YANG](https://en.wikipedia.org/wiki/YANG) support to the [PowerDNS Authoritative Server](https://docs.powerdns.com/authoritative) using [sysrepo](https://sysrepo.org).

It acts as an "indirect" plugin to sysrepo to configure PowerDNS.
Writing the configuration file for the PowerDNS Authoritative Server and restarting the service to load this configuration if needed.

Apart from configuration, this plugin can also manage zones stored in PowerDNS.

## Requirements
`pdns-sysrepo` requires a recent PowerDNS Authoritative Server (4.3) and has several dependencies (e.g. sysrepo, libyang and sdbusplus) that are not shipped in many distributions.

Please read the [documentation](documentation/install.rst) for requirements, installation and usage instructions.

## Project state
Pre-alpha

## Copyright and license
Copyright © 2019 Pieter Lexis
Copyright © 2019 PowerDNS.COM BV

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.