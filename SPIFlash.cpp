#include "esp_log.h"

#include "SPIFlash.h"

static const char* TAG = "spi_flash";

esp_err_t SPIFlash::initSPIbus() {
    spiBusConfig.miso_io_num = SPI3_IOMUX_PIN_NUM_MISO;
    spiBusConfig.mosi_io_num = SPI3_IOMUX_PIN_NUM_MOSI;
    spiBusConfig.sclk_io_num = SPI3_IOMUX_PIN_NUM_CLK;
    spiBusConfig.quadwp_io_num = -1;
    spiBusConfig.quadhd_io_num = -1;

    return spi_bus_initialize(spiHost, &spiBusConfig, SPI_DMA_CH_AUTO);
}

esp_err_t SPIFlash::addFlashDevice() {
    flashConfig = {
        .host_id = spiHost,
        .cs_io_num = SPI3_IOMUX_PIN_NUM_CS,
        .io_mode = SPI_FLASH_DIO,
        .speed = ESP_FLASH_80MHZ,
        .input_delay_ns = 0,
        .cs_id = 0,
    };

    return spi_bus_add_flash_device(&device, &flashConfig);
}

bool SPIFlash::registerPartition(const char* label, size_t size) {
    // TODO: automatic offset
    esp_err_t ret = esp_partition_register_external(
        device, 0, size, label, ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS, &partition
    );

    ESP_LOGI(TAG, "Successfully registered storage partition");
    ESP_LOGI(TAG, "part_label:  %s", partition->label);
    ESP_LOGI(TAG, "offset:      0x%x", partition->address);
    ESP_LOGI(TAG, "size:        0x%x", partition->size);

    return (ret == ESP_OK);
}

bool SPIFlash::install() {
    ESP_LOGI(TAG, "Initializing SPI flash");

    info.status = STORAGE_DEVICE_OFFLINE;
    info.type = STORAGE_DEVICE_TYPE_UNKNOWN;
    info.capacity = 0;

    esp_err_t ret;
    spiHost = SPI3_HOST;

    ret = initSPIbus();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus for SPI flash, error: %s", esp_err_to_name(ret));
        return false;
    }

    ret = addFlashDevice();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI bus SPI flash, error: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_flash_init(device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI flash, error: %s", esp_err_to_name(ret));
        return false;
    }

    uint32_t flash_id;
    esp_flash_read_id(device, &flash_id);

    info.status = STORAGE_DEVICE_ONLINE;
    info.type = STORAGE_DEVICE_TYPE_FLASH;
    info.capacity = device->size;

    return esp_flash_chip_driver_initialized(device);
}

bool SPIFlash::uninstall() {
    return true;
}