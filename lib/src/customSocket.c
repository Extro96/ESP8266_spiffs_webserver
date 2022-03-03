#include "customSocket.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "customSocket";

static void tcp_server_task(void *pvParameters)
{
    int    i, len, err, on = 1;
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int desc_ready, end_server = FALSE;
    int ip_protocol;
    int close_conn;
    int max_sd, new_sd;
    struct fd_set master_set, working_set;

    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket created");

    /*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
    /*************************************************************/
    err = setsockopt(listen_sock, SOL_SOCKET,  SO_REUSEADDR,(char *)&on, sizeof(on));
    if (err < 0){
        ESP_LOGE(TAG, "Unable to make socket reusable: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket is reusable");

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    err = ioctl(listen_sock, FIONBIO, (char *)&on);
    if (err < 0){
        ESP_LOGE(TAG, "Unable to make socket nonblocking: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket is nonblocking");

    /*************************************************************/
    /* Bind the socket                                           */
    /*************************************************************/
    err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket binded");

    /*************************************************************/
    /* Set the listen back log                                   */
    /*************************************************************/
    err = listen(listen_sock, 32);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occured during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket listening");

    /*************************************************************/
    /* Initialize the master fd_set                              */
    /*************************************************************/
    FD_ZERO(&master_set);
    max_sd = listen_sock;
    FD_SET(listen_sock, &master_set);

    /*************************************************************/
    /* Loop waiting for incoming connects or for incoming data   */
    /* on any of the connected sockets.                          */
    /*************************************************************/
    do
    {
        /**********************************************************/
        /* Copy the master fd_set over to the working fd_set.     */
        /**********************************************************/
        memcpy(&working_set, &master_set, sizeof(master_set));

        /**********************************************************/
        /* Call select() and wait 3 minutes for it to complete.   */
        /**********************************************************/
        ESP_LOGE(TAG, "Waiting on select()...");
        err = select(max_sd + 1, &working_set, NULL, NULL, NULL);

        /**********************************************************/
        /* Check to see if the select call failed.                */
        /**********************************************************/
        if (err < 0)
        {
            ESP_LOGE(TAG, "select() fail");
            break;
        }

        /**********************************************************/
        /* One or more descriptors are readable.  Need to         */
        /* determine which ones they are.                         */
        /**********************************************************/
        desc_ready = err;
        for (i=0; i <= max_sd  &&  desc_ready > 0; ++i)
        {
            /*******************************************************/
            /* Check to see if this descriptor is ready            */
            /*******************************************************/
            if (FD_ISSET(i, &working_set))
            {
                /****************************************************/
                /* A descriptor was found that was readable - one   */
                /* less has to be looked for.  This is being done   */
                /* so that we can stop looking at the working set   */
                /* once we have found all of the descriptors that   */
                /* were ready.                                      */
                /****************************************************/
                desc_ready -= 1;

                /****************************************************/
                /* Check to see if this is the listening socket     */
                /****************************************************/
                if (i == listen_sock){
                    ESP_LOGE(TAG, "Listening socket is readable");
                    /*************************************************/
                    /* Accept all incoming connections that are      */
                    /* queued up on the listening socket before we   */
                    /* loop back and call select again.              */
                    /*************************************************/
                    do{
                        /**********************************************/
                        /* Accept each incoming connection.  If       */
                        /* accept fails with EWOULDBLOCK, then we     */
                        /* have accepted all of them.  Any other      */
                        /* failure on accept will cause us to end the */
                        /* server.                                    */
                        /**********************************************/
                        new_sd = accept(listen_sock, NULL, NULL);
                        if (new_sd < 0)
                        {
                            if (errno != EWOULDBLOCK)
                            {
                                ESP_LOGE(TAG, "accept() failed");
                                end_server = TRUE;
                            }
                            break;
                        }

                        /**********************************************/
                        /* Add the new incoming connection to the     */
                        /* master read set                            */
                        /**********************************************/
                        ESP_LOGE(TAG, "New incoming connection - %d\n", new_sd);
                        FD_SET(new_sd, &master_set);
                        if (new_sd > max_sd)
                            max_sd = new_sd;

                        /**********************************************/
                        /* Loop back up and accept another incoming   */
                        /* connection                                 */
                        /**********************************************/
                    } while (new_sd != -1);
                }

                /****************************************************/
                /* This is not the listening socket, therefore an   */
                /* existing connection must be readable             */
                /****************************************************/
                else{
                    ESP_LOGE(TAG, "Descriptor %d is readable\n", i);
                    close_conn = FALSE;
                    do{
                        /*************************************************/
                        /* Receive all incoming data on this socket      */
                        /* before we loop back and call select again.    */
                        /* Receive data on this connection until the  */
                        /* recv fails with EWOULDBLOCK.  If any other */
                        /* failure occurs, we will close the          */
                        /* connection.                                */
                        /**********************************************/
                        err = recv(i, rx_buffer, sizeof(rx_buffer), 0);
                        if (err < 0){
                            if (errno != EWOULDBLOCK){
                                ESP_LOGE(TAG, "recv() failed");
                                close_conn = TRUE;
                            }
                            break;
                        }

                        /**********************************************/
                        /* Check to see if the connection has been    */
                        /* closed by the client                       */
                        /**********************************************/
                        if (err == 0){
                            ESP_LOGE(TAG, "Connection closed\n");
                            close_conn = TRUE;
                            break;
                        }

                        /**********************************************/
                        /* Data was received                          */
                        /**********************************************/
                        len = err;
                        ESP_LOGE(TAG, "%d bytes received\n", len);

                        /**********************************************/
                        /* Echo the data back to the client           */
                        /**********************************************/
                        err = send(i, rx_buffer, len, 0);
                        if (err < 0){
                            ESP_LOGE(TAG, "send() failed");
                            close_conn = TRUE;
                            break;
                        }

                        break;      // TODO When removing the do While and the break, it doesn't work anymore... why ?
                    } while (TRUE);

                    /*************************************************/
                    /* If the close_conn flag was turned on, we need */
                    /* to clean up this active connection.  This     */
                    /* clean up process includes removing the        */
                    /* descriptor from the master set and            */
                    /* determining the new maximum descriptor value  */
                    /* based on the bits that are still turned on in */
                    /* the master set.                               */
                    /*************************************************/
                    if (close_conn){
                        close(i);
                        FD_CLR(i, &master_set);
                        if (i == max_sd){
                            while (FD_ISSET(max_sd, &master_set) == FALSE)
                                max_sd -= 1;
                        }
                    }
                } /* End of existing connection is readable */
            } /* End of if (FD_ISSET(i, &working_set)) */
        } /* End of loop through selectable descriptors */

    } while (end_server == FALSE);

    /*************************************************************/
    /* Clean up all of the sockets that are open                 */
    /*************************************************************/
    for (i=0; i <= max_sd; ++i)
    {
        if (FD_ISSET(i, &master_set))
            close(i);
    }
    
    vTaskDelete(NULL);
}

void startSocket(){
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}