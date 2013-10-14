find_package(MySQL)

set(BACKEND_NAME gmysqlbackend)
set(${BACKEND_NAME}_SRCS
  gmysqlbackend.cc
  gmysqlbackend.hh
)
add_backend(${BACKEND_NAME})
target_link_libraries(${BACKEND_NAME} ${MYSQL_LIBRARIES})
