set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
					
register_component()
spiffs_create_partition_image(storage data FLASH_IN_PROJECT)