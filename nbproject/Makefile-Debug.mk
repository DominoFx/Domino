#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/4db70e6c/jsoncpp.o \
	${OBJECTDIR}/dmx/DmxController.o \
	${OBJECTDIR}/dmx/enttecdmxusb.o \
	${OBJECTDIR}/dmx/rs232.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=/usr/local/lib/libboost_system.so.1.62.0

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/domino

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/domino: /usr/local/lib/libboost_system.so.1.62.0

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/domino: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/domino ${OBJECTFILES} ${LDLIBSOPTIONS} -lwiringPi -pthread

${OBJECTDIR}/_ext/4db70e6c/jsoncpp.o: ../jsoncpp/jsoncpp.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/4db70e6c
	${RM} "$@.d"
	$(COMPILE.cc) -g -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/4db70e6c/jsoncpp.o ../jsoncpp/jsoncpp.cpp

${OBJECTDIR}/dmx/DmxController.o: dmx/DmxController.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -g -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/DmxController.o dmx/DmxController.cpp

${OBJECTDIR}/dmx/enttecdmxusb.o: dmx/enttecdmxusb.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -g -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/enttecdmxusb.o dmx/enttecdmxusb.cpp

${OBJECTDIR}/dmx/rs232.o: dmx/rs232.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -g -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/rs232.o dmx/rs232.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libboost_system.so.1.62.0
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/domino

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc