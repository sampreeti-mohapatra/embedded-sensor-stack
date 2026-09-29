SENSOR_STACK_VERSION = 1.0
SENSOR_STACK_SITE = $(BR2_EXTERNAL_SENSOR_STACK_PATH)
SENSOR_STACK_SITE_METHOD = local

define SENSOR_STACK_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 $(SENSOR_STACK_PKGDIR)/S50sensor-stack \
		$(TARGET_DIR)/etc/init.d/S50sensor-stack
endef

define SENSOR_STACK_INSTALL_INIT_SYSTEMD
	$(INSTALL) -D -m 0644 $(@D)/systemd/sensord.service \
		$(TARGET_DIR)/usr/lib/systemd/system/sensord.service
	$(INSTALL) -D -m 0644 $(@D)/systemd/procd.service \
		$(TARGET_DIR)/usr/lib/systemd/system/procd.service
	$(INSTALL) -D -m 0644 $(@D)/systemd/metricsd.service \
		$(TARGET_DIR)/usr/lib/systemd/system/metricsd.service
endef

$(eval $(cmake-package))
