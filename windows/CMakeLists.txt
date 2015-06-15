add_leatherman_includes("${Boost_INCLUDE_DIRS}")

leatherman_dependency(nowide)

add_leatherman_library(src/registry.cc)
add_leatherman_headers(inc/leatherman)
