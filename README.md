# MiniOS 🖥️

**A tiny operating system you can actually understand!**

MiniOS is an educational operating system written from scratch. It boots on a real x86-64 computer (or emulator), displays text on screen, reads keyboard input, accesses a disk, and even has basic networking. All in about 3,000 lines of code!

---

## 🤔 What Is An Operating System?

When you turn on your computer, something needs to:
1. **Start up the hardware** (screen, keyboard, disk)
2. **Give you a way to interact** with the computer
3. **Run your programs** and manage resources

That "something" is the **operating system** (OS). Windows, macOS, and Linux are all operating systems. MiniOS is a super-simple version that shows you how they work!

---

## 🏗️ How Does MiniOS Boot?

Here's what happens when MiniOS starts:

```
┌─────────────────────────────────────────────────────────────┐
│  1. BIOS/UEFI loads GRUB bootloader from the CD/disk        │
│                          ↓                                   │
│  2. GRUB finds our kernel and loads it into memory          │
│                          ↓                                   │
│  3. boot.asm runs: sets up CPU for 64-bit mode              │
│                          ↓                                   │
│  4. kernel.c runs: initializes all the drivers              │
│                          ↓                                   │
│  5. shell.c runs: shows you the command prompt!             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
minios/
├── 🥾 src/boot/          # Boot code - gets the CPU ready
│   ├── boot.asm          # Assembly code that runs first
│   ├── gdt.c             # Memory segment setup
│   ├── idt.c             # Interrupt handling setup
│   ├── pmm.c             # Physical memory manager
│   └── heap.c            # Memory allocation (like malloc)
│
├── 🧠 src/kernel/        # The brain of the OS
│   └── kernel.c          # Main entry point - starts everything
│
├── 🔌 src/drivers/       # Hardware drivers - talk to devices
│   ├── vga.c             # Screen output (text mode)
│   ├── keyboard.c        # Keyboard input
│   ├── ata.c             # Hard disk access
│   ├── pci.c             # PCI bus (finds hardware)
│   └── virtio_net.c      # Network card driver
│
├── 📚 src/lib/           # Helper functions
│   ├── printf.c          # Formatted printing
│   └── string.c          # String functions (strlen, memcpy, etc.)
│
├── 🌐 src/net/           # Networking stack
│   ├── ethernet.c        # Network packet handling
│   ├── arp.c             # Address resolution
│   └── icmp.c            # Ping protocol
│
├── 💻 src/shell/         # User interface
│   └── shell.c           # Command-line shell
│
└── 📋 include/           # Header files (function declarations)
```

---

## 🧩 Key Concepts Explained

### 1. Assembly Language (`boot.asm`)
Computers only understand numbers (machine code). Assembly is the closest human-readable language to machine code. We use it for the very first boot code because:
- We need exact control over the CPU
- No operating system exists yet to help us!

```asm
; This tells the CPU to stop
hlt
```

### 2. Memory Management
Your computer has RAM (like 8GB or 16GB). The OS needs to:
- **Track what memory is used** → Physical Memory Manager (`pmm.c`)
- **Give memory to programs** → Heap Allocator (`heap.c`)

```c
void* ptr = kmalloc(100);  // Get 100 bytes of memory
kfree(ptr);                 // Give it back when done
```

### 3. Drivers
Drivers are code that "speaks the language" of hardware devices:

| Driver | What It Does |
|--------|--------------|
| `vga.c` | Writes text to the screen by putting characters in video memory at address `0xB8000` |
| `keyboard.c` | Reads key presses from port `0x60` |
| `ata.c` | Reads/writes disk sectors using I/O ports |

### 4. Interrupts
When you press a key, the keyboard sends a signal to the CPU called an **interrupt**. The CPU stops what it's doing, runs our keyboard handler, then continues.

```
Keyboard pressed → IRQ 1 → CPU jumps to our handler → We read the key
```

### 5. I/O Ports
Hardware devices communicate through **ports** (like numbered mailboxes):

```c
outb(0x3D4, 0x0F);     // Send command to VGA port
uint8_t key = inb(0x60); // Read from keyboard port
```

---

## 🚀 Building and Running

### Prerequisites

You need these tools installed:

| Tool | What It Does |
|------|--------------|
| `nasm` | Assembles our `.asm` files |
| `x86_64-elf-gcc` | Compiles C code for x86-64 |
| `qemu` | Emulates a PC so we can test |
| `docker` | Creates bootable ISO (on macOS) |

### On macOS

```bash
# Install tools
brew install nasm qemu xorriso
brew tap nativeos/i386-elf-toolchain
brew install x86_64-elf-gcc

# Build the kernel
make

# Create bootable ISO (requires Docker)
make iso-docker

# Run in QEMU!
make run-iso
```

### On Linux

```bash
# Install tools
sudo apt install nasm qemu-system-x86 grub-pc-bin xorriso gcc

# Build and run
make
make run
```

---

## 🎮 Shell Commands

Once MiniOS boots, you'll see a shell prompt. Try these commands:

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `clear` | Clear the screen |
| `meminfo` | Show memory usage |
| `echo hello` | Print "hello" |
| `diskread 0` | Read sector 0 from disk |
| `diskwrite 1 Hi!` | Write "Hi!" to sector 1 |
| `netinfo` | Show network info |
| `reboot` | Restart the system |
| `halt` | Stop the system |

---

## 🔍 How Things Work (Deep Dive)

### How Text Appears on Screen

The VGA text mode uses memory at address `0xB8000`. Each character takes 2 bytes:
- Byte 1: ASCII character code
- Byte 2: Color (foreground + background)

```c
// Write 'A' in green on black at position (0,0)
volatile uint16_t* video = (uint16_t*)0xB8000;
video[0] = 'A' | (0x02 << 8);  // 0x02 = green
```

### How Keyboard Works

1. You press a key
2. Keyboard controller sends IRQ 1 (interrupt)
3. CPU runs our `keyboard_interrupt_handler`
4. We read the "scancode" from port `0x60`
5. We translate scancode to ASCII and store it
6. Later, `keyboard_getchar()` returns it

### How Disk Access Works

ATA hard disks use "Programmed I/O" - we send commands to ports:

1. Tell the disk which sector we want (LBA = Logical Block Address)
2. Send READ command to port `0x1F7`
3. Wait for disk to be ready
4. Read 512 bytes (one sector) from port `0x1F0`

---

## 🎓 Learning Path

Want to understand operating systems? Here's a suggested order:

1. **Start with `kernel.c`** - See how everything gets initialized
2. **Look at `vga.c`** - Simplest driver, just writes to memory
3. **Study `keyboard.c`** - Learn about interrupts
4. **Explore `shell.c`** - See how the user interface works
5. **Read `heap.c`** - Understand memory allocation
6. **Check `ata.c`** - Learn about disk I/O

---

## 📖 Glossary

| Term | Meaning |
|------|---------|
| **Kernel** | The core of an OS that manages everything |
| **Driver** | Code that controls a hardware device |
| **Interrupt** | A signal that makes the CPU stop and handle an event |
| **Port** | A numbered address used to communicate with hardware |
| **Bootloader** | Program that loads the OS into memory (we use GRUB) |
| **GDT** | Global Descriptor Table - defines memory segments |
| **IDT** | Interrupt Descriptor Table - maps interrupts to handlers |
| **Paging** | Dividing memory into small chunks (pages) for management |
| **Sector** | A 512-byte block on a disk |
| **LBA** | Logical Block Address - a sector's number |

---

## 🧪 Experiments to Try

1. **Change the boot message** in `kernel.c` - rebuild and see your change!

2. **Add a new command** in `shell.c`:
   ```c
   static void cmd_hello(int argc, char* argv[]) {
       printf("Hello from MiniOS!\n");
   }
   // Add to commands[] array
   ```

3. **Change screen colors** - modify `VGA_COLOR_*` values

4. **Read your own data** - use `diskwrite` to save something, reboot, then `diskread` to see it's still there!

---

## 🤝 Credits

MiniOS was created as an educational project to help people understand how operating systems work at a fundamental level. It's inspired by many OS tutorials and hobby OS projects.

---

## 📚 Further Reading

- [OSDev Wiki](https://wiki.osdev.org/) - The bible of OS development
- [Writing a Simple Operating System from Scratch](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf) - Great PDF book
- [Intel x86 Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - Official CPU documentation

---

*Happy hacking! 🚀*
# mini-os
