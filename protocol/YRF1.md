# YRF1 Protocol

YRF1 is the planned application/protocol layer for YWD-RF.

This document intentionally starts as a design placeholder. Field sizes and binary encoding will be finalized only after the RF Lab milestone establishes practical SX1262 packet sizes, timing, and loss characteristics on the target hardware.

## Initial requirements

YRF1 should support:

- protocol versioning
- source and destination node identity
- message/transfer IDs
- packet type
- sequence information
- payload length
- integrity checking
- ACK/retry semantics
- duplicate rejection
- text messages
- status/control frames
- fragmented binary transfers
- missing-block recovery
- interrupted-transfer resume

## Planned early packet types

```text
HELLO
TEXT
ACK
PING
PONG
STATUS
```

Later transfer types are expected to include:

```text
IMAGE_META
IMAGE_BLOCK
TRANSFER_ACK
TRANSFER_NACK
TRANSFER_DONE
TRANSFER_RESUME
```

## Scope rule

YRF1 begins as a point-to-point protocol. Addressing is included from the beginning so multi-node operation can be added later without redesigning every frame, but routing/mesh behavior is explicitly deferred.
