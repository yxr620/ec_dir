
add_custom_target(build_isal
WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/isa-l
BYPRODUCTS lib/isa-l/bin/*.o
COMMAND pwd; echo "\n";
COMMAND make -f ./Makefile.unx)

add_custom_target(clean_isal
WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/isa-l
BYPRODUCTS lib/isa-l/bin/*.o
COMMAND pwd; echo "\n";
COMMAND make -f Makefile.unx clean)
