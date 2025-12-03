set(
        gazprea_statements_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Assignment.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Block.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Break.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Conditional.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Continue.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Declaration.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructDeclaration.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IdentifierLeft.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Input.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IteratorLoop.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Loop.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Output.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ProcedureCall.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Return.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleElementAssign.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/StructElementAssign.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArrayElementAssign.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleUnpackAssign.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Typealias.cpp"
)

target_sources(gazc PRIVATE ${gazprea_statements_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")
