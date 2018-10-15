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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/4db70e6c/jsoncpp.o \
	${OBJECTDIR}/BMA220.o \
	${OBJECTDIR}/LISD3H.o \
	${OBJECTDIR}/I2CBus.o \
	${OBJECTDIR}/OscController.o \
	${OBJECTDIR}/dmx/DominoState.o \
	${OBJECTDIR}/dmx/DominoController.o \
	${OBJECTDIR}/dmx/enttecdmxusb.o \
	${OBJECTDIR}/dmx/rs232.o \
	${OBJECTDIR}/ip/IpEndpointName.o \
	${OBJECTDIR}/ip/posix/NetworkingUtils.o \
	${OBJECTDIR}/ip/posix/UdpSocket.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/osc/OscOutboundPacketStream.o \
	${OBJECTDIR}/osc/OscPrintReceivedElements.o \
	${OBJECTDIR}/osc/OscReceivedElements.o \
	${OBJECTDIR}/osc/OscTypes.o


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
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/domino ${OBJECTFILES} ${LDLIBSOPTIONS} -lwiringPi -pthread -lusb

${OBJECTDIR}/_ext/4db70e6c/jsoncpp.o: ../jsoncpp/jsoncpp.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/4db70e6c
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/4db70e6c/jsoncpp.o ../jsoncpp/jsoncpp.cpp

${OBJECTDIR}/BMA220.o: BMA220.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/BMA220.o BMA220.cpp

${OBJECTDIR}/LISD3H.o: LISD3H.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/LISD3H.o LISD3H.cpp

${OBJECTDIR}/I2CBus.o: I2CBus.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/I2CBus.o I2CBus.cpp

${OBJECTDIR}/OscController.o: OscController.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/OscController.o OscController.cpp

${OBJECTDIR}/dmx/DominoState.o: dmx/DominoState.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/DominoState.o dmx/DominoState.cpp

${OBJECTDIR}/dmx/DominoController.o: dmx/DominoController.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/DominoController.o dmx/DominoController.cpp

${OBJECTDIR}/dmx/enttecdmxusb.o: dmx/enttecdmxusb.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/enttecdmxusb.o dmx/enttecdmxusb.cpp

${OBJECTDIR}/dmx/rs232.o: dmx/rs232.cpp
	${MKDIR} -p ${OBJECTDIR}/dmx
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dmx/rs232.o dmx/rs232.cpp

${OBJECTDIR}/ip/IpEndpointName.o: ip/IpEndpointName.cpp
	${MKDIR} -p ${OBJECTDIR}/ip
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ip/IpEndpointName.o ip/IpEndpointName.cpp

${OBJECTDIR}/ip/posix/NetworkingUtils.o: ip/posix/NetworkingUtils.cpp
	${MKDIR} -p ${OBJECTDIR}/ip/posix
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ip/posix/NetworkingUtils.o ip/posix/NetworkingUtils.cpp

${OBJECTDIR}/ip/posix/UdpSocket.o: ip/posix/UdpSocket.cpp
	${MKDIR} -p ${OBJECTDIR}/ip/posix
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ip/posix/UdpSocket.o ip/posix/UdpSocket.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/osc/OscOutboundPacketStream.o: osc/OscOutboundPacketStream.cpp
	${MKDIR} -p ${OBJECTDIR}/osc
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/osc/OscOutboundPacketStream.o osc/OscOutboundPacketStream.cpp

${OBJECTDIR}/osc/OscPrintReceivedElements.o: osc/OscPrintReceivedElements.cpp
	${MKDIR} -p ${OBJECTDIR}/osc
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/osc/OscPrintReceivedElements.o osc/OscPrintReceivedElements.cpp

${OBJECTDIR}/osc/OscReceivedElements.o: osc/OscReceivedElements.cpp
	${MKDIR} -p ${OBJECTDIR}/osc
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/osc/OscReceivedElements.o osc/OscReceivedElements.cpp

${OBJECTDIR}/osc/OscTypes.o: osc/OscTypes.cpp
	${MKDIR} -p ${OBJECTDIR}/osc
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -I../jsoncpp -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/osc/OscTypes.o osc/OscTypes.cpp

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
