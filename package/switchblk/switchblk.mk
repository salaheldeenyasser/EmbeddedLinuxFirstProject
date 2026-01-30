################################################################################
#
# switchblk
#
################################################################################

SWITCHBLK_VERSION = 1.0
SWITCHBLK_SITE = $(BR2_EXTERNAL_MYPROJECT_PATH)/package/switchblk/src
SWITCHBLK_SITE_METHOD = local

SWITCHBLK_MODULE_SUBDIRS = .

$(eval $(kernel-module))
$(eval $(generic-package))
