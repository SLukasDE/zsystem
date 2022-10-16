cmake_minimum_required(VERSION 3.23)

project(zsystem VERSION 1.5.0
            LANGUAGES CXX)

#include(FetchContent)
#FetchContent_Declare(
#  catch
#  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#  GIT_TAG        v2.13.6
#)
#FetchContent_MakeAvailable(catch)


# The compiled library code is here
#add_subdirectory(src/main)
# STATIC, SHARED, MODULE, INTERFACE
add_library(zsystem STATIC
	./src/main/zsystem/Backtrace.cpp
	./src/main/zsystem/SignalHandler.cpp
	./src/main/zsystem/Process.cpp
	./src/main/zsystem/SharedMemory.cpp
	./src/main/zsystem/Signal.h
	./src/main/zsystem/Signal.cpp
	./src/main/zsystem/SharedMemory.h
	./src/main/zsystem/SignalHandler.h
	./src/main/zsystem/process/ProducerStatic.h
	./src/main/zsystem/process/ProducerDynamic.h
	./src/main/zsystem/process/FileDescriptor.h
	./src/main/zsystem/process/Arguments.h
	./src/main/zsystem/process/ProducerDynamic.cpp
	./src/main/zsystem/process/Environment.h
	./src/main/zsystem/process/ProducerStatic.cpp
	./src/main/zsystem/process/ConsumerFile.cpp
	./src/main/zsystem/process/FeatureProcess.cpp
	./src/main/zsystem/process/FeatureProcess.h
	./src/main/zsystem/process/FileDescriptor.cpp
	./src/main/zsystem/process/FeatureTime.h
	./src/main/zsystem/process/ProducerFile.h
	./src/main/zsystem/process/Environment.cpp
	./src/main/zsystem/process/Consumer.h
	./src/main/zsystem/process/Feature.h
	./src/main/zsystem/process/FeatureTime.cpp
	./src/main/zsystem/process/Producer.h
	./src/main/zsystem/process/ProducerFile.cpp
	./src/main/zsystem/process/Arguments.cpp
	./src/main/zsystem/process/ConsumerFile.h
	./src/main/zsystem/Backtrace.h
	./src/main/zsystem/Process.h
	./src/main/zsystem/Logger.h
)

target_include_directories(zsystem PUBLIC ./src/main)
target_compile_features(zsystem PUBLIC cxx_std_11)
set_property(TARGET zsystem PROPERTY POSITION_INDEPENDENT_CODE ON)

install(TARGETS zsystem
        EXPORT zsystem-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

install(EXPORT      zsystem-export
        FILE        zsystemTargets.cmake
        NAMESPACE   zsystem::
        DESTINATION lib/cmake
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/zsystemConfig.cmake
        DESTINATION "lib/cmake"
)

