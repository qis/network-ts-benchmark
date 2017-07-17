# https://github.com/chriskohlhoff/asio/commit/524288cb4fcf84664b3dc39cb4424c7509969b92
# Version: 1.11.0 (Git)

if(NOT TARGET asio)
  add_library(asio INTERFACE IMPORTED)
  set_target_properties(asio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/include"
    INTERFACE_COMPILE_DEFINITIONS "ASIO_STANDALONE=1"
    INTERFACE_LINK_LIBRARIES "$<$<PLATFORM_ID:Windows>:ws2_32>")
endif()
