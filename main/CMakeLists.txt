set(srcs
        "main.c"
        "db_esp32_control.c"
        "msp_ltm_serial.c"
        "http_server.c"
        "db_esp32_comm.c"
        "db_comm.c"
        "db_crc.c"
        "tcp_server.c" )

if(CONFIG_OLED_ENABLE)
        list(APPEND srcs "db_display.cpp")
endif()

idf_component_register(
        SRCS "${srcs}"
        INCLUDE_DIRS . )