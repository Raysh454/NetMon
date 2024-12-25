# Protocols

## Informer Initialization

#### Client Request
```
 PTYPE
   1 Byte     32 Bytes       16 Bytes   32 Bytes      1 Byte         2 bytes             2 bytes           8 bytes
| 00000000 | Computer Name | Platform | CPU Model | Num Of Cores | Main Memory (GB) | Swap Memory (GB) | Total Storage (GB) | Remaining Null Bytes |
-----------------------------------------------------------------------------------------------------------------------------------------------
                                                               128 Bytes

```

#### Server Response (If all goes well)
```
    PTYPE    Error Code
            0 = No error
   1 Byte      1 Byte        32 Byte          Remaining
| 00000001 |  00000000  | Informer_ID | Null * (128 - size of previous sections)|
---------------------------------------------------------------------------------
								                  128 Bytes
```

## Informer System Information
#### Client Payload

```
   PTYPE
   1 Byte      32 Bytes     
| 00000010 | Unique Code | CPU Usage | Memory Usage | Network Usage | Total Disk Used(Accross all drives)(GB) |  Remaining Null Bytes |
-------------------------------------------------------------------------------------------------------------
                                128 Bytes
```

#### Server Response

```
    PTYPE   Error Code Acknowledge
    1 Byte    1 Byte     1 Byte
| 00000011 | 00000000 | 00000001 |  Remaining Null Bytes |

----------------------------------------------------------
                               128 Bytes
```
- If Acknowledge byte is 0: it means the response was corrupted(or something idk)