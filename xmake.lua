
package("bootlin_armv7_eabihf")
    set_kind("toolchain")
    -- target: armv7l, glibc 2.28, libstdc++ 6.0.25, kernel 4.14.206-perf
    add_urls("https://toolchains.bootlin.com/downloads/releases/toolchains/armv7-eabihf/tarballs/armv7-eabihf--glibc--bleeding-edge-2018.11-1.tar.bz2")    
    add_versions("glibc-bleeding-edge-2018.11-1", "81963e17289395d28cdecde0a1e42deb4bbb672f54c00c02e716eb05d6e560bc")
    on_install(function (package)
        os.cp("*", package:installdir())
    end)
package_end()

toolchain("bootlin_armv7")
    set_kind("cross")

    set_toolset("cc", "arm-buildroot-linux-gnueabihf-gcc")
    set_toolset("cxx", "arm-buildroot-linux-gnueabihf-g++")
    set_toolset("ld", "arm-buildroot-linux-gnueabihf-g++", "arm-buildroot-linux-gnueabihf-gcc")
    set_toolset("sh", "arm-buildroot-linux-gnueabihf-g++", "arm-buildroot-linux-gnueabihf-gcc")
    set_toolset("ar", "arm-buildroot-linux-gnueabihf-ar")
    set_toolset("ex", "arm-buildroot-linux-gnueabihf-ar")
    set_toolset("strip", "arm-buildroot-linux-gnueabihf-strip")
    set_toolset("mm", "arm-buildroot-linux-gnueabihf-gcc")
    set_toolset("mxx", "arm-buildroot-linux-gnueabihf-g++")
    set_toolset("as", "arm-buildroot-linux-gnueabihf-as")

    on_load(function (toolchain)
        import("core.project.project")
        local pack = project.required_package("bootlin_armv7_eabihf")
        if pack then
            toolchain:set("sdkdir", pack:installdir())
            toolchain:set("bindir", path.join(pack:installdir(), "bin"))
        end
        -- cpu features: half thumb fastmult vfp edsp neon vfpv3 tls vfpv4 idiva idivt vfpd32 lpae evtstrm
        toolchain:add("cxflags", "-march=armv7-a", "-mfloat-abi=hard", "-mfpu=neon-vfpv4", {force = true})
    end)
toolchain_end()

package("curl_headers")
    set_kind("library", {headeronly = true})
    -- libcurl.so.4.0.5
    add_urls("https://curl.se/download/curl-7.61.1.tar.gz")
    add_versions("7.61.1", "eaa812e9a871ea10dbe8e1d3f8f12a64a8e3e62aeab18cb23742e2f1727458ae")
    on_install(function (package)
        os.cp("include/*", package:installdir("include"))
    end)
package_end()

add_requires("bootlin_armv7_eabihf glibc-bleeding-edge-2018.11-1")
add_requires("curl_headers 7.61.1")

add_rules("mode.debug", "mode.release")

target("sms-forward")
    set_kind("binary")
    set_languages("gnu17")
    set_arch("armv7")
    add_files("src/*.c")
    add_files("src/*/*.c")
    add_includedirs("src")

    set_toolchains("@bootlin_armv7")

    add_packages("curl_headers")
    add_linkdirs("third/lib")
    add_ldflags("-Wl,-rpath-link,third/lib", "-Wl,--as-needed", {force = true})
    add_links("curl", "meigapps", "meigbase", "pthread")
