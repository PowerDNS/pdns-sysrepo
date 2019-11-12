sysrepo gotchas
===============
Sysrepo uses fifo's and ``/dev/shm`` for communication between its components.
It can happen that sometimes things go south and data is not committed because an application that has a subscription has not exited properly.

To "fix" this, try the following:

Remove all the things
---------------------
This is the big-hammer approach and it busts the running config, but is a reliable fix during development.
First, stop all applications that hold a sysrepo lock (e.g. by having a subscription), then remove all things related to connection management::

  rm /dev/shm/sr_*
  rm ~/.local/opt/etc/sysrepo/sr_*
  # or on a production machine
  rm /etc/sysrepo/sr_*