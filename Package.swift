// swift-tools-version:5.1
import PackageDescription

#if os(Linux)
let libraryType: PackageDescription.Product.Library.LibraryType = .dynamic
#else
let libraryType: PackageDescription.Product.Library.LibraryType = .static
#endif

let package = Package(
    name: "HalfLifeSDK",
    products: [
        .library(
            name: "HalfLifeServer",
            type: libraryType,
            targets: [
                "HalfLifeServer",
                "HalfLifePlayerMove"
            ]
        ),
        .library(
            name: "HalfLifeClient",
            type: libraryType,
            targets: [
                "HalfLifeClient",
                "HalfLifePlayerMove"
            ]
        )
    ],
    targets: [
        .target(
            name: "HalfLifeServer",
            dependencies: [],
            path: "./dlls",
            exclude: [
                "./build",
                "./cmake"
            ],
            cSettings: [
                .define("GOLDSOURCE_SUPPORT"),
                .define("LINUX"),
                .define("_LINUX"),
                .define("HAVE_TGMATH_H", to: "1"),
                .define("HAVE_CMATH", to: "1"),
                .define("CLIENT_WEAPONS", to: "1"),
                .define("NO_VOICEGAMEMGR"),
                .define("stricmp", to: "strcasecmp"),
                .define("strnicmp", to: "strncasecmp"),
                .define("_snprintf", to: "snprintf"),
                .define("_vsnprintf", to: "vsnprintf"),
                .headerSearchPath("./"),
                .headerSearchPath("./../dlls"),
                .headerSearchPath("./../common"),
                .headerSearchPath("./../engine"),
                .headerSearchPath("./../public"),
                .headerSearchPath("./../pm_shared"),
                .headerSearchPath("./../game_shared"),
                .headerSearchPath("./../utils/false_vgui/include")
            ]
        ),
        .target(
            name: "HalfLifeClient",
            dependencies: [],
            path: "./cl_dll",
            exclude: [
                "./build",
                "./cmake"
            ],
            cSettings: [
                .define("GOLDSOURCE_SUPPORT"),
                .define("LINUX"),
                .define("_LINUX"),
                .define("HAVE_TGMATH_H", to: "1"),
                .define("HAVE_CMATH", to: "1"),
                .define("CLIENT_WEAPONS", to: "1"),
                .define("CLIENT_DLL", to: "1"),
                .define("NO_VOICEGAMEMGR"),
                .define("stricmp", to: "strcasecmp"),
                .define("strnicmp", to: "strncasecmp"),
                .define("_snprintf", to: "snprintf"),
                .define("_vsnprintf", to: "vsnprintf"),
                .headerSearchPath("./"),
                .headerSearchPath("./../common"),
                .headerSearchPath("./../engine"),
                .headerSearchPath("./../public"),
                .headerSearchPath("./../pm_shared"),
                .headerSearchPath("./../game_shared"),
                .headerSearchPath("./../dlls"),
                .headerSearchPath("./../utils/false_vgui/include")
            ]
        ),
        .target(
            name: "HalfLifePlayerMove",
            dependencies: [],
            path: "./pm_shared",
            cSettings: [
                .define("GOLDSOURCE_SUPPORT"),
                .define("LINUX"),
                .define("_LINUX"),
                .define("stricmp", to: "strcasecmp"),
                .define("strnicmp", to: "strncasecmp"),
                .define("_snprintf", to: "snprintf"),
                .define("_vsnprintf", to: "vsnprintf"),
                .headerSearchPath("./"),
                .headerSearchPath("./../common"),
                .headerSearchPath("./../engine"),
                .headerSearchPath("./../public"),
                .headerSearchPath("./../pm_shared"),
                .headerSearchPath("./../game_shared"),
                .headerSearchPath("./../dlls"),
                .headerSearchPath("./../utils/false_vgui/include")
            ]
        )
    ]
)
