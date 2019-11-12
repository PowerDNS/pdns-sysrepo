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

.. _yang-module-install:

Installing YANG modules
-----------------------
This project comes with several YANG modules.
In the source code tarball or git repository, these are stored in ``yang/``.
When installing packages, these files are available in ``/usr/share/pdns-sysrepo/*.yang``.

These modules need to be installed into sysrepo before they can be used.
The :program:`sysrepoctl` tool can be used::

  sysrepoctl -i ietf-inet-types@2013-07-15.yang
  sysrepoctl -i pdns-server.yang