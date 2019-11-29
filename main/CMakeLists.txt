idf_component_register(
        SRCS 
                main.c 
                db_esp32_control.c  
                msp_ltm_serial.c
                http_server.c
                db_esp32_comm.c
                db_comm.c
                db_crc.c
                tcp_server.c
                db_display.cpp
        INCLUDE_DIRS .)

spiffs_create_partition_image(www html FLASH_IN_PROJECT)