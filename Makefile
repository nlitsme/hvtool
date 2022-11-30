CMAKEARGS+=$(if $(D),-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_BUILD_TYPE=Release)
CMAKEARGS+=$(if $(COV),-DOPT_COV=1)
CMAKEARGS+=$(if $(PROF),-DOPT_PROF=1)
CMAKEARGS+=$(if $(LIBCXX),-DOPT_LIBCXX=1)
CMAKEARGS+=$(if $(STLDEBUG),-DOPT_STL_DEBUGGING=1)
CMAKEARGS+=$(if $(SANITIZE),-DOPT_SANITIZE=1)
CMAKEARGS+=$(if $(ANALYZE),-DOPT_ANALYZE=1)

all:
	cmake -B build . $(CMAKEARGS)
	cmake --build build $(if $(V),--verbose)

VC_CMAKE=C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe
vc:
	"$(VC_CMAKE)" -G"Visual Studio 16 2019" -B build . $(CMAKEARGS)
	"$(VC_CMAKE)" --build build $(if $(V),--verbose)

nmake:
	"$(VC_CMAKE)" -G"NMake Makefiles" -B build . $(CMAKEARGS)
	"$(VC_CMAKE)" --build build $(if $(V),--verbose)

llvm:
	CC=clang CXX=clang++ cmake -B build . $(CMAKEARGS)
	cmake --build build $(if $(V),--verbose)

SCANBUILD=$(firstword $(wildcard /usr/bin/scan-build*))
llvmscan:
	CC=clang CXX=clang++ cmake -B build . $(CMAKEARGS)
	$(SCANBUILD) cmake --build build $(if $(V),--verbose)

clean:
	$(RM) -r build CMakeFiles CMakeCache.txt CMakeOutput.log
