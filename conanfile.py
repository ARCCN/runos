from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.scm import Git
from conan.tools.files import copy
from pathlib import Path
import os

class RunOS( ConanFile ):
    name = "runos"
    version = "2.0"
    vendor = "arccn"
    settings = {"os", "compiler", "build_type", "arch"}
    generators = "CMakeDeps"
    package_type = "application"

    exports_sources = [ "cmake/*", 
                        "doc/*", 
                        "etc/*", 
                        "include/*", 
                        "src/*", 
                        "tools/*", 
                        "web/*", 
                        "CMakeLists.txt", 
                        "LICENSE", 
                        "NOTICE", 
                        "README.md", 
                        "default-settings.json", 
                        "runos-settings.json" ]

    def requirements(self):
        self.requires( "fmt/10.2.1" )
        self.requires( "spdlog/1.14.1" )
        self.requires( "backward-cpp/1.6" )
        self.requires( "boost/1.69.0", options={"boost/*:pch": False })
        self.requires( "editline/3.1" )
        self.requires( "cpp-netlib/0.13.0" )
        self.requires( "libevent/2.1.12" )
        self.requires( "libfluid_base/1.0.1" )
        self.requires( "libfluid_msg/1.0.1" )
        self.requires( "libtins/4.5" )
        self.requires( "range-v3/0.3.7" )
        self.requires( "tiny-process-library/1.0.5" )
        self.requires( "qt/5.15.7", options = { 
            "qt/*:openssl": False,
            "qt/*:with_pcre2": True,
            "qt/*:with_doubleconversion": True,
            "qt/*:with_fontconfig": False,
            "qt/*:with_freetype": False,
            "qt/*:with_icu": False,
            "qt/*:with_libjpeg": False,
            "qt/*:with_libpng": False,
            "qt/*:with_sqlite3": False,
            "qt/*:with_pq": False,
            "qt/*:with_odbc": False,
            "qt/*:with_openal": False,
            "qt/*:with_zstd": False,
            "qt/*:with_md4c": False,
            "qt/*:with_x11": False,
            "qt/*:gui": False,
            "qt/*:widgets": False })

    def build_requirements(self):
        if int( self.settings_build.get_safe("compiler.version")) < 12:
            self.tool_requires("gcc/12.2.0", run=True )
        if int( self.settings_build.get_safe("compiler.version")) > 15:
            self.tool_requires("gcc/15.2.0", run=True )
        self.tool_requires("cmake/3.26.3", run=True )

        self.tool_requires("nodejs/20.16.0", run=True )
        self.tool_requires("uglifyjs/3.19.2", run=True )
        # self.test_requires("gtest/1.13.0")

    def layout(self):
        self.folders.source="."
        self.folders.build="build"
        self.folders.generators="build/cmake"

    def source( self ):
        print( "Current folder is used as 'source_folder'" )
        '''
          TODO: may need get source 'git clone <repository>':
            git = Git(self)
            link = self.conan_data["source"]["url"]
            source_version = self.conan_data["versions"][self.version]
            git.clone(url=link, target=self.name)

            git.folder = self.name 
            git.checkout(commit=source_version)
        '''

    def generate(self):
        tc = CMakeToolchain(self)
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_FUTURE"] = True
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION"] = True
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_SHARED_FUTURE_CONTINUATION"] = True
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY"] = True
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_FUTURE_UNWRAP"] = True
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET"] = True
        tc.preprocessor_definitions["BOOST_THREAD_PROVIDES_EXECUTORS"] = True
        '''
          TODO: may need define some new Boost preprocessor flag as:
            tc.preprocessor_definitions["BOOST_THREAD_USES_MOVE"] = ""

            In order to implement Movable classes, move parameters and return types Boost.
            Thread uses the rvalue reference when the compiler support it. On compilers not 
            supporting it Boost.Thread uses either the emulation provided by Boost.Move or the 
            emulation provided by the previous versions of Boost.
            Thread depending whether BOOST_THREAD_USES_MOVE is defined or not. 
            This macros is unset by default when BOOST_THREAD_VERSION is 2. 
            Since BOOST_THREAD_VERSION 3, BOOST_THREAD_USES_MOVE is defined.
            https://www.boost.org/doc/libs/1_58_0/doc/html/thread/emulations.html
        '''

        tc.generate()

    def build( self ):
        src_dir = os.path.join( self.source_folder, "src")
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package( self ):
        copy(self, "runos", self.build_folder, os.path.join(self.package_folder, "bin"))
        copy(self, "web/*", self.build_folder, os.path.join(self.package_folder, "bin"))

        copy(self, "etc/*", self.source_folder, self.package_folder)
        copy(self, "tools/*", self.source_folder, self.package_folder)
        copy(self, "web/*", self.source_folder, self.package_folder)

        copy(self, "runos-settings.json", self.source_folder, self.package_folder)
        copy(self, "default-settings.json", self.source_folder, self.package_folder)

        copy(self, "NOTICE", self.source_folder, self.package_folder)
        copy(self, "LICENSE", self.source_folder, self.package_folder)

    def deploy(self):
        copy(self, "*", src=self.package_folder, dst="." )
        '''
          TODO: need to set deploy folder for "conan install" command
            $ conan install --deployer=direct_deploy  --requires=runos/2.1
        '''

    def package_info( self ):
        pass
        '''
          TODO: may need add any package info same as:
            # Binaries to link
            self.cpp_info.libs = []  # The libs to link against
            self.cpp_info.system_libs = []  # System libs to link against
          or
            # Directories
            self.cpp_info.includedirs = ['include']  # Ordered list of include paths
            self.cpp_info.libdirs = ['lib']  # Directories where libraries can be found
            self.cpp_info.bindirs = ['bin']  # Directories where executables and shared libs can be found

          https://docs.conan.io/2/reference/conanfile/methods/package_info.html#cpp-info-library-and-build-information
        '''
