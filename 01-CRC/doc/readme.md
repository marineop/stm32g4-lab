## Tutorial
* Per the board's user manual, we can communicate with LPUART1 using a USB VCOM connection (PA2/PA3).
![alt text](image.png)

* According to the reference manual, the maximum baud rate is 9600.  
![alt text](image-3.png)

* Configure the .ioc file  
![alt text](image-1.png)
![alt text](image-2.png)

* Setup printf to use LPUART1  
    ``` C
    /* USER CODE BEGIN 0 */
    int __io_putchar(int ch)
    {
        HAL_UART_Transmit(&hlpuart1, (const uint8_t *)&ch, 1, HAL_MAX_DELAY);
        return ch;
    }
    /* USER CODE END 0 */
    ```

* Print a test message. Be careful to only add code within the user code regions, as any code outside these regions will be overwritten when the .ioc regenerates code.  
    ``` C
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        HAL_Delay(1000);
        printf("Hello World!\r\n");
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
    ```

* Check the CRC computed by MCU is correct  
![alt text](image-4.png)
![alt text](image-5.png)