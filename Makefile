.PHONY: build

TARGET=i3-battery
build:
	clang $$(cat config | awk '{ print "-D" $$1 }' | tr '\n' ' ') `pkg-config --cflags libnotify libudev libsystemd` $(TARGET).c -o $(TARGET) `pkg-config --libs libudev libnotify libsystemd`
run: build
	./$(TARGET)
install: build
	cp i3-battery.service ~/.config/systemd/user/
	systemctl --user daemon-reload
	systemctl --user enable i3-battery.service
	systemctl --user restart i3-battery.service
systemd-logs:
	journalctl --user -u i3-battery.service
status:
	systemctl --user status i3-battery.service
