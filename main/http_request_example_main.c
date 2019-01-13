/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "stdbool.h"
#include "string.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/*sd card library*/
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID Hai Dotcom
#define EXAMPLE_WIFI_PASS doremonnobita

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

xQueueHandle http_task2_queue_handle = 0;
xSemaphoreHandle task2_signal = 0;

/*Prototype*/
void sd_card_init();
static void http_get_task_1();

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "host767.000webhostapp.com"
#define WEB_PORT 80
#define WEB_URL "https://host767.000webhostapp.com/data/hcm_fine_arts_museum/phuoc_long_operation/english/sound/phuoc_long_operation.wav"
static const char *TAG1 = "Task1";
static const char *TAG2 = "Task2";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
    			.sta = {
    					.ssid = "Hai Dotcom",
    					.password = "doremonnobita",
    					.bssid_set = 0
    			},
    		};
    ESP_LOGI(TAG1, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void http_get_task_1()
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];
    static bool begin_write_file;
    int pos_begin = 0;

    //while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
    	/* H:Wait for BIT0 (CONNECTEC_BIT)
    	 * 	Clear on exit = TRUE - parameter will be cleared in the event group before xEventGroupWaitBits() returns
    me	 *	xWaitForAllBits = FALSE -If xWaitForAllBits is set to pdFALSE then xEventGroupWaitBits() will return when
    	 *	any of the bits set
    	 */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG1, "Connected to AP");

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG1, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            //continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG1, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG1, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            //continue;
        }
        ESP_LOGI(TAG1, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG1, "... socket connect failed errno=%d", err);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            //continue;
        }

        ESP_LOGI(TAG1, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG1, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            //continue;
        }
        ESP_LOGI(TAG1, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG1, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            //continue;
        }
        ESP_LOGI(TAG1, "... set socket receiving timeout success");

        /* Read HTTP response */
        printf("Read HTTP response \n");
        FILE *f = fopen("/sdcard/ask1.wav","w");
        if(f == NULL){
          	printf("Failed to open file \n");
        }else
           	printf("Open file \n");
        int count = 0;
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            if(strstr(recv_buf,"RIFF") != NULL){
            	begin_write_file = true;
            	pos_begin = (int)strstr(recv_buf,"RIFF")- (int)recv_buf;
            }
            if(begin_write_file == true){
            	for(int i = 0; i<r; i++){
            		if(i >= pos_begin){
            	       fputc(recv_buf[i],f);
            	       pos_begin = 0; //mean after check where RIFF begin, start writing at first byte
            	     }
   	            }
            }

            printf("\n downloading ... %d \n",count++);

        } while(r > 0);
        fclose(f);
        ESP_LOGI(TAG1, "... done reading from socket. Last read return=%d errno=%d\r\n", r, err);
        close(s);
        char ch;
/*        for(int countdown = 3; countdown >= 0; countdown--) {
            ESP_LOGI(TAG1, "%d... ", countdown);
            if(xSemaphoreTake(task2_signal,portMAX_DELAY)){
            	ch ='i';
            	//ESP_LOGI(TAG1,"%c\n",ch);
            	if(!xQueueSend(http_task2_queue_handle,&ch,portMAX_DELAY)){
            		printf("Failed to send \n");
            	}
            }
        }
        ESP_LOGI(TAG1, "Starting again!");*/
        while(1){

        }
    //}
}

static void http_get_task_2(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];
    char  * rec_ch;
    rec_ch = (char*)calloc(32,sizeof(char));
    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
    	/* H:Wait for BIT0 (CONNECTEC_BIT)
    	 * 	Clear on exit = TRUE - parameter will be cleared in the event group before xEventGroupWaitBits() returns
    me	 *	xWaitForAllBits = FALSE -If xWaitForAllBits is set to pdFALSE then xEventGroupWaitBits() will return when
    	 *	any of the bits set
    	 */
    	if(xQueueReceive(http_task2_queue_handle,&rec_ch,portMAX_DELAY)){
    		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
    		                            false, true, portMAX_DELAY);
    		        ESP_LOGI(TAG2, "Connected to AP");
    		        //ESP_LOGI(TAG2,"%c\n",rec_ch);
    		        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    		        if(err != 0 || res == NULL) {
    		            ESP_LOGE(TAG2, "DNS lookup failed err=%d res=%p", err, res);
    		            vTaskDelay(1000 / portTICK_PERIOD_MS);
    		            continue;
    		        }

    		        /* Code to print the resolved IP.

    		           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    		        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    		        ESP_LOGI(TAG2, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    		        s = socket(res->ai_family, res->ai_socktype, 0);
    		        if(s < 0) {
    		            ESP_LOGE(TAG2, "... Failed to allocate socket.");
    		            freeaddrinfo(res);
    		            vTaskDelay(1000 / portTICK_PERIOD_MS);
    		            //continue;
    		        }
    		        ESP_LOGI(TAG2, "... allocated socket");

    		        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
    		            ESP_LOGE(TAG2, "... socket connect failed errno=%d", err);
    		            close(s);
    		            freeaddrinfo(res);
    		            vTaskDelay(4000 / portTICK_PERIOD_MS);
    		            continue;
    		        }

    		        ESP_LOGI(TAG2, "... connected");
    		        freeaddrinfo(res);

    		        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
    		            ESP_LOGE(TAG2, "... socket send failed");
    		            close(s);
    		            vTaskDelay(4000 / portTICK_PERIOD_MS);
    		            continue;
    		        }
    		        ESP_LOGI(TAG2, "... socket send success");

    		        struct timeval receiving_timeout;
    		        receiving_timeout.tv_sec = 5;
    		        receiving_timeout.tv_usec = 0;
    		        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
    		                sizeof(receiving_timeout)) < 0) {
    		            ESP_LOGE(TAG2, "... failed to set socket receiving timeout");
    		            close(s);
    		            vTaskDelay(4000 / portTICK_PERIOD_MS);
    		            continue;
    		        }
    		        ESP_LOGI(TAG2, "... set socket receiving timeout success");

    		        /* Read HTTP response */
    		        static int i =0;
    		        static char name[32];
    		        sprintf(name,"/sdcard/%d.txt",i++);
    		        ESP_LOGI(TAG2,"Read http response %s",name);
    		        FILE *f = fopen(name,"w");
    		        if(f == NULL){
    		          	ESP_LOGI(TAG2,"Failed to open file \n");
    		        }else
    		        	ESP_LOGI(TAG2,"open file \n");;
    		        int count = 0;
    		        do {
    		            bzero(recv_buf, sizeof(recv_buf));
    		            r = read(s, recv_buf, sizeof(recv_buf)-1);
    		            for(int i = 0; i<r; i++){
    		            	fputc(recv_buf[i],f);
    		            }
    		            ESP_LOGI(TAG2,"\n downloading ... %d",count++);

    		        } while(r > 0);
    		        fclose(f);
    		        ESP_LOGI(TAG2, "... done reading from socket. Last read return=%d errno=%d\r\n", r, err);
    		        close(s);
    		        //free(rec_ch);
    		        xSemaphoreGive(task2_signal);
    	}

    }
}

void sd_card_init()
{
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
	// Internal pull-ups are not sufficient. However, enabling internal pull-ups
	// does make a difference some boards, so we do that here.

	gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
	gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
	gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
	gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
	gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
	        .format_if_mount_failed = false,
	        .max_files = 5,
	        .allocation_unit_size = 16 * 1024
	    };
	// Use settings defined above to initialize SD card and mount FAT filesystem.
	    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
	    // Please check its source code and implement error recovery when developing
	    // production applications.
	    sdmmc_card_t* card;
	    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

	    if (ret != ESP_OK) {
	        if (ret == ESP_FAIL) {
	            printf( "Failed to mount filesystem.\n");
	        } else {
	            printf("Failed to initialize the card (%d).\n",ret);
	        }
	    }
	    // Card has been initialized, print its properties
	    sdmmc_card_print_info(stdout, card);
	    printf("SD card init \n");

}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    sd_card_init();
    http_task2_queue_handle = xQueueCreate(2,sizeof(char));
    vSemaphoreCreateBinary(task2_signal);
    xTaskCreate(&http_get_task_1, "http_get_task_1", 2048, NULL, 2, NULL);
    //xTaskCreate(&http_get_task_2, "http_get_task_2", 2048, NULL, 1, NULL);

}
