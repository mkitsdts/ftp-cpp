set_project("ftp")
set_version("1.0.0")
set_xmakever("2.7.9")
set_languages("cxx20")

target("ftp")
    set_kind("binary")
    add_includedirs("src")
    add_files("src/*.cpp")