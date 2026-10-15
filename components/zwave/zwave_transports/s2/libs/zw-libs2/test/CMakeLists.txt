add_compile_definitions(
  UNIT_TEST
)

set(LIBS2_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_zpc_gtest(NAME test_aes SRCS test_aes.cpp LIBS aes INCLUDES ${LIBS2_INCLUDE})

add_zpc_gtest(NAME test_aes_cmac SRCS test_aes_cmac.cpp LIBS aes INCLUDES ${LIBS2_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})

add_zpc_gtest(NAME test_curve25519 SRCS test_curve25519.cpp LIBS s2crypto aes INCLUDES ${LIBS2_INCLUDE})

add_zpc_gtest(
  NAME test_ccm
  SRCS test_ccm.cpp ../crypto/ccm/ccm.c ../crypto/aes/aes.c
  INCLUDES ${LIBS2_INCLUDE})

add_zpc_gtest(
  NAME test_ctr_dbrg
  SRCS test_ctr_dbrg.cpp ../crypto/ctr_drbg/ctr_drbg.c ../crypto/aes/aes.c
  INCLUDES ${LIBS2_INCLUDE})
target_compile_definitions(test_ctr_dbrg PRIVATE RANDLEN=64)

add_zpc_gtest(
  NAME test_kderiv
  SRCS test_kderiv.cpp ../crypto/kderiv/kderiv.c ../crypto/aes-cmac/aes_cmac.c ../crypto/aes/aes.c
  INCLUDES ${LIBS2_INCLUDE})
