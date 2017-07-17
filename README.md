# WagoContinental
A program to interface with the Wago PLC through Modbus and graph the status of the current shift.

## Arguments
#### Modbus Server (Required)
This argument should contain the IP address of the Modbus server (PLC) sans port. The port is hardcoded to 503 for now.
#### First shift start time
Use -a or --first-shift. Should be in the form of military time with the leading "0" and no ":" seperator.
#### Second shift start time
Use -b or --second-shif. Should be the same format as First shift start time but a later time.

#### Proper usage
./WagoContinental -a 0300 -b 1500 192.168.1.2

./WagoContinental --second-shift 1500 --first-shift 0300 192.168.1.2

#### Improper usage
./WagoContinental -a 300 -b 1500 192.168.1.2. Missing the leading "0" for "-a".

./WagoContinental -a 1500 -b 300 192.168.1.2. "-a" should always come after "-b".

./WagoContinental -a 03:00 -b 15:00 192.168.1.2. There should be no ":" seperator between minutes and hours.
