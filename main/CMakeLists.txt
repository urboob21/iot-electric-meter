idf_component_register(SRCS "LCD20x4.c" "userGPIOs.c" "wifiApplication.c" "httpServer.c" "main.c" "MQTT.c" "nvs.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js)	
                    target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")				