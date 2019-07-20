# ECE499

Instructions:

1. Turn on Bluetooth on phone
2. Upload Calc_UV sketch to flora board (
	- check all connections
	- if running without computer, ensure to comment out line 210 (while (!Serial);)
3. Optional: open serial monitor in arduino IDE (control+shift+m)
3. Open Bluefruit LE app, connect to "Adafruit Bluefruit LE"
4. Open "UART" in modules screen
5. Send a digit from uart screen to start communication (still trying to fix this)
6. Enter skin type (1-6) and SPF as prompted
7. UV data should be sent to your device