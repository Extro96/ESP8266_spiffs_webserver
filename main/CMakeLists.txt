set(COMPONENT_SRCS  "main.c" 
                    "../lib/src/customSpiffs.c"
                    )

set(COMPONENT_ADD_INCLUDEDIRS   "." 
                                "../lib/inc"
                                )
					
register_component()
#spiffs_create_partition_image(storage data FLASH_IN_PROJECT)