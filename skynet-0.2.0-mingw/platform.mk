PLAT ?= none
PLATS = mingw

CC ?= gcc

.PHONY : none $(PLATS) clean all cleanall

#ifneq ($(PLAT), none)

.PHONY : default

default :
	$(MAKE) $(PLAT)

#endif

none :
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"

SKYNET_LIBS := -lpthread
SHARED := -shared
EXPORT := -Wl,-E

mingw : PLAT = mingw

# Turn off jemalloc and malloc hook on macosx

mingw : MALLOC_STATICLIB :=
mingw : SKYNET_DEFINES :=-DNOUSE_JEMALLOC

mingw :
	$(MAKE) all PLAT=$@ SKYNET_LIBS="$(SKYNET_LIBS)" SHARED="$(SHARED)" EXPORT="$(EXPORT)" MALLOC_STATICLIB="$(MALLOC_STATICLIB)" SKYNET_DEFINES="$(SKYNET_DEFINES)"
