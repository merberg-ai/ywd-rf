# YRF1 protocol

YRF1 is the experimental application/protocol layer for YWD-RF.

## Status

`0.0.1-dev` uses only a temporary RF Lab test frame:

```text
YRF1|TEST|<node-id>|<sequence>
```

Example:

```text
YRF1|TEST|YWD-A43F|218
```

This human-readable frame is intentionally temporary. It is useful for first hardware bring-up and serial debugging but is not the final binary protocol.

## Planned binary framing

The first real YRF1 frame format will reserve fields for:

- protocol magic/version
- frame type
- source address
- destination address
- message/transfer ID
- sequence or fragment number
- flags
- payload length
- payload
- integrity check

Initial frame types are expected to include `HELLO`, `TEXT`, `ACK`, `PING`, `PONG`, and `STATUS`, followed later by image-transfer metadata/data/recovery frames.

## Design rules

- A node must be able to operate without a phone connected.
- Duplicate frames must be safely detectable.
- Higher layers must tolerate loss and retransmission.
- Interrupted bulk transfers must eventually support selective resume.
- RF transport details should not leak unnecessarily into the WebUI.
- Node identity is runtime configuration; all compatible nodes run one firmware image.
