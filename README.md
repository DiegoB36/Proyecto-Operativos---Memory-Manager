# Memory Manager

Memory management simulator for an Operating Systems course. Models a **paged segmentation** scheme with main memory (RAM) and swap memory (Swap), persisting state in JSON files.

The program takes a source code file, splits it into segments and pages, distributes them across RAM and Swap, and lets you query free memory, release processes, and swap pages between Swap and RAM (demand paging).

## Features

- **Memory allocation** (`memoryAllocation`): splits a file into 3 segments and paginates each segment into 50-character blocks.
- **Load to RAM/Swap** (`uploadToRam`): places the first page of each segment in RAM and the rest in Swap. If the process already exists, it frees its memory before reloading.
- **Free memory query** (`freeMem`): counts free frames and returns available memory in KB.
- **Memory per process** (`calculateMemoryUsedByProcess`): memory consumed by a specific `process_id`.
- **Memory release** (`releaseMemory`): frees all frames of a process in RAM and Swap and removes its page tables.
- **Page swap** (`memorySwap`): brings a page from Swap into RAM, evicting the resident page and updating the page table (`presence_bit`, `frame_ram`).

## Project structure

```
.
├── MemoryManager.cpp      # Core memory manager logic
├── ProgramaEjemplo.cpp    # Sample file to paginate (Towers of Hanoi)
├── ProgramaEjemplo.txt    # Alternative sample file
├── nlohmann/
│   └── json.hpp           # JSON library (nlohmann/json, header-only)
└── .gitignore
```

> `RAM.json` and `Swap.json` are generated/consumed at runtime and are ignored by git.

## Data model

Each frame in `RAM.json` / `Swap.json`:

| Field          | Type   | Description                       |
|----------------|--------|-----------------------------------|
| `content`      | string | Page content                      |
| `frame_number` | int    | Frame index                       |
| `is_free`      | bool   | Whether the frame is free         |
| `page_number`  | int    | Page number within the segment    |
| `process_id`   | int    | Process that owns the frame       |
| `segment_id`   | int    | Segment the frame belongs to      |

Frame size: `4 KB` (`FRAME_SIZE = 4 * 1024`).

`RAM.json` also holds the `SO` key: list of processes with their segments and page tables (`frame_swap`, `frame_ram`, `presence_bit`).

## Requirements

- C++ compiler with C++11 or later support (g++, clang, MSVC).
- [nlohmann/json](https://github.com/nlohmann/json) — already bundled in `nlohmann/json.hpp`.

## Build and run

```bash
g++ MemoryManager.cpp -o MemoryManager
./MemoryManager
```

> Paths in `MemoryManager.cpp` are relative (`../RAM.json`, `../Swap.json`, `../ProgramaEjemplo.cpp`), so the executable is expected inside a subdirectory (e.g. `output/`). Adjust the `jsonRAMPath`, `jsonSwapPath`, and `filePath` constants if your structure differs.

## Usage

`main()` controls which operation runs. Uncomment the desired call:

```cpp
int process_id = 0;

bool result = memoryAllocation(process_id);                    // Allocate memory
cout << "Memoria disponible: " << freeMem() << " KB" << endl;  // Query free memory
releaseMemory(process_id);                                     // Release process
memorySwap(1, 3, 0);                                           // Swap segment 1, page 3, process 0
```

## Notes

- `RAM.json` and `Swap.json` must exist with an initialized `frames` array before running load operations.
- The scheme always splits the file into 3 segments (`segmentSize = ceil(lines / 3.0)`).
