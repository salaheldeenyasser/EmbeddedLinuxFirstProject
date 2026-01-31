################################################################################
#
# demo_app
#
################################################################################

DEMO_APP_VERSION = 1.0
DEMO_APP_SITE = ~/buildroot/package/demo_app/src
DEMO_APP_SITE_METHOD = local

DEMO_APP_DEPENDENCIES = ledblk switchblk

define DEMO_APP_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" -C $(@D)
endef

define DEMO_APP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/demo_app \
		$(TARGET_DIR)/usr/bin/demo_app
endef

$(eval $(generic-package))
