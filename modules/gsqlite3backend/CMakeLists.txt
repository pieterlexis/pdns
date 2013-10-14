pkg_check_modules(SQLITE3 sqlite3)

set(BACKEND_NAME gsqlite3backend)
set(${BACKEND_NAME}_SRCS
  gsqlite3backend.cc
  gsqlite3backend.hh
)
add_backend(${BACKEND_NAME})
target_link_libraries(${BACKEND_NAME} ${SQLITE3_LIBRARIES})
