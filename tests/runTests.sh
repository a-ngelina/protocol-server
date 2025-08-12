#!/bin/bash

TEMP_DIR=$(mktemp -d)

for test_file in *.cpp; do
	g++ -std=c++20 ${test_file} -DDEBUG -o ${TEMP_DIR}/${test_file%.cpp}
	echo "Running test" ${test_file%.cpp}
	${TEMP_DIR}/${test_file%.cpp}
	echo "Done"
done

rm -rf ${TEMP_DIR}
