# GNU Make project makefile autogenerated by Premake

ifndef config
  config=server
endif

ifndef verbose
  SILENT = @
endif

.PHONY: clean prebuild prelink

ifeq ($(config),server)
  RESCOMP = windres
  TARGETDIR = ../bin
  TARGET = $(TARGETDIR)/gmsv_navigation_linux.dll
  OBJDIR = obj/Server
  DEFINES += -DGMMODULE -DRAD_TELEMETRY_DISABLED -DLUA_SERVER -DNO_MALLOC_OVERRIDE -D_LINUX -DLINUX -D_POSIX -DPOSIX -DGNUC
  INCLUDES += -I../../source-sdk-2013/mp/src/lib/public -I../../source-sdk-2013/mp/src/lib/common -I../../source-sdk-2013/mp/src/public -I../../source-sdk-2013/mp/src/public/tier0 -I../../source-sdk-2013/mp/src/public/tier1 -I../../source-sdk-2013/mp/src/common -I../../garrysmod_common/include -I../../garrysmod_common/include/garrysmod/lua -I.. -I../../source-sdk-2013/mp/src/thirdparty/SDL2 -I../../source-sdk-2013/mp/src/public/mathlib
  FORCE_INCLUDE +=
  ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
  ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m32 -O2 -fPIC -m32 -msse -std=gnu++0x
  ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m32 -O2 -fPIC -m32 -msse -std=gnu++0x
  ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
  LIBS += -ltier0 -lvstdlib
  LDDEPS +=
  ALL_LDFLAGS += $(LDFLAGS) -L/usr/lib32 -L../../source-sdk-2013/mp/src/lib/public/linux32 -m32 -shared -Wl,-soname=gmsv_navigation_linux.dll -s -l:mathlib.a -l:tier1.a
  LINKCMD = $(CXX) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
all: prebuild prelink $(TARGET)
	@:

endif

ifeq ($(config),client)
  RESCOMP = windres
  TARGETDIR = ../bin
  TARGET = $(TARGETDIR)/gmcl_navigation_linux.dll
  OBJDIR = obj/Client
  DEFINES += -DGMMODULE -DRAD_TELEMETRY_DISABLED -DNO_MALLOC_OVERRIDE -D_LINUX -DLINUX -D_POSIX -DPOSIX -DGNUC
  INCLUDES += -I../../source-sdk-2013/mp/src/lib/public -I../../source-sdk-2013/mp/src/lib/common -I../../source-sdk-2013/mp/src/public -I../../source-sdk-2013/mp/src/public/tier0 -I../../source-sdk-2013/mp/src/public/tier1 -I../../source-sdk-2013/mp/src/common -I../../garrysmod_common/include -I../../garrysmod_common/include/garrysmod/lua -I.. -I../../source-sdk-2013/mp/src/thirdparty/SDL2 -I../../source-sdk-2013/mp/src/public/mathlib
  FORCE_INCLUDE +=
  ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
  ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m32 -O2 -fPIC -m32 -msse -std=gnu++0x
  ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m32 -O2 -fPIC -m32 -msse -std=gnu++0x
  ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
  LIBS += -ltier0 -lvstdlib
  LDDEPS +=
  ALL_LDFLAGS += $(LDFLAGS) -L/usr/lib32 -L../../source-sdk-2013/mp/src/lib/public/linux32 -m32 -shared -Wl,-soname=gmcl_navigation_linux.dll -s -l:mathlib.a -l:tier1.a
  LINKCMD = $(CXX) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
all: prebuild prelink $(TARGET)
	@:

endif

OBJECTS := \
	$(OBJDIR)/kdtree.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/nav.o \
	$(OBJDIR)/node.o \

RESOURCES := \

CUSTOMFILES := \

SHELLTYPE := msdos
ifeq (,$(ComSpec)$(COMSPEC))
  SHELLTYPE := posix
endif
ifeq (/bin,$(findstring /bin,$(SHELL)))
  SHELLTYPE := posix
endif

$(TARGET): $(GCH) ${CUSTOMFILES} $(OBJECTS) $(LDDEPS) $(RESOURCES)
	@echo Linking navigation
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

clean:
	@echo Cleaning navigation
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild:
	$(PREBUILDCMDS)

prelink:
	$(PRELINKCMDS)

ifneq (,$(PCH))
$(OBJECTS): $(GCH) $(PCH)
$(GCH): $(PCH)
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif
	$(SILENT) $(CXX) -x c++-header $(ALL_CXXFLAGS) -o "$@" -MF "$(@:%.gch=%.d)" -c "$<"
endif

$(OBJDIR)/kdtree.o: ../navigation/sources/kdtree.c
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif
	$(SILENT) $(CC) $(ALL_CFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/main.o: ../navigation/sources/main.cpp
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/nav.o: ../navigation/sources/nav.cpp
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/node.o: ../navigation/sources/node.cpp
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

-include $(OBJECTS:%.o=%.d)
ifneq (,$(PCH))
  -include $(OBJDIR)/$(notdir $(PCH)).d
endif