import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, rmdir


class OpensmileConan(ConanFile):
    name = "opensmile"
    version = "3.0.1"
    license = "audEERING Research License Agreement"
    url = "https://github.com/audeering/opensmile"
    description = "Speech and Music Interpretation by Large-space Extraction"
    topics = ("speech", "music", "feature extraction")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_libsvm": [True, False],
        "with_rnn": [True, False],
        "with_svmsmo": [True, False],
        "with_portaudio": [True, False],
        "with_ffmpeg": [True, False],
        "with_opensles": [True, False],
        "with_opencv": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_libsvm": True,
        "with_rnn": True,
        "with_svmsmo": True,
        "with_portaudio": False,
        "with_ffmpeg": False,
        "with_opensles": False,
        "with_opencv": False,
    }

    exports_sources = "src/*", "progsrc/*", "CMakeLists.txt", "LICENSE", "licenses/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self, src_folder=".")

    def generate(self):
        build_flags = ""
        build_flags += "-DBUILD_LIBSVM " if self.options.with_libsvm else ""
        build_flags += "-DBUILD_RNN " if self.options.with_rnn else ""
        build_flags += "-DBUILD_SVMSMO " if self.options.with_svmsmo else ""

        tc = CMakeToolchain(self)
        tc.variables["BUILD_FLAGS"] = build_flags
        tc.variables["WITH_PORTAUDIO"] = self.options.with_portaudio
        tc.variables["WITH_FFMPEG"] = self.options.with_ffmpeg
        tc.variables["WITH_OPENSLES"] = self.options.with_opensles
        tc.variables["WITH_OPENCV"] = self.options.with_opencv
        tc.variables["SMILEAPI_STATIC_LINK"] = not self.options.shared
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        # Remove CMake config files
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

        # Package licenses
        license_src_folder = os.path.join(self.source_folder, "licenses")
        license_dst_folder = os.path.join(self.package_folder, "licenses")
        copy(self, "LICENSE", src=self.source_folder, dst=license_dst_folder)
        copy(self, "newmat.txt", src=license_src_folder, dst=license_dst_folder)
        copy(self, "Rapidjson.txt", src=license_src_folder, dst=license_dst_folder)
        copy(self, "Speex.txt", src=license_src_folder, dst=license_dst_folder)
        if self.options.with_libsvm:
            copy(self, "LibSVM.txt", src=license_src_folder, dst=license_dst_folder)
        if self.options.with_portaudio:
            copy(self, "PortAudio.txt", src=license_src_folder, dst=license_dst_folder)

    def package_info(self):
        self.cpp_info.libs = ["SMILEapi", "opensmile"]

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.extend(["pthread", "m"])

        if self.settings.os == "Android":
            self.cpp_info.system_libs.append("log")
