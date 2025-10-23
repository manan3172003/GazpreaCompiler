set(
        gazprea_walkers_src
        "${CMAKE_CURRENT_SOURCE_DIR}/AstBuilder.cpp"
)

target_sources(gazc PRIVATE ${gazprea_walkers_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")