#include <stdio.h>
#include <unistd.h>
#include <libudev.h>
#include <poll.h>
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <systemd/sd-journal.h>
#include <time.h>

#ifndef LOW_THRESHOLD
#define LOW_THRESHOLD 20
#endif
#ifndef SUPER_LOW_THRESHOLD
#define SUPER_LOW_THRESHOLD 15
#endif
#ifndef CRITICAL_THRESHOLD
#define CRITICAL_THRESHOLD 10
#endif
#ifndef GOODBYE_THRESHOLD
#define GOODBYE_THRESHOLD 5
#endif
#ifndef JOURNAL_SYSTEMD
#define JOURNAL_SYSTEMD 1
#endif
#ifndef NOTIFICATION_INTERVAL
#define NOTIFICATION_INTERVAL 1
#endif
#ifndef NOTIFICATION_TIMEOUT
#define NOTIFICATION_TIMEOUT 900
#endif

#define log_err(message)\
    do {\
	if(JOURNAL_SYSTEMD) {\
	    sd_journal_print(LOG_ERR, message " | %s:%d\n", __FILE__, __LINE__);\
	} else {\
	    fprintf(stderr, "ERR: " message " | %s:%d\n", __FILE__, __LINE__);\
	}\
	return 1;\
    } while(0)

#define log_err_with_notification(message)\
    do {\
	NotifyNotification* notification = notify_notification_new(message, NULL, NULL);\
	notify_notification_set_urgency(notification, NOTIFY_URGENCY_CRITICAL);\
	if(!notify_notification_show(notification, NULL)) log_err(message "(+ error on showing error notification)");\
	return 1;\
    } while(0)

#define log_info(message)\
    do {\
	if(JOURNAL_SYSTEMD) {\
	    sd_journal_print(LOG_NOTICE, message " | %s:%d\n", __FILE__, __LINE__);\
	} else {\
	    fprintf(stderr, "INFO: " message " | %s:%d\n", __FILE__, __LINE__);\
	}\
    } while(0)

int handle_data(const char* status_str, const char* capacity_str, NotifyNotification** notification, time_t* timer) {
    time_t new_time = time(NULL);
    if(new_time - *timer < (NOTIFICATION_INTERVAL * 60)) {
	return 0;
    }
    *timer = new_time;
    if(!capacity_str || !status_str) log_err("battery status and/or capacity returned NULL!");
    int discharging = *status_str == 'D';
    if(discharging) {
	int capacity;
	sscanf(capacity_str, "%u", &capacity);
	if(capacity < LOW_THRESHOLD && *notification) {
	    if(!notify_notification_close(*notification, NULL)) log_err("couldn't close notification");
	}
	if(capacity < GOODBYE_THRESHOLD) {
	    *notification = notify_notification_new("I see it... the void", "It's sad... but in way... beautiful...", NULL);
	    notify_notification_set_urgency(*notification, NOTIFY_URGENCY_CRITICAL);
	} else if(capacity < CRITICAL_THRESHOLD) {
	    *notification = notify_notification_new("BATTERY IS CRITICAL!", "HELP ME! PLEASE!", NULL);
	    notify_notification_set_urgency(*notification, NOTIFY_URGENCY_CRITICAL);
	} else if(capacity < SUPER_LOW_THRESHOLD) {
	    *notification = notify_notification_new("Battery is really low. Mind hurrying up with the charger?", NULL, NULL);
	    notify_notification_set_urgency(*notification, NOTIFY_URGENCY_LOW);
	} else if(capacity < LOW_THRESHOLD) {
	    *notification = notify_notification_new("Battery is low. Can you get the charger please?", NULL, NULL);
	    notify_notification_set_urgency(*notification, NOTIFY_URGENCY_LOW);
	}
	if(capacity < LOW_THRESHOLD) {
	    notify_notification_set_timeout(*notification, NOTIFICATION_TIMEOUT * 1000);
	    if(!notify_notification_show(*notification, NULL)) log_err("couldn't show notification");
	}
    }
    return 0;
}

int main(void) {
    struct udev *udev;
    struct udev_monitor *monitor;
    struct udev_device *dev;
    int fd;
    NotifyNotification* notification = NULL;

    // start libnotify
    if(!notify_init("i3-battery")) {
	log_err("couldn't initialize libnotify");
	return 1;
    }

    // Create udev object
    udev = udev_new();
    if (!udev) {
	log_err("couldn't create udev object");
        return 1;
    }

    // Create udev monitor
    monitor = udev_monitor_new_from_netlink(udev, "udev");
    if (!monitor) {
	log_err("couldn't create udev monitor");
        udev_unref(udev);
        return 1;
    }

    // Add filter for power supply subsystem
    udev_monitor_filter_add_match_subsystem_devtype(monitor, "power_supply", NULL);

    // Set up monitor to receive events
    udev_monitor_enable_receiving(monitor);

    // Get file descriptor for monitor
    fd = udev_monitor_get_fd(monitor);

    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    log_info("starting to monitor");

    time_t timer = 0;
    // Monitor battery events
    while (1) {
        // Wait for udev event
        if (poll(fds, 1, -1) > 0) {
            // Receive device from monitor
            dev = udev_monitor_receive_device(monitor);
            if (dev) {
                // Check if device type is battery
                // Get battery status and capacity
                const char* status_str = udev_device_get_sysattr_value(dev, "status");
                const char* capacity_str = udev_device_get_sysattr_value(dev, "capacity");
		if(handle_data(status_str, capacity_str, &notification, &timer) == 1) return 1;
                udev_device_unref(dev);
            }
        }
    }

    // Cleanup
    udev_monitor_unref(monitor);
    udev_unref(udev);
    notify_uninit();
}
