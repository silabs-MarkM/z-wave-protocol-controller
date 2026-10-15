add_compile_definitions(
  UNIT_TEST
)

add_zpc_gtest(
  NAME test_inclusion_joining_node_slave
  SRCS
    test_inclusion_joining_node.cpp
    libs2_gmock.cpp
    ../s2_inclusion.c
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
