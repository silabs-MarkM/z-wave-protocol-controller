add_compile_definitions(
  UNIT_TEST
)

add_zpc_gtest(
  NAME test_span
  SRCS
    test_span.cpp
    ../S2.c
    ../../inclusion/s2_inclusion.c
  LIBS s2crypto aes
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inclusion
)
