DEBUG          := YES

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CXXFLAGS   := -Wall -Wno-format -g -DDEBUG
RELEASE_CXXFLAGS := -Wall -Wno-unknown-pragmas -Wno-format -O3

LIBS		 :=

DEBUG_LDFLAGS    := -g -lcurl -ljsoncpp
RELEASE_LDFLAGS  := -lcurl -ljsoncpp

ifeq (YES, ${DEBUG})
   CXXFLAGS     := ${DEBUG_CXXFLAGS}
   LDFLAGS      := ${DEBUG_LDFLAGS}
else
   CXXFLAGS     := ${RELEASE_CXXFLAGS}
   LDFLAGS      := ${RELEASE_LDFLAGS}
endif

INCS :=

OUTPUT := kodi-txupdate

all: ${OUTPUT}

SRCS := src/TinyXML/tinyxml.cpp src/TinyXML/tinyxmlparser.cpp src/TinyXML/tinystr.cpp src/TinyXML/tinyxmlerror.cpp \
src/HTTPUtils.cpp \
src/Langcodes.cpp \
src/CharsetUtils.cpp \
src/FileUtils.cpp \
src/Fileversioning.cpp \
src/Log.cpp \
src/POHandler.cpp \
src/ResourceHandler.cpp \
src/ProjectHandler.cpp \
src/ConfigHandler.cpp \
src/AddonXMLHandler.cpp \
$(OUTPUT)

OBJS := $(addsuffix .o,$(basename ${SRCS}))

${OUTPUT}: ${OBJS}
	${LD} -o $@ ${OBJS} ${LIBS} ${LDFLAGS}

%.o : %.cpp
	${CXX} -c ${CXXFLAGS} ${INCS} $< -o $@

dist:
	bash makedistlinux

clean:
	-rm -f core ${OBJS} ${OUTPUT}

tinyxml.o: tinyxml.h tinyxml.cpp tinystr.o tinyparser.o tinyxmlerror.o
tinyxmlparser.o: tinyxmlparser.cpp tinyxmlparser.h
tinyxmlerror.o: tinyxmlerror.cpp tinyxmlerror.h
tinystr.o: tinystr.cpp tinystr.h
CharsteUtils.o: CharsetUtils.h CharsetUtils.cpp Log.cpp Log.h
block_allocator.o: block_allocator.cpp block_allocator.h Log.cpp Log.h
FileUtils.o: FileUtils.h FileUtils.cpp Log.cpp Log.h
POHandler.o: POHandler.h POHandler.cpp Log.cpp Log.h
ResourceHandler.o: ResourceHandler.h ResourceHandler.cpp POHandler.h POHandler.cpp Log.cpp Log.h
ProjectHandler.o: ProjectHandler.h ProjectHandler.cpp ResourceHandler.h ResourceHandler.cpp Log.cpp Log.h
ConfigHandler.o: ConfigHandler.h Log.cpp Log.h tinyxml.o
HTTPUtils.o: HTTPUtils.h Log.h Log.cpp
Fileversioning.o: Fileversioning.h HTTPUtils.h HTTPUtils.cpp
AddonXMLHandler.cpp: AddonXMLHandler.h

install:
	mkdir -p ~/bin
	install -m 755 kodi-txupdate ~/bin/
uninstall:
	rm -rf ~/bin/kodi-txupdate
