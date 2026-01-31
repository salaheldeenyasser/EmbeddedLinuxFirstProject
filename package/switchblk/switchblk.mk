################################################################################
#
# switchblk
#
################################################################################

SWITCHBLK_VERSION = 1.0
SWITCHBLK_SITE = ~/buildroot/package/switchblk/src
SWITCHBLK_SITE_METHOD = local

SWITCHBLK_MODULE_SUBDIRS = .

$(eval $(kernel-module))
$(eval $(generic-package))
