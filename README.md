## i3-battery

Linux daemon to send battery notifications. Done in the most bare bones way I could.
Battery notification daemon for Linux notebooks in C. Done in the most bare bones way I could.

1. libnotify (to send the actual notifications)
2. udev (to watch when the energy levels and battery status changes)
3. systemd-dev (for journaling to systemd)

## Config

You may want to change the default values of `config` before building:

```
LOW_THRESHOLD=20
SUPER_LOW_THRESHOLD=15
CRITICAL_THRESHOLD=10
GOODBYE_THRESHOLD=5
JOURNAL_SYSTEMD=1
NOTIFICATION_INTERVAL=5
```

When `JOURNAL_SYSTEMD` is `0`, the program will use `fprintf` instead.

`NOTIFICATION_INTERVAL` is the minimum number of minutes that must pass before a new notification is sent.

When possible, fatal errors will be notified to desktop (don't matter the `JOURNAL_SYSTEMD` value)

## Install to Systemd as a User unit

Modify `ExecStart` in `i3-battery.service` to point to where you want to store the executable and then:

```
make install
```

## Using without Systemd

If you don't want to use Systemd, just set `JOURNAL_SYSTEMD=0` and remove `libsystemd-dev` from the `Makefile`
