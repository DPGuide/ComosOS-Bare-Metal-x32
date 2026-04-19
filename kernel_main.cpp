/// kernel_main.cpp (Wird zu KERNEL.BIN kompiliert)
#include "boot_info.h"
#include <stdint.h>
#include <stddef.h> /// FIX 1: Bringt uns 'size_t' für den Memory Manager!

/// ==========================================
/// 1. HEAP FÜR OS TEIL 2
/// ==========================================
struct MemoryBlock { size_t size; uint8_t is_free; MemoryBlock* next; };
MemoryBlock* heap_head = nullptr;

void init_heap() { heap_head = (MemoryBlock*)0x03000000; heap_head->size = 1024 * 1024 * 32; heap_head->is_free = 1; heap_head->next = nullptr; }
void* malloc(size_t size) { MemoryBlock* curr = heap_head; while (curr != nullptr) { if (curr->is_free == 1 && curr->size >= size) { curr->is_free = 0; return (void*)((uint8_t*)curr + sizeof(MemoryBlock)); } curr = curr->next; } return nullptr; }
void free(void* ptr) { if (ptr == nullptr) return; MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock)); block->is_free = 1; }
void* operator new(size_t size) { return malloc(size); }
void* operator new[](size_t size) { return malloc(size); }
void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
void operator delete(void* ptr, size_t size) noexcept { free(ptr); }

/// ==========================================
/// 2. BARE METAL I/O PORTS
/// ==========================================
inline uint8_t inb(uint16_t port) { uint8_t ret; asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
inline void outb(uint16_t port, uint8_t val) { asm volatile("outb %0, %1" : : "a"(val), "Nd"(port)); }

/// ==========================================
/// 3. BARE METAL MINI TEXT-ENGINE
/// ==========================================
void HexToStr(uint32_t value, char* buffer) {
    const char hex_chars[] = "0123456789ABCDEF"; buffer[0] = '0'; buffer[1] = 'x'; buffer[10] = '\0';
    for(int i = 7; i >= 0; i--) { buffer[i + 2] = hex_chars[value & 0xF]; value >>= 4; }
}

void DrawMiniChar(uint32_t* screen, uint32_t w, uint32_t x, uint32_t y, char c) {
    uint8_t f_u[] = { 0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22, 0x7F,0x41,0x41,0x22,124, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x49,0x49,0x7A, 0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E, 0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F, 0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43 };
    uint8_t f_l[] = { 0x20,0x54,0x54,0x54,0x78, 0x7F,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x20, 0x38,0x44,0x44,0x48,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x08,0x7E,0x09,0x01,0x02, 0x0C,0x52,0x52,0x52,0x3E, 0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x44,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00, 0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x18,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38, 0x7C,0x14,0x14,0x14,0x08, 0x08,0x14,0x14,0x18,0x7C, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20, 0x04,0x3F,0x44,0x40,0x20, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C, 0x44,0x28,0x10,0x28,0x44, 0x0C,0x50,0x50,0x50,0x3C, 0x44,0x64,0x54,0x4C,0x44 };
    uint8_t f_n[] = { 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31, 0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03, 0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E };
    
    uint8_t* ptr = nullptr;
    if(c >= 'A' && c <= 'Z') ptr = &f_u[(c-'A')*5]; 
    else if(c >= 'a' && c <= 'z') ptr = &f_l[(c-'a')*5];
    else if(c >= '0' && c <= '9') ptr = &f_n[(c-'0')*5];
    else if(c == ':') { uint8_t s[]={0,0x36,0x36,0,0}; ptr=s; }
    else if(c == ' ') { return; } // Leerzeichen ignorieren
    if(!ptr) return;
    
    for(uint32_t m=0;m<5;m++){ 
        uint8_t l=ptr[m]; 
        for(uint32_t n=0;n<7;n++) {
            if((l>>n)&1) { screen[(y+n)*w + (x+m)] = 0xFFFFFF; } 
        } 
    } 
}

void DrawMiniText(uint32_t* screen, uint32_t w, uint32_t x, uint32_t y, const char* s) {
    while(*s) { DrawMiniChar(screen, w, x, y, *s++); x+=6; }
}

/// ==========================================
/// 4. INTERRUPT DESCRIPTOR TABLE (IDT) & TASTATUR
/// ==========================================
struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

IDTEntry idt[256];
IDTPtr idt_ptr;

void set_idt_gate(int n, uint32_t handler) {
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = 0x08; // Kernel Code Segment (Von GRUB vorgegeben)
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E; // 32-bit Interrupt Gate
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

/// Globale Variablen für den Tastatur-Cursor
uint32_t* global_screen = nullptr;
uint32_t global_width = 0;
uint32_t cursor_x = 100;
uint32_t cursor_y = 200;

char get_ascii_qwertz(uint8_t sc) {
    char k_low[] = {
        0,27,'1','2','3','4','5','6','7','8','9','0',0,0,'\b','\t',
        'q','w','e','r','t','z','u','i','o','p',0,0,'\n',0,
        'a','s','d','f','g','h','j','k','l',0,0,0,0,0,
        'y','x','c','v','b','n','m',0,0,0,0,0,0,' '
    };
    if (sc < sizeof(k_low)) return k_low[sc];
    return 0;
}

/// DER KEYBOARD TREIBER (Wird von der Hardware aufgerufen!)
struct interrupt_frame;

/// FIX 2: general-regs-only zwingt den GCC, keine FPU Register im Interrupt zu nutzen!
__attribute__((interrupt, target("general-regs-only"))) 
void keyboard_isr(struct interrupt_frame* frame) {
    uint8_t scancode = inb(0x60); // Taste auslesen
    
    /// Wenn das oberste Bit NICHT gesetzt ist, wurde die Taste gedrückt (Make-Code)
    if (!(scancode & 0x80)) {
        char c = get_ascii_qwertz(scancode);
        if (c != 0) {
            DrawMiniChar(global_screen, global_width, cursor_x, cursor_y, c);
            cursor_x += 8; // Cursor nach rechts schieben
        }
    }
    
    outb(0x20, 0x20); // Sag der Hardware: "Danke, habe den Befehl erhalten!" (EOI)
}

/// Programmiert den Interrupt-Controller (PIC) um
void remap_pic() {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28); // Lege Hardware-Interrupts auf 0x20 aufwärts
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFD); outb(0xA1, 0xFF); // ALLES blockieren, AUSSER der Tastatur (IRQ1 = 0xFD)
}

/// ==========================================
/// 5. DER HAUPT-EINSTIEG FÜR OS TEIL 2
/// ==========================================
extern "C" void main(BootInfo* sys_info) {
    init_heap();
    
    global_screen = (uint32_t*)sys_info->framebuffer_addr;
    global_width = sys_info->screen_width;
    uint32_t h = sys_info->screen_height;
    
    /// Bildschirm dunkelgrün färben
    for(uint32_t i = 0; i < (global_width * h); i++) global_screen[i] = 0x00004400;

    /// Willkommenstext
    DrawMiniText(global_screen, global_width, 100, 100, "KERNEL V2 ONLINE");
    DrawMiniText(global_screen, global_width, 100, 130, "USB PORT OVER HANDOFF:");

    char port_str[16];
    HexToStr(sys_info->uhci_base_port, port_str);
    DrawMiniText(global_screen, global_width, 250, 130, port_str);
    
    DrawMiniText(global_screen, global_width, 100, 170, "PRESS ANY KEY TO TYPE:");

    /// === IDT ZÜNDUNG ===
    remap_pic();
    idt_ptr.limit = (sizeof(IDTEntry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;
    set_idt_gate(33, (uint32_t)keyboard_isr); // 33 = 0x20 (PIC Offset) + 1 (Tastatur IRQ)
    
    asm volatile("lidt %0" : : "m"(idt_ptr)); // Lade das neue Telefonbuch in die CPU
    asm volatile("sti");                      // Aktiviere Hardware-Interrupts!

    /// Endlosschleife - Ab jetzt übernimmt die ISR!
    while(1) { asm volatile("hlt"); }
}