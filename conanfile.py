from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import load
import os


class RinMotionDanceConan(ConanFile):
    """RinMotionDance - behavioral reconstruction of MikuMikuDance v932 x64.

    Package management via Conan 2.  The only third-party link-time
    dependency of the original binary is Bullet Physics 2.75 (statically
    linked); it is provided by the local recipe in ``recipes/bullet275``.
    DirectX 9 and the system Win32 APIs are not supplied by this recipe.
    D3DX is a required normal DLL import selected by the build; parity evidence and
    remaining limitations are recorded in docs/PORTING_STATUS.md.
    """

    name = "rinmotiondance"
    exports = "VERSION"

    def set_version(self):
        self.version = load(self, os.path.join(self.recipe_folder, "VERSION")).strip()
    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "bullet275/2.75",
    )

    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "exports/*",
        "docs/*",
    )

    generators = ("CMakeDeps", "CMakeToolchain")

    def layout(self):
        cmake_layout(self)

    def validate(self):
        if self.info.settings.arch == "x86":
            self.output.warning(
                "The behavioral reference is MikuMikuDanceE_v932x64; "
                "use profiles/x64 for parity work."
            )

    def build(self):
        pass
