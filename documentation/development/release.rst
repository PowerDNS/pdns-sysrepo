Building Releases
=================
There are several steps needed to build a release.

Update version number
---------------------
Update the version number in ``meson.build``.
This should be a true version number (i.e. no "alpha", "beta", "rc", or "dev").
This project uses `semver <https://semver.org/>`__.

Given a version number MAJOR.MINOR.PATCH, increment the:

:MAJOR: version when you make incompatible API changes,
:MINOR: version when you add functionality in a backwards compatible manner, and
:PATCH: version when you make backwards compatible bug fixes.

The MINOR will be the most likely candidate for incrementing.

Note that for pre 1.0.0 releases, backwards incompatible changes may be added in MINOR updates.

Update Changelog
----------------
Add the version including all changes to the changelog.

Update the package files
------------------------
Add a new changelog entry to ``builder-support/debian/pdns-sysrepo/changelog``.
The entry can just say "New version released", just ensure the version number matches the one in ``meson.build``.

Commit all changes
------------------
Evidently....

Tag the Commit
--------------

Build the packages
------------------
.. code-block:: bash

  ./builder/build.sh ubuntu-bionic

And store the resulting tarball and packages somewhere.

Update version number
---------------------
   * Increase the PATCH number in ``meson.build``
   * Update ``builder-support/debian/pdns-sysrepo/changelog`` with the previous release and ``+git`` attached
   * Commit
