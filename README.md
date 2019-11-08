# PDNS Sysrepo Configurator

## Working with the software
After installing the dependencies and building the PDNS Sysrepo configurator (see below), the configurator can be run and config can be changed.

The tool requires a configuration file in YAML format, there is an annotated example available.
By default `/etc/pdns-configurator/pdns-configurator.yaml` is used, but the `-c` option can be used to specify a different file.

### Loading the YANG models into sysrepo
Use the `sysrepoctl` tool to load the YANG models:

```bash
sysrepoctl -i yang/ietf-inet-types@2013-07-15.yang
sysrepoctl -i yang/pdns-server.yang
```

Should the pdns-server.yang be updated, use `sysrepoctl -U yang/pdns-server.yang`.

### Changing PowerDNS configuration
In lieu of using NETOPEER2 to manage the YANG datastore, the `sysrepocfg` tool can be used to modify configuration at runtime.

`sysrepocfg --edit -v4 -f json -m pdns-server` will spawen an editor allowing changing the configuration in JSON format.

An example of a configuration (in JSON format) may look like this:

```json
{
  "pdns-server:pdns-server": {
    "listen-addresses": [
      {
        "name": "main",
        "ip-address": "127.0.0.1",
        "port": 5300
      }
    ],
    "backend": [
      {
        "name": "main",
        "backendtype": "gsqlite3",
        "database": "/var/lib/powerdns/pdns.sqlite3"
      }
    ],
    "master": true,
    "slave": false
  }
}
```

## How to set up a working development environment
This project requires sysrepo 1.2.x and its C++ bindings, which in turn require the libyang C++ bindings.

### Dependencies
This project uses C++17 and requires the following libraries:

 * boost filesystem (`libboost-filesystem-dev`)
 * libsystemd (`libsystemd-dev`)
 * libyang (see below)
 * mstch (`libmstch-dev`)
 * sdbusplus (see below)
 * spdlog (`libspdlog-dev`)
 * sysrepo (see below)
 * yaml-cpp (`libyaml-cpp-dev`)

It also requires the `meson` package to build.

### installing libyang and sysrepo
```bash
## libyang
export SYSREPO_INSTALL="${HOME}/.local/opt/sysrepo"
curl -LO https://github.com/CESNET/libyang/archive/v1.0-r4.tar.gz
tar xf v1.0-r4.tar.gz
cd libyang-1.0-r4
mkdir build; cd build
cmake -DCMAKE_INSTALL_PREFIX=${SYSREPO_INSTALL} \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_BUILD_TYPE=Release \
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
### You'll need to build and install sysrepo first, **then** create the bindings
### see https://github.com/sysrepo/sysrepo/issues/1653
git clone --branch devel https://github.com/sysrepo/sysrepo.git
cd sysrepo
mkdir build-sysrepo; cd build-sysrepo
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=${SYSREPO_INSTALL} \
    -DREPOSITORY_LOC=${SYSREPO_INSTALL}/etc/sysrepo \
    -DREPO_PATH=${SYSREPO_INSTALL}/etc/sysrepo \
    -DGEN_CPP_BINDINGS=0 \
    -Wno-dev \
    ..
make install
cd ..
mkdir build-sysrepo-cpp; cd build-sysrepo-cpp
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

## sdbusplus is not in Debian, but can be statically linked into the binary during build-time
### You do need to have the autoconf-archive package installed
git clone https://github.com/openbmc/sdbusplus.git
cd sdbusplus
./bootstrap.sh
./configure --disable-sdbuspp --enable-static --prefix=${SYSREPO_INSTALL}
make install
cd ../..
```

### exports for development
```bash
SYSREPO_INSTALL="${HOME}/.local/opt/sysrepo"
export PKG_CONFIG_PATH="${SYSREPO_INSTALL}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export PATH="${SYSREPO_INSTALL}/bin:${PATH}"
export LD_LIBRARY_PATH="${SYSREPO_INSTALL}/lib:${LD_LIBRARY_PATH}"
```

## Building the software
The [Meson](https://mesonbuild.com/Builtin-options.html) build system is used for this project.

To build, run

```bash
meson build
cd build
ninja
```

After updating the sources or the `meson.build` file, running `ninja` in the `build` directory will rebuild the project.
