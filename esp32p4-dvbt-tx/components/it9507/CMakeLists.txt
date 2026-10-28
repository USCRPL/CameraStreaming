idf_component_register(
    SRCS
        "src/it9507_usb.c"
        "src/it9507_core.c"
    INCLUDE_DIRS
        "include"            # public API: it9507.h, it9507_regs.h
    PRIV_INCLUDE_DIRS
        "priv_include"       # internal: it9507_private.h, it9507_firmware.h
    REQUIRES
        usb                  # ESP-IDF USB Host Library
        freertos
        esp_common
)

# Define this once you have dropped the 4560-byte firmware blob into
# priv_include/it9507_firmware.h (see that file and the README).
# target_compile_definitions(${COMPONENT_LIB} PRIVATE IT9507_HAVE_FIRMWARE=1)
