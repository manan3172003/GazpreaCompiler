set(
        gazprea_types_src
        "${CMAKE_CURRENT_SOURCE_DIR}/IntegerTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RealTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/CharacterTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BooleanTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleTypeAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_types_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")