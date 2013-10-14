find_package(PostgreSQL)
include_directories(${PostgreSQL_INCLUDE_DIR})

set(BACKEND_NAME gpgsqlbackend)
set(${BACKEND_NAME}_SRCS
  gpgsqlbackend.cc
  gpgsqlbackend.hh
)
add_backend(${BACKEND_NAME})
target_link_libraries(${BACKEND_NAME} ${PostgreSQL_LIBRARIES})
