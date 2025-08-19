import powermake

def on_build(config: powermake.Config):
    config.add_flags("-Wall", "-Wextra")

    config.add_includedirs("./")

    files = powermake.get_files("**/*.c",
        "**/*.cpp", "**/*.cc", "**/*.C",
        "**/*.asm", "**/*.s", "**/*.rc")

    objects = powermake.compile_files(config, files)

    config.add_shared_libs("turbojpeg", "jpeg")

    powermake.link_files(config, objects)

powermake.run("my_project", build_callback=on_build)
