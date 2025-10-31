set(
        gazprea_walkers_src
        "${CMAKE_CURRENT_SOURCE_DIR}/AstBuilder.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AstWalker.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/DefineWalker.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ResolveWalker.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ValidationWalker.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ValidationHelpers.cpp"
)

target_sources(gazc PRIVATE ${gazprea_walkers_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")