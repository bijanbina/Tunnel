# Tunnel

### Observations
1. Each side can send 5 packet before the conn is dropped. so on 1 connection you can tx 5 pack and rx 5 pack at the same time
2. MTU is set to be 1500 byte. thus the payload on each side is 7KB considering tcp header and footer
3. MTU can be inreased to 9K but this needs to be supported on all router alng the way which is not the case so this idea is dropped