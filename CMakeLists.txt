cmake_minimum_required (VERSION 3.13)

project(I3CAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
	src/I3CAnalyzer.cpp
	src/I3CAnalyzer.h
	src/I3CAnalyzerResults.cpp
	src/I3CAnalyzerResults.h
	src/I3CAnalyzerSettings.cpp
	src/I3CAnalyzerSettings.h
	src/I3CSimulationDataGenerator.cpp
	src/I3CSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
