include_directories(..)
add_executable( unittests basic.c)
target_link_libraries(unittests runtime PkgConfig::IGRAPH PkgConfig::GLIB)


