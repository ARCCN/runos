{ stdenv, fetchFromGitHub, cmake }:

stdenv.mkDerivation rec {
  name = "boost165";
  version = "1.65.1";

  src = fetchFromGitHub {
    owner = "boostorg";
    repo = "boost";
    rev = "436ad1dfcfc7e0246141beddd11c8a4e9c10b146";
    # sha256 = "0nwf2a6f80nnh41gplia3p1axzznw7zj9dwlxzq992nrx0dy6cjq";
    sha256 = "0lzk91dkn4v4aqma9r56bcjhfdj8k46ca8agzhdlqqfxqjyznzal";
    fetchSubmodules = true;
  };

  configureScript = ''
    echo ":::::::::::::::::::::::::::::: iconfigure Script boost-165 ::::::::::::::"
  '';

  buildPhase = ''
    echo ":::::::::::::::::::::::::::::: build phase boost-165 ::::::::::::::::::::"
    ./bootstrap.sh
    ./b2 --includedir=$prefix --libdir=$out/lib -j$NIX_BUILD_CORES --layout=system variant=release threading=multi link=shared runtime-link=shared debug-symbols=off toolset=gcc --without-python
  '';

  installPhase = ''
    echo ":::::::::::::::::::::::::::::: install phase boost-165 :::::::::::::::::::"
    for ENTRY in $(ls -d libs/*); do
      if [[ -d $ENTRY/conversion ]]  
        then echo $ENTRY && cp -r $ENTRY/conversion/include/ $prefix; 
      fi
      if [ -d $ENTRY/interval ]
        then echo $ENTRY && cp -r $ENTRY/interval/include/ $prefix; 
      fi
      if [ -d $ENTRY/odeint ]
        then echo $ENTRY && cp -r $ENTRY/odeint/include/ $prefix; 
      fi
      if [ -d $ENTRY/ublas ]
        then echo $ENTRY && cp -r $ENTRY/ublas/include/ $prefix; 
      fi

      if [ -d $ENTRY/include ]
        then echo $ENTRY && cp -r $ENTRY/include/ $prefix; 
        else echo $ENTRY 
      fi
    done

    ./b2 --includedir=$prefix --libdir=$out/lib -j$NIX_BUILD_CORES --layout=system variant=release threading=multi link=shared runtime-link=shared debug-symbols=off toolset=gcc --without-python install
  '';

  enableParallelBuilding = true;
}
