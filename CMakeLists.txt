project(smart_factory CXX)
cmake_minimum_required(VERSION 3.3)
#enable_testing()

#set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/Users/cbchoi/Desktop/DDSAgent/OpenDDS/")

find_package(OpenDDS REQUIRED)

# Make sure the MPC-generated headers are gone so the CMake build will use the
# right ones. This is not needed in a real project.
file(GLOB headers "*.h")
file(GLOB listener_header "DataReaderListenerImpl.h")
list(REMOVE_ITEM headers ${listener_header})
list(LENGTH headers header_count)
if(header_count GREATER 0)
  file(REMOVE ${headers})
endif()

INCLUDE_DIRECTORIES( include )
LINK_DIRECTORIES( lib )

# IDL TypeSupport Library
add_library(dds_agent_idl)
OPENDDS_TARGET_SOURCES(dds_agent_idl PUBLIC "ControlProtocol.idl")
target_link_libraries(dds_agent_idl PUBLIC OpenDDS::Dcps)

set(opendds_libs
  OpenDDS::Dcps # Core OpenDDS Library
  OpenDDS::InfoRepoDiscovery OpenDDS::Tcp # For run_test.pl
  OpenDDS::Rtps OpenDDS::Rtps_Udp # For run_test.pl --rtps
  dds_agent_idl
  libzmq-v142-mt-4_3_4
)

# Publisher
add_executable(DDSPubAgent
  DDSPubAgent.cpp
)
target_link_libraries(DDSPubAgent ${opendds_libs})

# Sublisher
add_executable(DDSSubAgent
  DDSSubAgent.cpp
  DataReaderListenerImpl.cpp
)
target_link_libraries(DDSSubAgent ${opendds_libs})