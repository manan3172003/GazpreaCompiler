set(
        gazprea_types_src
        "${CMAKE_CURRENT_SOURCE_DIR}/IntegerTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RealTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/CharacterTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BooleanTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AliasTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArrayTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/VectorTypeAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructTypeAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_types_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")