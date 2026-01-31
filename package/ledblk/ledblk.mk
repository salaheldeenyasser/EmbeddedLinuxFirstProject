################################################################################
#
# ledblk
#
################################################################################

LEDBLK_VERSION = 1.0
LEDBLK_SITE = ~/buildroot/package/ledblk/src
LEDBLK_SITE_METHOD = local

LEDBLK_MODULE_SUBDIRS = .

$(eval $(kernel-module))
$(eval $(generic-package))
