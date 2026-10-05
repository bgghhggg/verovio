// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "VerovioToolkit",
    platforms: [
        .iOS(.v16),
        .macOS(.v11)
    ],
    products: [
        .library(
            name: "VerovioToolkit",
            targets: ["VerovioToolkit"]
        )
    ],
    targets: [
        .target(
            name: "VerovioCore",
            path: ".",
            // Humdrum's library is about half of Verovio's code and one of
            // its slowest files to build; nothing here reads Humdrum.
            exclude: ["src/hum"],
            sources: [
                "src",
                "libmei/dist",
                "libmei/addons",
                "tools/c_wrapper.cpp"
            ],
            publicHeadersPath: "bindings/swift-core",
            cxxSettings: [
                // As Verovio's CMake release builds: asserts compiled out,
                // no runtime clock, and only the importers these apps use
                // (MEI and MusicXML; MIDI and timemap output are kept).
                .define("NDEBUG", .when(configuration: .release)),
                .define("NO_RUNTIME"),
                .define("NO_HUMDRUM_SUPPORT"),
                .define("NO_ABC_SUPPORT"),
                .define("NO_PAE_SUPPORT"),
                .define("NO_DARMS_SUPPORT"),
                .define("NO_GABC_SUPPORT"),
                .headerSearchPath("include/crc"),
                .headerSearchPath("include/hum"),
                .headerSearchPath("include/json"),
                .headerSearchPath("include/midi"),
                .headerSearchPath("include/pugi"),
                .headerSearchPath("include/tuning-library"),
                .headerSearchPath("include/utf8"),
                .headerSearchPath("include/vrv"),
                .headerSearchPath("include/zip"),
                .headerSearchPath("libmei/dist"),
                .headerSearchPath("libmei/addons"),
                .unsafeFlags(["-std=c++23"])
            ]
        ),
        .target(
            name: "VerovioToolkit",
            dependencies: ["VerovioCore"],
            path: ".",
            sources: ["bindings/swift-toolkit"],
            resources: [.copy("data")]
        )
    ]
)
