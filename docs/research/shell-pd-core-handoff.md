# Shell-PD Core Handoff Research Note

## Summary

This experiment validates a first step toward treating LionsOS user space as a set of core-local execution domains. It does not implement a multikernel AMP system yet. It proves that a Microkit/seL4 SMP image can host four shell protection domains, pin each shell to a different core, and hand off the active interactive shell between them.

## Experiment

The `examples/shell` system now builds four Shell-PDs:

- `shell0` pinned to core 0
- `shell1` pinned to core 1
- `shell2` pinned to core 2
- `shell3` pinned to core 3

The system runs with `MICROKIT_CONFIG=smp-debug` and QEMU uses `-smp 4`. The serial driver and serial virtualisers remain on core 0. `shell0` owns serial ingress because sDDF serial RX has one active client. When the user runs `core N`, the current shell deactivates itself and requests activation of `shellN`. For shells 1-3, `shell0` forwards terminal characters to the active shell through protected calls, and activation is signalled with notifications.

## Observed Result

The shell successfully handed off across all four core-local Shell-PDs:

```text
Shell-PD ready: shell0 on core 0
lions> core 1
Shell-PD ready: shell1 on core 1
lions> core 2
Shell-PD ready: shell2 on core 2
lions> core 3
Shell-PD ready: shell3 on core 3
lions> core 0
activated shell0 on core 0
```

Commands such as `hello` execute in the active Shell-PD and report the expected shell identity.

## What This Proves

- Microkit can place PDs on specific cores using the `cpu` attribute.
- A single SMP seL4 kernel can run multiple core-local shell execution domains.
- Shell activation can be handed off using seL4 notifications.
- Terminal input can be routed to active shell domains using protected IPC.

## What This Does Not Prove Yet

This is not a multikernel AMP boot. There is still one seL4 kernel managing all four cores. It does not prove multiple independent kernel images, cross-kernel shared memory, or distributed capability translation. Those require a separate memory map, shared physical regions, inter-kernel signalling, and user-space authority protocol.

## Next Steps

1. Add a shared memory region between Shell-PDs for command state or mailbox data.
2. Replace protected-call input forwarding with shared-memory queues plus notifications.
3. Prototype the same shared-memory queue layout across independent kernel images.
4. Define how a distributed user-space capability token maps to local seL4 caps per kernel.
