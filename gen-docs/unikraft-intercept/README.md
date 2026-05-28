# Unikraft Intercept Docs Index

This directory contains the generated documentation for the current Unikraft
intercept implementation.

## Start Here

- [STATUS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/STATUS.md)
  - current guest coverage
  - active examples
  - known limits
  - current implementation order

## Implementation Detail Docs

- [INTERCEPT_ARCHITECTURE.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/INTERCEPT_ARCHITECTURE.md)
  - overall design
  - boot lifecycle
  - remote fd model
  - syscall semantics

- [INTERCEPT_HOOKS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/INTERCEPT_HOOKS.md)
  - where intercept is wired into Unikraft
  - which examples exercise which hooks
  - extension checklist

- [INTERCEPT_RPC.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/INTERCEPT_RPC.md)
  - ONC RPC/XDR constants
  - transport behavior
  - shared RPC core
  - per-syscall wire details and caveats

## Planning And Server Contract Docs

- [HTTP_SERVER_ROADMAP.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/HTTP_SERVER_ROADMAP.md)
  - what `intercept-http` still needs for real remote file serving
  - mixed local-socket plus remote-file roadmap

- [SERVER_REQUIREMENTS.md](/home/liviu/dev/hearc/tb/unikraft/catalog-core/gen-docs/unikraft-intercept/SERVER_REQUIREMENTS.md)
  - requirements the guest side should be able to demand from the syscall
    server
  - connection/session semantics
  - fd mapping and protocol upgrade requests
