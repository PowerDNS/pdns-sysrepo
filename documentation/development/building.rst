Building :program:`pdns-sysrepo`
================================
This page will guide developers through the set up of a working development environment.

This project is written in C++17, meaning a modern compiler is required.
GCC 7 and clang 5 at a minimum.

The `Meson <https://mesonbuild.com>`__ (``meson`` package) build-system is used to build the software.

This list has the rest of the dependencies, including the package names for Debian-based distributions.

.. note::

    Note that ``libsysrepo``, ``libyang`` and ``sdbusplus`` are not available on most distributions.
    This repository has a way to build these packages for Ubuntu Bionic, see :ref:`building_pkgs`.
    Alternatively, they can be built and installed into a local directory, see :ref:`dev-setup`.

* `boost filesystem <https://www.boost.org/doc/libs/1_71_0/libs/filesystem/doc/index.htm>`__
* `boost program options <https://www.boost.org/doc/libs/1_71_0/doc/html/program_options.html>`__
* `libsystemd <https://freedesktop.org/wiki/Software/systemd/>`__ version 221 or higher
* `libyang-cpp <https://github.com/CESNET/libyang>`__ version 1.0.0 or higher
* `mstch <https://github.com/no1msd/mstch>`__
* `sdbusplus <https://github.com/openbmc/sdbusplus>`__
* `spdlog <https://github.com/gabime/spdlog>`__ version 1.0.0 or higher
* `sysrepo <https://www.sysrepo.org/>`__ version 1.2.0 or higher
* `yaml-cpp <https://github.com/jbeder/yaml-cpp>`__ version 0.5.0 or higher
* `cpprestsdk <https://github.com/Microsoft/cpprestsdk>`__

To install the distribution provided dependencies on a Debian system::

  apt install -y libboost-filesystem-dev libboost-system-dev libboost-program-options-dev libsystemd-dev libmstch-dev libspdlog-dev libyaml-cpp-dev libcpprest-dev

Furthermore, some other build requirements should be installed::

  apt install -y meson g++ 

.. _dev-setup:

Setting up a development environment
------------------------------------
After installing the distribution-provided dependencies, ``libsysrepo``, ``libyang`` and ``sdbusplus`` can be installed:

This requires some more development packages::

  apt install -y cmake autoconf-archive

Now build the dependent software, it will be installed into ``~/.local/opt/sysrepo``.

.. code-block:: bash

    ## libyang
    export SYSREPO_INSTALL="${HOME}/.local/opt/sysrepo"
    git clone https://github.com/CESNET/libyang.git
    cd libyang
    mkdir build; cd build
    cmake -DCMAKE_INSTALL_PREFIX=${SYSREPO_INSTALL} \
            -DCMAKE_INSTALL_LIBDIR=lib \
            -DCMAKE_BUILD_TYPE=Debug \
            -DENABLE_LYD_PRIV=ON \
            -DGEN_LANGUAGE_BINDINGS=1 \
            -DGEN_PYTHON_BINDINGS=0 \
            -DGEN_CPP_BINDINGS=1 \
            ..
    make install
    cd ../..

    export PKG_CONFIG_PATH="${SYSREPO_INSTALL}/lib/pkgconfig:${PKG_CONFIG_PATH}"
    export LIBRARY_PATH="${SYSREPO_INSTALL}/lib:${LIBRARY_PATH}"

    ## sysrepo
    git clone --branch devel https://github.com/sysrepo/sysrepo.git
    cd sysrepo
    mkdir build; cd build
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=${SYSREPO_INSTALL} \
        -DREPOSITORY_LOC=${SYSREPO_INSTALL}/etc/sysrepo \
        -DREPO_PATH=${SYSREPO_INSTALL}/etc/sysrepo \
        -DGEN_CPP_BINDINGS=1 \
        -Wno-dev \
        ..
    make install
    cd ../..

    ## sdbusplus
    git clone https://github.com/openbmc/sdbusplus.git
    cd sdbusplus
    ./bootstrap.sh
    ./configure --disable-sdbuspp --enable-static --prefix=${SYSREPO_INSTALL}
    make install
    cd ../..

Building :program:`pdns-sysrepo`
--------------------------------
If the external dependencies have been installed separately, export the following:

.. code-block:: bash

    SYSREPO_INSTALL="${HOME}/.local/opt/sysrepo"
    export PKG_CONFIG_PATH="${SYSREPO_INSTALL}/lib/pkgconfig:${PKG_CONFIG_PATH}"
    export PATH="${SYSREPO_INSTALL}/bin:${PATH}"
    export LD_LIBRARY_PATH="${SYSREPO_INSTALL}/lib:${LD_LIBRARY_PATH}"

Now use :program:`meson` to create the build directory, in the root of the git repository::

    meson build

The :program:`ninja` program can be used to build the software::

    cd build
    ninja

The :program:`pdns-sysrepo` binary is now built in the ``build`` directory and can be run from there.

When developing, running the :program:`ninja` command in the ``build`` directory is enough to rebuild the program.
After editing the ``meson.build`` file, running :program:`ninja` will regenerate the build files as well.

libyang and sysrepo documentation
---------------------------------
Sysrepo and libyang come with extended documentation in `Doxygen <http://www.doxygen.nl/>`__\ -format.
These documents are built separately from the programs.
First, install the documentation dependencies::

  apt install -y doxygen graphviz

Then go to the git repository for each program and build the documentation::

  mkdir build-doc
  cd build-doc
  cmake ..
  make doc

HTML documentation can now be found in the ``doc/html`` directory of the git repository.

Installing YANG modules
-----------------------
See :ref:`yang-module-install`.