This document stores some logging information about the GMS unit.

## Receiving a call

The serial receive buffer will receive the following lines when a call starts: 
```
RING


```

... when the call keeps ringing, the following line will be repeated every second or so
```
RING

```

And so on...
```
RING

RING

RING

RING

```

Until the call stops (from caller side)
```
RING
+CIEV: "CALL",0

```

## Receiving a SMS

The serial receive buffer will receive the following lines when a SMS is sent to the unit:
```
+CIEV: "MESSAGE",1
+CMT: "+27<rest of phone number>",,"2022/09/01,15:40:28+02"
<content line 1>
<content line 2>
<content line etc...>
```

## Sending a SMS

Send this to enable text mode:
```
AT+CMGF=1
```

Send this to start a SMS to a phone number:
```
AT+CMGS=<phone number>
```

It will prompt for the SMS content, so you can just send that:
```
This is the SMS content\r\n
And a new line like so.
```

Then submit your message by sendint a Ctrl+Z (0x1a) character.
```

```

The GSM unit will respond with `+CMGS=<number>` to indicate the message was received, and with `OK` to indicate the message was sent. 

## HTTP requests

First enable internet and connect to the domain (and find its IP).
```
AT+CGATT=1
AT+CIPSTART="TCP","httpbin.org",80
```

Then query a URL. This will print the response of the server also.
```
AT+HTTPGET="http://httpbin.org/get?q=test"
```
Or a POST request:
```
AT+HTTPPOST="http://httpbin.org/post","application/json","{\"var1\":12}"
```

Close the connection.
```
AT+CIPCLOSE
```

To check if the connection is open or closed:
```
AT+CIPSTATUS
```

## Interact with USSD

Start the USSD to number `*135*502#` (text mode needs to be disabled):
```
AT+CMGF=0
AT+CUSD=1,"*135*502#",15
```

Catch the output and convert the data between the `"` to hex. Parse this data on http://smstools3.kekekasvi.com/topic.php?id=288 in the _USSD Entry/Display_ field as _GSM 7bit packed_.

To select an option of the message, use the same command again but this time with the option you want to select. To select option 2:
```
AT+CUSD=1,"2",15
```

If an SMS is sent, it will be received in PDU format. Pare this data on https://www.diafaan.com/sms-tutorials/gsm-modem-tutorial/online-sms-pdu-decoder/.
