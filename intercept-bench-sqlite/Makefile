UK_ROOT ?= $(PWD)/workdir/unikraft
UK_BUILD ?= $(PWD)/workdir/build
UK_APP ?= $(PWD)
LIBS_BASE = $(PWD)/workdir/libs
UK_LIBS ?= $(LIBS_BASE)/musl:$(LIBS_BASE)/lwip:$(LIBS_BASE)/sqlite
UK_CFLAGS ?= -std=gnu17

.PHONY: all

all:
	@$(MAKE) -C $(UK_ROOT) L=$(UK_LIBS) A=$(UK_APP) O=$(UK_BUILD) UK_CFLAGS="$(UK_CFLAGS)"

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) L=$(UK_LIBS) A=$(UK_APP) O=$(UK_BUILD) UK_CFLAGS="$(UK_CFLAGS)" $(MAKECMDGOALS)
