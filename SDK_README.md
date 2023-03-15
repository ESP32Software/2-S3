## Settings

# Flash Size
16 MB (if using Esp32s3-WROOM-1, othersise apply the size to match your device)

# Partition Settings
Custom parition name "partitions.csv"
Offset of partition table 0x9000   (normal value of 0x8000 won't allow debugging messages during development)

# Indication Config
[*] Enable RGB LED
WS2812 LED GPIO (select the correct GPIO)