deps_config := \
	/home/hai_dotcom/esp/esp-idf/components/app_trace/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/aws_iot/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/bt/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/esp32/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/ethernet/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/fatfs/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/freertos/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/heap/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/libsodium/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/log/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/lwip/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/mbedtls/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/openssl/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/pthread/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/spi_flash/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/spiffs/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/hai_dotcom/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/hai_dotcom/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/hai_dotcom/Project/project_zer0_esp32/Workspace_ZERO/http_request/main/Kconfig.projbuild \
	/home/hai_dotcom/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/hai_dotcom/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
