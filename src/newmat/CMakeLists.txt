# newmat target #########################################################

add_library(newmat OBJECT 
    bandmat.cpp
    cholesky.cpp
	evalue.cpp
    fft.cpp
    hholder.cpp
	jacobi.cpp
    myexcept.cpp
    newfft.cpp
    newmat1.cpp
	newmat2.cpp
    newmat3.cpp
	newmat4.cpp
	newmat5.cpp
	newmat6.cpp
	newmat7.cpp
    newmat8.cpp
	newmat9.cpp
    newmatex.cpp
    newmatnl.cpp
	newmatrm.cpp	
	solution.cpp
	sort.cpp
	submat.cpp
	svd.cpp
)

target_include_directories(newmat PUBLIC ../../src/include)

# allow linking of newmat into shared libraries
set_property(TARGET newmat PROPERTY POSITION_INDEPENDENT_CODE ON)
