Installing ``pdns-sysrepo``
===========================
This page guides you through the installation of :program:`pdns-sysrepo`.

Installing the software
-----------------------

Requirements
^^^^^^^^^^^^
:program:`pdns-sysrepo` is written in C++17 and requires a fairly modern \*nix operating system.
Specifically, it targets Ubuntu Bionic.

Installing from packages
^^^^^^^^^^^^^^^^^^^^^^^^
.. note::
    If the packages are not provided, use the instructions :ref:`below <building_pkgs>`.

The easiest way to install :program:`pdns-sysrepo` is use the packages.

If these are in a repository, add the repository to the ``sources.list`` and use :program:`apt` or :program:`apt-get` to install the ``pdns-sysrepo`` package.

Without a repository, the packages can be installed using the :program:`apt` program.
Using :program:`apt` ensures that dependencies from the Debian respositories are installed automatically.
To install this program, run the following in the directory of package files::

  apt install ./pdns-sysrepo_*.deb ./lib*.deb

.. _building_pkgs:

Building packages
^^^^^^^^^^^^^^^^^
To build packages, this project uses `pdns-builder <https://github.com/PowerDNS/pdns-builder>`__.
This is a a `docker <https://www.docker.com>`__-based system that uses templated ``Dockerfile``\ s to build source tarballs and Debian packages.

As some dependencies are not available, the builder also builds these packages.
For Ubuntu Bionic, these are the following:

 - Libyang
 - Sysrepo
 - sbdplus

To build packages for these dependencies and :program:`pdns-sysrepo`, run the following command::

  ./builder/build.sh ubuntu-bionic

Now get some coffee and wait, the program will tell where the packages are located after building.

Building from Source
^^^^^^^^^^^^^^^^^^^^
First install all dependencies::

  apt install -y libboost-filesystem-dev libboost-system-dev libboost-program-options-dev libsystemd-dev libmstch-dev libspdlog-dev libyaml-cpp-dev

``libyang``\ 's C++ bindings, ``sysrepo``\ 's C++ binding and ``libsdbusplus`` need to be installed as well.
Refer to the :ref:`development setup <dev-setup>` section on how to install these.

From the root of the git repository or an extracted source tarball, :program:`pdns-sysrepo` can be built as follows::

  meson build
  cd build
  ninja install

Post installation steps
-----------------------
No matter the installation method, several actions need to be taken to ensure the service operates correctly.

.. _yang-module-install:

Install YANG modules
^^^^^^^^^^^^^^^^^^^^
This project comes with several YANG modules.
In the source code tarball or git repository, these are stored in ``yang/``.
When installing packages, these files are available in ``/usr/share/pdns-sysrepo/*.yang``.

These modules need to be installed into sysrepo before they can be used.
The :program:`sysrepoctl` tool can be used::

  sysrepoctl -i ietf-inet-types@2013-07-15.yang
  sysrepoctl -i iana-dns-class-rr-type@2019-06-27.yang
  sysrepoctl -i pdns-server.yang

Configure :program:`pdns-sysrepo`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Configure the service in ``/etc/pdns-sysrepo/pdns-sysrepo.yaml``.
See :doc:`guides/config` for more information.

Create an initial PowerDNS startup config
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The YANG model comes without listen-addresses and backends for the PowerDNS Authoritative Server configured.
As these are required for the server to start and there are no defaults from the package, these need to be added to the startup config.

See :doc:`guides/config-changes` on how to change the settings for the pdns-server YANG model.
Use ``-d startup`` in the :program:`sysrepocfg` invocation.

Then copy the startup config to the running config::

  sysrepocfg -m pdns-server -d running -C startup

Enable and start the service
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Neither the package nor the source installation starts or enables the :program:`pdns-sysrepo` service automatically, as it will fail starting when the YANG modules are not installed.
This has to be done after all the above steps have been completed::

  systemctl enable pdns-sysrepo.service
  systemctl start pdns-sysrepo.service
