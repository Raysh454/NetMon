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
   1 Byte      32 Bytes     8 Bytes      8 Bytes         8 Bytes             8 Bytes                    8 Bytes
| 00000010 | Informer_ID | CPU Usage | Memory Usage | Network Download |  Netowrk Upload | Total Disk Used(Accross all drives)(GB) |  Remaining Null Bytes |
--------------------------------------------------------------------------------------------------------------------------------------------------------------
                                                             128 Bytes
```

#### Server Response to Informer

```
    PTYPE   Error Code Acknowledge
    1 Byte    1 Byte     1 Byte       32 Bytes
| 00000011 | 00000000 | 00000001 |  Error Message | Remaining Null Bytes |

----------------------------------------------------------------------------
                               128 Bytes
```
- If Acknowledge byte is 1 = Successfully updated system usage.
- Error Message = Null if no error.

## Overseer Authentication

```
   PTYPE
   1 Byte    64 Bytes
| 00000100 | Password |  Remaining Null Bytes |
--------------------------------------------------------------------------------------------------------------------------------------------------------------
                                                             128 Bytes
```

## Server Response to Overseer

```
    PTYPE   Error Code 
    1 Byte    1 Byte       32 Bytes
| 00000101 | 00000000 | Error Message | Remaining Null Bytes |

---------------------------------------------------------------
                               128 Bytes
```
- Error Code == 0 Then Authentication Accepted
- Error Code == 1 Then Error

## Server Sends Informer Info to Overseer (After Authentication)

```
 PTYPE
   1 Byte     32 Bytes      32 Bytes      16 Bytes   32 Bytes      1 Byte         2 bytes             2 bytes           8 bytes
| 00000110 | Informer_ID | Computer Name | Platform | CPU Model | Num Of Cores | Main Memory (GB) | Swap Memory (GB) | Total Storage (GB) | Remaining Null Bytes |
-----------------------------------------------------------------------------------------------------------------------------------------------
                                                               128 Bytes
```

- This is sent to overseers for all connected informers when the overseer first connects.
- When a new informer connects, this is sent to all overseers for that informer.


## Server Payload to overseer updating system usage

```
   PTYPE
   1 Byte      32 Bytes     8 Bytes      8 Bytes         8 Bytes             8 Bytes                    8 Bytes
| 00001000 | Informer_ID | CPU Usage | Memory Usage | Network Download |  Netowrk Upload | Total Disk Used(Accross all drives)(GB) |  Remaining Null Bytes |
--------------------------------------------------------------------------------------------------------------------------------------------------------------
                                                             128 Bytes
```

- The server updates all overseers of system usage information for an informer, periodically

## Server Alerting Overseer of an Informer timeout

```
   PTYPE
  1 Bytes      32 Bytes     32 Bytes      
| 00001001 | Informer_ID |   Reason   | Remaining Null Bytes |
--------------------------------------------------------------
                      128 Bytes
```

## Server Check Whether Overseer Is Active or Not

```
  PTYPE
  1 Byte      32 Bytes
| 00001010 | Overseer_ID | Remaining Null Bytes |
-----------------------------------
           128 Bytes
```

## Overseer Response if Active

```
   PTYPE
   1 Byte      32 Bytes
| 00001011 | Overseer ID | Remaining Null Bytes |
-----------------------------------
            128 Bytes
```

- If No response is received overseer is removed from clients.
