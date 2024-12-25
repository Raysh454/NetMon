# Protocols

## Informer Initialization

#### Client Request
```
 PTYPE
   1 Byte     32 Bytes       16 Bytes   32 Bytes      1 Byte        8 bytes
| 00000000 | Computer Name | Platform | CPU Model | Num Of Cores | Storage (GB) |
| Remaining Null Bytes |
---------------------------------------------------------------------------------
								   128 Bytes

```

#### Server Response (If all goes well)
```
    PTYPE    Error Code
            0 = No error
   1 Byte      1 Byte        32 Byte          Remaining
| 00000001 |  00000000  | Unique Code | Null * (128 - size of previous sections)|
---------------------------------------------------------------------------------
								                  128 Bytes
```

## Informer System Information
#### Client Payload

```
   PTYPE
   1 Byte      32 Bytes     
| 00000010 | Unique Code | CPU Usage | Memory Usage | Network Usage | Disk Used(GB) |  Remaining Null Bytes |
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
