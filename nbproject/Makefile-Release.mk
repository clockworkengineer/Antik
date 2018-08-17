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
	${OBJECTDIR}/classes/CApprise.o \
	${OBJECTDIR}/classes/CCurl.o \
	${OBJECTDIR}/classes/CFTP.o \
	${OBJECTDIR}/classes/CFile.o \
	${OBJECTDIR}/classes/CIMAP.o \
	${OBJECTDIR}/classes/CIMAPBodyStruct.o \
	${OBJECTDIR}/classes/CIMAPParse.o \
	${OBJECTDIR}/classes/CMIME.o \
	${OBJECTDIR}/classes/CPath.o \
	${OBJECTDIR}/classes/CRedirect.o \
	${OBJECTDIR}/classes/CSCP.o \
	${OBJECTDIR}/classes/CSFTP.o \
	${OBJECTDIR}/classes/CSMTP.o \
	${OBJECTDIR}/classes/CSSHChannel.o \
	${OBJECTDIR}/classes/CSSHSession.o \
	${OBJECTDIR}/classes/CSocket.o \
	${OBJECTDIR}/classes/CTask.o \
	${OBJECTDIR}/classes/CZIP.o \
	${OBJECTDIR}/classes/CZIPIO.o \
	${OBJECTDIR}/classes/implementation/CFileEventNotifier.o \
	${OBJECTDIR}/utility/FTPUtil.o \
	${OBJECTDIR}/utility/SCPUtil.o \
	${OBJECTDIR}/utility/SFTPUtil.o \
	${OBJECTDIR}/utility/SSHChannelUtil.o \
	${OBJECTDIR}/utility/SSHSessionUtil.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1 \
	${TESTDIR}/TestFiles/f2 \
	${TESTDIR}/TestFiles/f3 \
	${TESTDIR}/TestFiles/f4 \
	${TESTDIR}/TestFiles/f5 \
	${TESTDIR}/TestFiles/f6

# Test Object Files
TESTOBJECTFILES= \
	${TESTDIR}/tests/ITCApprise.o \
	${TESTDIR}/tests/UTCFile.o \
	${TESTDIR}/tests/UTCIMAPParse.o \
	${TESTDIR}/tests/UTCPath.o \
	${TESTDIR}/tests/UTCSMTP.o \
	${TESTDIR}/tests/UTCTask.o

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
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libantik.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libantik.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libantik.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libantik.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libantik.a

${OBJECTDIR}/classes/CApprise.o: classes/CApprise.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CApprise.o classes/CApprise.cpp

${OBJECTDIR}/classes/CCurl.o: classes/CCurl.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CCurl.o classes/CCurl.cpp

${OBJECTDIR}/classes/CFTP.o: classes/CFTP.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CFTP.o classes/CFTP.cpp

${OBJECTDIR}/classes/CFile.o: classes/CFile.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CFile.o classes/CFile.cpp

${OBJECTDIR}/classes/CIMAP.o: classes/CIMAP.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CIMAP.o classes/CIMAP.cpp

${OBJECTDIR}/classes/CIMAPBodyStruct.o: classes/CIMAPBodyStruct.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CIMAPBodyStruct.o classes/CIMAPBodyStruct.cpp

${OBJECTDIR}/classes/CIMAPParse.o: classes/CIMAPParse.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CIMAPParse.o classes/CIMAPParse.cpp

${OBJECTDIR}/classes/CMIME.o: classes/CMIME.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CMIME.o classes/CMIME.cpp

${OBJECTDIR}/classes/CPath.o: classes/CPath.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CPath.o classes/CPath.cpp

${OBJECTDIR}/classes/CRedirect.o: classes/CRedirect.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CRedirect.o classes/CRedirect.cpp

${OBJECTDIR}/classes/CSCP.o: classes/CSCP.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSCP.o classes/CSCP.cpp

${OBJECTDIR}/classes/CSFTP.o: classes/CSFTP.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSFTP.o classes/CSFTP.cpp

${OBJECTDIR}/classes/CSMTP.o: classes/CSMTP.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSMTP.o classes/CSMTP.cpp

${OBJECTDIR}/classes/CSSHChannel.o: classes/CSSHChannel.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSSHChannel.o classes/CSSHChannel.cpp

${OBJECTDIR}/classes/CSSHSession.o: classes/CSSHSession.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSSHSession.o classes/CSSHSession.cpp

${OBJECTDIR}/classes/CSocket.o: classes/CSocket.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSocket.o classes/CSocket.cpp

${OBJECTDIR}/classes/CTask.o: classes/CTask.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CTask.o classes/CTask.cpp

${OBJECTDIR}/classes/CZIP.o: classes/CZIP.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CZIP.o classes/CZIP.cpp

${OBJECTDIR}/classes/CZIPIO.o: classes/CZIPIO.cpp
	${MKDIR} -p ${OBJECTDIR}/classes
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CZIPIO.o classes/CZIPIO.cpp

${OBJECTDIR}/classes/implementation/CFileEventNotifier.o: classes/implementation/CFileEventNotifier.cpp
	${MKDIR} -p ${OBJECTDIR}/classes/implementation
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/implementation/CFileEventNotifier.o classes/implementation/CFileEventNotifier.cpp

${OBJECTDIR}/utility/FTPUtil.o: utility/FTPUtil.cpp
	${MKDIR} -p ${OBJECTDIR}/utility
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/FTPUtil.o utility/FTPUtil.cpp

${OBJECTDIR}/utility/SCPUtil.o: utility/SCPUtil.cpp
	${MKDIR} -p ${OBJECTDIR}/utility
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SCPUtil.o utility/SCPUtil.cpp

${OBJECTDIR}/utility/SFTPUtil.o: utility/SFTPUtil.cpp
	${MKDIR} -p ${OBJECTDIR}/utility
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SFTPUtil.o utility/SFTPUtil.cpp

${OBJECTDIR}/utility/SSHChannelUtil.o: utility/SSHChannelUtil.cpp
	${MKDIR} -p ${OBJECTDIR}/utility
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SSHChannelUtil.o utility/SSHChannelUtil.cpp

${OBJECTDIR}/utility/SSHSessionUtil.o: utility/SSHSessionUtil.cpp
	${MKDIR} -p ${OBJECTDIR}/utility
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SSHSessionUtil.o utility/SSHSessionUtil.cpp

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-tests-subprojects .build-conf ${TESTFILES}
.build-tests-subprojects:

${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/ITCApprise.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f2: ${TESTDIR}/tests/UTCFile.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f2 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f3: ${TESTDIR}/tests/UTCIMAPParse.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f3 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f4: ${TESTDIR}/tests/UTCPath.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f4 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f5: ${TESTDIR}/tests/UTCSMTP.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f5 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f6: ${TESTDIR}/tests/UTCTask.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f6 $^ ${LDLIBSOPTIONS}   


${TESTDIR}/tests/ITCApprise.o: tests/ITCApprise.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/ITCApprise.o tests/ITCApprise.cpp


${TESTDIR}/tests/UTCFile.o: tests/UTCFile.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/UTCFile.o tests/UTCFile.cpp


${TESTDIR}/tests/UTCIMAPParse.o: tests/UTCIMAPParse.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/UTCIMAPParse.o tests/UTCIMAPParse.cpp


${TESTDIR}/tests/UTCPath.o: tests/UTCPath.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/UTCPath.o tests/UTCPath.cpp


${TESTDIR}/tests/UTCSMTP.o: tests/UTCSMTP.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/UTCSMTP.o tests/UTCSMTP.cpp


${TESTDIR}/tests/UTCTask.o: tests/UTCTask.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/UTCTask.o tests/UTCTask.cpp


${OBJECTDIR}/classes/CApprise_nomain.o: ${OBJECTDIR}/classes/CApprise.o classes/CApprise.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CApprise.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CApprise_nomain.o classes/CApprise.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CApprise.o ${OBJECTDIR}/classes/CApprise_nomain.o;\
	fi

${OBJECTDIR}/classes/CCurl_nomain.o: ${OBJECTDIR}/classes/CCurl.o classes/CCurl.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CCurl.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CCurl_nomain.o classes/CCurl.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CCurl.o ${OBJECTDIR}/classes/CCurl_nomain.o;\
	fi

${OBJECTDIR}/classes/CFTP_nomain.o: ${OBJECTDIR}/classes/CFTP.o classes/CFTP.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CFTP.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CFTP_nomain.o classes/CFTP.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CFTP.o ${OBJECTDIR}/classes/CFTP_nomain.o;\
	fi

${OBJECTDIR}/classes/CFile_nomain.o: ${OBJECTDIR}/classes/CFile.o classes/CFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CFile.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CFile_nomain.o classes/CFile.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CFile.o ${OBJECTDIR}/classes/CFile_nomain.o;\
	fi

${OBJECTDIR}/classes/CIMAP_nomain.o: ${OBJECTDIR}/classes/CIMAP.o classes/CIMAP.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CIMAP.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CIMAP_nomain.o classes/CIMAP.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CIMAP.o ${OBJECTDIR}/classes/CIMAP_nomain.o;\
	fi

${OBJECTDIR}/classes/CIMAPBodyStruct_nomain.o: ${OBJECTDIR}/classes/CIMAPBodyStruct.o classes/CIMAPBodyStruct.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CIMAPBodyStruct.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CIMAPBodyStruct_nomain.o classes/CIMAPBodyStruct.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CIMAPBodyStruct.o ${OBJECTDIR}/classes/CIMAPBodyStruct_nomain.o;\
	fi

${OBJECTDIR}/classes/CIMAPParse_nomain.o: ${OBJECTDIR}/classes/CIMAPParse.o classes/CIMAPParse.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CIMAPParse.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CIMAPParse_nomain.o classes/CIMAPParse.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CIMAPParse.o ${OBJECTDIR}/classes/CIMAPParse_nomain.o;\
	fi

${OBJECTDIR}/classes/CMIME_nomain.o: ${OBJECTDIR}/classes/CMIME.o classes/CMIME.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CMIME.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CMIME_nomain.o classes/CMIME.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CMIME.o ${OBJECTDIR}/classes/CMIME_nomain.o;\
	fi

${OBJECTDIR}/classes/CPath_nomain.o: ${OBJECTDIR}/classes/CPath.o classes/CPath.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CPath.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CPath_nomain.o classes/CPath.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CPath.o ${OBJECTDIR}/classes/CPath_nomain.o;\
	fi

${OBJECTDIR}/classes/CRedirect_nomain.o: ${OBJECTDIR}/classes/CRedirect.o classes/CRedirect.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CRedirect.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CRedirect_nomain.o classes/CRedirect.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CRedirect.o ${OBJECTDIR}/classes/CRedirect_nomain.o;\
	fi

${OBJECTDIR}/classes/CSCP_nomain.o: ${OBJECTDIR}/classes/CSCP.o classes/CSCP.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CSCP.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSCP_nomain.o classes/CSCP.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CSCP.o ${OBJECTDIR}/classes/CSCP_nomain.o;\
	fi

${OBJECTDIR}/classes/CSFTP_nomain.o: ${OBJECTDIR}/classes/CSFTP.o classes/CSFTP.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CSFTP.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSFTP_nomain.o classes/CSFTP.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CSFTP.o ${OBJECTDIR}/classes/CSFTP_nomain.o;\
	fi

${OBJECTDIR}/classes/CSMTP_nomain.o: ${OBJECTDIR}/classes/CSMTP.o classes/CSMTP.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CSMTP.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSMTP_nomain.o classes/CSMTP.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CSMTP.o ${OBJECTDIR}/classes/CSMTP_nomain.o;\
	fi

${OBJECTDIR}/classes/CSSHChannel_nomain.o: ${OBJECTDIR}/classes/CSSHChannel.o classes/CSSHChannel.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CSSHChannel.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSSHChannel_nomain.o classes/CSSHChannel.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CSSHChannel.o ${OBJECTDIR}/classes/CSSHChannel_nomain.o;\
	fi

${OBJECTDIR}/classes/CSSHSession_nomain.o: ${OBJECTDIR}/classes/CSSHSession.o classes/CSSHSession.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CSSHSession.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSSHSession_nomain.o classes/CSSHSession.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CSSHSession.o ${OBJECTDIR}/classes/CSSHSession_nomain.o;\
	fi

${OBJECTDIR}/classes/CSocket_nomain.o: ${OBJECTDIR}/classes/CSocket.o classes/CSocket.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CSocket.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CSocket_nomain.o classes/CSocket.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CSocket.o ${OBJECTDIR}/classes/CSocket_nomain.o;\
	fi

${OBJECTDIR}/classes/CTask_nomain.o: ${OBJECTDIR}/classes/CTask.o classes/CTask.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CTask.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CTask_nomain.o classes/CTask.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CTask.o ${OBJECTDIR}/classes/CTask_nomain.o;\
	fi

${OBJECTDIR}/classes/CZIP_nomain.o: ${OBJECTDIR}/classes/CZIP.o classes/CZIP.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CZIP.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CZIP_nomain.o classes/CZIP.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CZIP.o ${OBJECTDIR}/classes/CZIP_nomain.o;\
	fi

${OBJECTDIR}/classes/CZIPIO_nomain.o: ${OBJECTDIR}/classes/CZIPIO.o classes/CZIPIO.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/CZIPIO.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/CZIPIO_nomain.o classes/CZIPIO.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/CZIPIO.o ${OBJECTDIR}/classes/CZIPIO_nomain.o;\
	fi

${OBJECTDIR}/classes/implementation/CFileEventNotifier_nomain.o: ${OBJECTDIR}/classes/implementation/CFileEventNotifier.o classes/implementation/CFileEventNotifier.cpp 
	${MKDIR} -p ${OBJECTDIR}/classes/implementation
	@NMOUTPUT=`${NM} ${OBJECTDIR}/classes/implementation/CFileEventNotifier.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/classes/implementation/CFileEventNotifier_nomain.o classes/implementation/CFileEventNotifier.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/classes/implementation/CFileEventNotifier.o ${OBJECTDIR}/classes/implementation/CFileEventNotifier_nomain.o;\
	fi

${OBJECTDIR}/utility/FTPUtil_nomain.o: ${OBJECTDIR}/utility/FTPUtil.o utility/FTPUtil.cpp 
	${MKDIR} -p ${OBJECTDIR}/utility
	@NMOUTPUT=`${NM} ${OBJECTDIR}/utility/FTPUtil.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/FTPUtil_nomain.o utility/FTPUtil.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/utility/FTPUtil.o ${OBJECTDIR}/utility/FTPUtil_nomain.o;\
	fi

${OBJECTDIR}/utility/SCPUtil_nomain.o: ${OBJECTDIR}/utility/SCPUtil.o utility/SCPUtil.cpp 
	${MKDIR} -p ${OBJECTDIR}/utility
	@NMOUTPUT=`${NM} ${OBJECTDIR}/utility/SCPUtil.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SCPUtil_nomain.o utility/SCPUtil.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/utility/SCPUtil.o ${OBJECTDIR}/utility/SCPUtil_nomain.o;\
	fi

${OBJECTDIR}/utility/SFTPUtil_nomain.o: ${OBJECTDIR}/utility/SFTPUtil.o utility/SFTPUtil.cpp 
	${MKDIR} -p ${OBJECTDIR}/utility
	@NMOUTPUT=`${NM} ${OBJECTDIR}/utility/SFTPUtil.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SFTPUtil_nomain.o utility/SFTPUtil.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/utility/SFTPUtil.o ${OBJECTDIR}/utility/SFTPUtil_nomain.o;\
	fi

${OBJECTDIR}/utility/SSHChannelUtil_nomain.o: ${OBJECTDIR}/utility/SSHChannelUtil.o utility/SSHChannelUtil.cpp 
	${MKDIR} -p ${OBJECTDIR}/utility
	@NMOUTPUT=`${NM} ${OBJECTDIR}/utility/SSHChannelUtil.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SSHChannelUtil_nomain.o utility/SSHChannelUtil.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/utility/SSHChannelUtil.o ${OBJECTDIR}/utility/SSHChannelUtil_nomain.o;\
	fi

${OBJECTDIR}/utility/SSHSessionUtil_nomain.o: ${OBJECTDIR}/utility/SSHSessionUtil.o utility/SSHSessionUtil.cpp 
	${MKDIR} -p ${OBJECTDIR}/utility
	@NMOUTPUT=`${NM} ${OBJECTDIR}/utility/SSHSessionUtil.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/utility/SSHSessionUtil_nomain.o utility/SSHSessionUtil.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/utility/SSHSessionUtil.o ${OBJECTDIR}/utility/SSHSessionUtil_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	    ${TESTDIR}/TestFiles/f2 || true; \
	    ${TESTDIR}/TestFiles/f3 || true; \
	    ${TESTDIR}/TestFiles/f4 || true; \
	    ${TESTDIR}/TestFiles/f5 || true; \
	    ${TESTDIR}/TestFiles/f6 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
