set(
        gazprea_types_src
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleType.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/VectorType.cpp"
)

target_sources(gazc PRIVATE ${gazprea_types_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")
