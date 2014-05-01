PROJECT_NAME = autonomous_control
COMPILER = gcc

INCLUDE_DIR = ../include
TEST_DIR = ../test
SRC_DIR = ../src
EXECUTABLE_NAME = autonomous_control

SRC_FILES = ${SRC_DIR}/timer.c ${SRC_DIR}/open_serial_port.c ${SRC_DIR}/open_usb_serial_port.c
TEST_FILES = ${TEST_DIR}/main.c 
INCLUDE_FILES = ${INCLUDE_DIR}/timer.h


${EXECUTABLE_NAME} : ${INCLUDE_FILES} ${SRC_FILES} ${TEST_FILES}
	${COMPILER} -I ${INCLUDE_DIR} ${SRC_FILES} ${TEST_FILES} -o ${EXECUTABLE_NAME}