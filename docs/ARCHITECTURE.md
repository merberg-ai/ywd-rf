# YWD-RF Architecture

## Design principles

YWD-RF treats the ESP32-S3 node as an autonomous radio appliance. The phone/browser UI is a client of the node, not the owner of the radio protocol state.

The firmware should continue receiving, acknowledging, queueing, storing, and completing transfers even when no phone is connected.

## Initial layers

```text
Phone / browser
      |
      | Wi-Fi / HTTP
      v
Local WebUI + API
      |
      v
Messages / transfers / storage
      |
      v
YRF1 protocol
      |
      v
Radio transport
      |
      v
RadioLib -> SX1262
```

## Firmware modules

Planned logical modules:

- `radio` — SX1262 configuration, TX/RX, RSSI/SNR, PHY switching
- `protocol` — YRF1 framing, addressing, packet validation
- `messages` — text message state and delivery tracking
- `transfers` — fragmentation, image blocks, resume
- `storage` — persistent configuration, queues, history
- `display` — OLED status and notifications
- `web` — local HTTP/API/WebUI service

## Firmware identity

All nodes run the same firmware image. Device name, short ID, role, RF profile, and peer configuration are persisted at runtime.

## Early development rule

Do not add routing or mesh behavior until the point-to-point link, delivery semantics, queueing, and interrupted-transfer recovery are dependable.
