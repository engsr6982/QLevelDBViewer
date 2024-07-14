add_rules("mode.release")

-- This Qt app:
--    Windows:
--      MSVC:
--          xmake f -c --qt=C:\Qt\6.7.2\msvc2019_64


add_requires(
    "nlohmann_json 3.11.3",
    "fmt 10.2.1",
    "leveldb 1.23"
)


if not has_config("vs_runtime") then 
    set_runtimes("MD")
end 


target("QLevelDBViewer")
    add_cxflags(
        "/utf-8"
    )
    add_defines(
        "NOMINMAX",
        "UNICODE"
    )
    set_symbols("debug")

    add_rules("qt.widgetapp")

    add_files(
        "src/**.cpp",
        "src/**.h",
        "src/**.ui",
        "src/**.qrc",
        "src/**.rc"
    )
    add_includedirs("src")
    add_headerfiles("src/*.h")

    add_packages(
        "nlohmann_json",
        "fmt",
        "leveldb"
    )
