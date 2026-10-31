# Writes names of functions of a PS4 game library to .prx.sym, see cmake/PS4.cmake
# cmake -DNM=llvm-nm -DINPUT=library.so -DOUTPUT=library.prx.sym -P PS4MakeSymbols.cmake

execute_process(COMMAND "${NM}" --defined-only -n "${INPUT}"
	OUTPUT_VARIABLE symbols RESULT_VARIABLE result)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "${NM} failed on ${INPUT}")
endif()

# "address type name" for functions: T and t, W and w for weak ones
string(REGEX MATCHALL "[0-9a-fA-F]+ [TtWw] [^\n]+" lines "${symbols}")
set(out "")
foreach(line ${lines})
	string(REGEX REPLACE "^([0-9a-fA-F]+) [TtWw] (.+)$" "\\1 \\2\n" line "${line}")
	string(APPEND out "${line}")
endforeach()
file(WRITE "${OUTPUT}" "${out}")
