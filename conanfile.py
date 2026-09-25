from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import cmake_layout, CMake, CMakeDeps, CMakeToolchain

class uLocalMagnitudeServiceConan(ConanFile):
   name = "uLocalMagnitudeService"
   #version = "0.0.1"
   license = "MIT"
   description = "Local (Richter) magnitude calculator service used at UUSS."
   url = "https://github.com/uofuseismo/uLocalMagnitude"
   settings = "os", "compiler", "build_type", "arch"
   options = {"build_tests" : [True, False],
              "build_training" : [True, False],
              "with_conan" : [True, False]}
   default_options = {"opentelemetry-cpp/*:with_otlp_http": "True",
                      "opentelemetry-cpp/*:with_otlp_grpc": "True",
                      "opentelemetry-cpp/*:with_abi_v2" : "True",
                      "spdlog/*:header_only" : "True",
                      "build_tests" : "True",
                      "build_training" : "False",
                      "with_conan" : "True",}
   export_sources = "CMakeLists.txt", "LICENSE", "README.md", "cmake/*", "src/*", "testing/*"
   generators = "CMakeDeps", "CMakeToolchain"

   def requirements(self):
       # dependencies
       self.requires("grpc/1.83.0")
       self.requires("opentelemetry-cpp/1.26.0")
       self.requires("protobuf/6.33.5")
       self.requires("boost/1.91.0")
       self.requires("spdlog/1.17.0")

   def build_requirements(self):
       # test dependncies and build tools
       self.test_requires("catch2/3.15.3")

   def layout(self):
       # defines the project layout
       cmake_layout(self)
 
   def build(self):
       # invokes the build system
       cmake = CMake(self)
       cmake.configure()
       cmake.build()
       #if can_run(self):
       #   # run tests particularly CTest 
       #   cmake.test()

   def test(self):
       if can_run(self):
          cmake.test()

   #def generate(self):
   #    tc = CMakeToolchain(self)
   #    tc.generate()

   def package(self):
       # copies files from the build to package folder
       cmake = CMake(self)
       cmake.install()

   def package_info(self):
       self.cpp_info.libs = ["uLocalMagnitudeService"]

