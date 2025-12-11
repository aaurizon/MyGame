# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds C++ sources: platform layer in `Win32/`, graphics backends under `Graphics/{OpenGL,Vulkan,DirectX11,DirectX12}`, and core engine objects (window, viewport, world, camera) at the root.
- `include/` exposes engine headers (extensionless includes such as `<AWindow>`).
- `src_old_version/` is legacy reference code; avoid editing unless explicitly migrating something.
- `CMakeLists.txt` builds the static library `MyGameEngine` plus the `MyGame` executable; adjust `VULKAN_SDK_ROOT` if your SDK path differs.

## Build, Run, and Development Commands
- Configure: `cmake -S . -B build` (uses C++20, expects Vulkan SDK at `C:/VulkanSDK/1.4.335.0` by default).
- Build: `cmake --build build`
- Run: `./build/MyGame` from the repo root (Win32 window opens with four viewports; `WASD`/mouse to move, `P` toggles mouse capture, `O` cycles graphics backends).
- Debug tip: keep the Vulkan SDK `Bin` folder on `PATH` to load validation layers.

## Coding Style & Naming Conventions
- Formatting: `.editorconfig` enforces LF endings, 4-space indentation, no tabs; `.clang-format` sets 4-space indent, 120 column limit, brace-wrapping after control statements/classes/namespaces.
- Linting: `.clang-tidy` is enabled with warnings-as-errors; preferred cases—classes `CamelCase`, functions/variables `camelBack`, member fields prefixed `m_`.
- Naming: engine types start with `A` (e.g., `AWindow`, `ARenderOverlay`); interfaces start with `I` (e.g., `IWindowImpl`, `IRendererImpl`); enums start with `E` (e.g., `EGraphicsBackend`).
- Headers: include engine headers with angle brackets and no extension (e.g., `<AWindow>`, `<ARenderOverlay>`); keep includes sorted per clang-format. Source-local headers in `src/` use `.hpp` when present; template fragments use `.tpl` included from headers.
- Platform impls live under platform folders (e.g., `Win32/`) behind an `*Impl` interface; avoid direct platform calls elsewhere.

## Testing Guidelines
- No automated tests yet. Validate locally by running `./build/MyGame` and checking: window opens, four viewports render, backend cycling (`O`) works, resize keeps layout, and `Esc` closes cleanly.
- When adding tests, prefer lightweight unit tests near the module under test and name them after the class/function being exercised.

## Commit & Pull Request Guidelines
- Commits follow short, capitalized summaries (e.g., `Revise and expand project documentation`); keep them under ~72 characters and scoped to one logical change.
- For PRs, include: goal/what changed, build/run commands executed, notable platform/SDK requirements, and screenshots or logs if you touch rendering. Link related issues when applicable.

## Ownership
- `AFloatingText` is owned by `AWorld` (world takes ownership of pointers you add; don’t share across worlds).
- `ARenderOverlay` owns its texts internally; attach/detach overlays to viewports as needed (a viewport keeps pointers only; caller manages overlay lifetime).
