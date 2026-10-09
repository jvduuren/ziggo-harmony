# PlatformIO post-build script: merge bootloader, partition table, boot_app0 and the app into
# one factory.bin that can be flashed at offset 0x0, e.g. from a browser with
# https://espressif.github.io/esptool-js/ (Chrome/Edge), no toolchain needed.
Import("env")  # noqa: F821 (provided by PlatformIO)


def make_factory_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    images = [(offset, env.subst(image)) for offset, image in env.get("FLASH_EXTRA_IMAGES", [])]
    images.append((env.subst("$ESP32_APP_OFFSET"), str(target[0])))
    flash_size = env.BoardConfig().get("upload.flash_size", "4MB")
    cmd = [
        env.subst("$PYTHONEXE"), env.subst("$OBJCOPY"), "--chip", env.BoardConfig().get("build.mcu"),
        "merge_bin", "-o", f"{build_dir}/factory.bin",
        "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", flash_size,
    ]
    for offset, image in images:
        cmd += [offset, image]
    env.Execute(" ".join(f'"{c}"' for c in cmd))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", make_factory_bin)  # noqa: F821
