/// kernel_main.cpp (Cosmos OS V2 - Windows & Live RTC FIXED)
#include "boot_info.h"
#include "schneider_lang.h"
#include <stdint.h>
#include <stddef.h>
/// ==========================================
/// BARE METAL TOOL: STRING CONCAT (DER KLEBER)
/// ==========================================
void str_cat(char* dest, const char* src) {
    /// 1. Gehe zum Ende des ersten Textes (bis zur Null)
    while (*dest) {
        dest++;
    }
    /// 2. Kopiere den zweiten Text genau dort hin
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    /// 3. Setze am Ende die neue Null-Terminierung
    *dest = 0;
}
/// ==========================================
/// BARE METAL FIX: LOKALER PCI-READER (BYPASS LINKER)
/// ==========================================
uint32_t pci_read(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    
    /// Direkter CPU-Hardware-Zugriff (Inline Assembler) -> Braucht keine externen Dateien!
    __asm__ volatile("outl %0, %w1" : : "a"(address), "Nd"(0xCF8));
    uint32_t result;
    __asm__ volatile("inl %w1, %0" : "=a"(result) : "Nd"(0xCFC));
    
    return result;
}
struct MirrorEntry {
    uint32_t bus, dev, func;
    uint32_t vendor, device;
    uint64_t bar0;
    char name[32];
};
MirrorEntry mirror_list[32];
int mirror_count = 0;
bool show_oracle = false;

extern _50 pci_scan_all();
extern _44 xhci_bot_read_sectors(_184 slot_id, _43 lba, _89 dest_ram);
extern void xhci_poll_events_and_mouse();
extern void (*usb_mouse_callback)(int, int, int);


/// ==========================================
/// BARE METAL FIX: DER UNIVERSAL-LESE-ADAPTER (FINAL)
/// ==========================================
extern int selected_drive_idx; 
extern int ahci_read_sectors(_89 lba, _89 dest_ram); /// BARE METAL FIX: Return-Typ ist int!
extern _44 xhci_bot_read_sectors(_184 slot_id, _89 lba, _89 dest_ram);

_44 disk_read_auto(_89 lba, _89 dest_ram) {
    /// BARE METAL FIX: Wipe the target buffer BEFORE the controller writes to it!
    /// We assume a standard 512-byte sector read here. 
    /// If you read more sectors at once later, increase this loop size.
    _39(int i=0; i<512; i++) {
        ((char*)dest_ram)[i] = 0;
    }

    _15(selected_drive_idx == 99) {
        /// Es ist der USB Stick!
        _96 xhci_bot_read_sectors(1, lba, dest_ram);
    } _41 {
        /// Es ist eine SATA Festplatte!
        _96 ahci_read_sectors(lba, dest_ram);
    }
}
/// ==========================================
/// BARE METAL FIX: DER UNIVERSAL-SCHREIB-ADAPTER
/// ==========================================
extern int ahci_write_sectors(_89 lba, _89 src_ram); 
extern _44 xhci_bot_write_sectors(_184 slot_id, _89 lba, _89 src_ram);

_44 disk_write_auto(_89 lba, _89 src_ram) {
    _15(selected_drive_idx == 99) {
        /// Es ist der USB Stick!
        _96 xhci_bot_write_sectors(1, lba, src_ram);
    } _41 {
        /// Es ist eine SATA Festplatte!
        _96 ahci_write_sectors(lba, src_ram);
    }
}
/// BARE METAL FIX: Den RAM für Texte restlos reinigen!
void mem_set(void* ptr, uint8_t value, uint32_t num) {
    uint8_t* p = (uint8_t*)ptr;
    while (num--) *p++ = value;
}
/// ==========================================
/// 1. HEAP ALLOCATOR (64-BIT GEFIXT)
/// ==========================================
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
/// 2. BARE METAL PORTS & MOUSE DRIVER
/// ==========================================
inline uint8_t inb(uint16_t port) { uint8_t ret; asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
inline void outb(uint16_t port, uint8_t val) { asm volatile("outb %0, %1" : : "a"(val), "Nd"(port)); }
inline uint32_t inl(uint16_t port) { uint32_t ret; asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
inline void outl(uint16_t port, uint32_t val) { asm volatile("outl %0, %1" : : "a"(val), "Nd"(port)); }
inline void outw(uint16_t port, uint16_t val) { asm volatile("outw %0, %1" : : "a"(val), "Nd"(port)); }
_50 mouse_wait(_184 type) { _89 t = 100000; _114(t--) { _15(type EQ 0 AND (inb(0x64)&1)) _96; _15(type EQ 1 AND !(inb(0x64)&2)) _96; } }
_50 mouse_write(_184 w) { mouse_wait(1); outb(0x64, 0xD4); mouse_wait(1); outb(0x60, w); }
_184 mouse_read() { mouse_wait(0); _96 inb(0x60); }
_50 init_mouse() { mouse_wait(1); outb(0x64,0xA8); mouse_wait(1); outb(0x64,0x20); mouse_wait(0); _184 s=inb(0x60)|2; mouse_wait(1); outb(0x64,0x60); mouse_wait(1); outb(0x60,s); mouse_write(0xF6); mouse_read(); mouse_write(0xF4); mouse_read(); }
/// ==========================================
/// 3. REAL-TIME CLOCK (RTC) BARE METAL
/// ==========================================
_43 rtc_h, rtc_m, rtc_day, rtc_mon, rtc_year;
_184 bcd2bin(_184 b) { _96 ((b >> 4) * 10) + (b & 0xF); }
_50 read_rtc() { 
    outb(0x70, 4); rtc_h = bcd2bin(inb(0x71)); 
    outb(0x70, 2); rtc_m = bcd2bin(inb(0x71)); 
    outb(0x70, 7); rtc_day = bcd2bin(inb(0x71)); 
    outb(0x70, 8); rtc_mon = bcd2bin(inb(0x71)); 
    outb(0x70, 9); rtc_year = bcd2bin(inb(0x71)); 
    rtc_h = (rtc_h + 1) % 24; /// Zeitzonen-Korrektur (CET)
}
/// ==========================================
/// 4. ENGINE GLOBALS & DATA STRUCTURES
/// ==========================================
_89* fb = 0; _89* bb = (_89*)0x02000000;
_89 screen_w = 800, screen_h = 600, frame = 0;
_43 mouse_x = 400, mouse_y = 300; _44 mouse_down = _86, mouse_just_pressed = _86;
int mouse_sub_x = 40000;
int mouse_sub_y = 30000;
/// Deine einstellbare Sensitivität! 
/// 100 = 1.0 (Normal) | 50 = 0.5 (Halb) | 10 = 0.1 (Extrem langsam)
int mouse_sens = 10; /// <-- Versuch es mal mit 30 (0.3) für die Diva!
/// ==========================================
/// BARE METAL FIX: DIE USB-MAUS SCHNITTSTELLE
/// ==========================================
void update_mouse_position(int dx, int dy, int btn) {
    
    /// BARE METAL FIX: Der USB-Werte-Wrap!
    /// Wir zwingen die 0-255 Werte hart in den negativen Bereich (-128 bis +127)
    int real_dx = (signed char)dx;
    int real_dy = (signed char)dy;
    
    /// 1. Die echte Mausbewegung mit Sensitivität multiplizieren
    mouse_sub_x += (real_dx * mouse_sens);
    mouse_sub_y += (real_dy * mouse_sens);
    
    /// 2. Den echten Bildschirm-Pixel berechnen
    int new_x = mouse_sub_x / 100;
    int new_y = mouse_sub_y / 100;

    /// 3. Achsen abriegeln UND Sub-Pixel-Konto zurücksetzen!
    if (new_x < 0) { new_x = 0; mouse_sub_x = 0; }
    if (new_x > 799) { new_x = 799; mouse_sub_x = 799 * 100; }
    
    if (new_y < 0) { new_y = 0; mouse_sub_y = 0; }
    if (new_y > 599) { new_y = 599; mouse_sub_y = 599 * 100; }

    /// 4. An dein System übergeben
    mouse_x = new_x;
    mouse_y = new_y;

    /// 5. Klick-Logik
    if (btn & 1) { 
        if (!mouse_down) mouse_just_pressed = _128; 
        else mouse_just_pressed = _86;
        mouse_down = _128;
    } else {
        mouse_down = _86;
        mouse_just_pressed = _86;
    }
}
_72 _184 m_packet[3]; _72 _43 m_ptr = 0;
_44 galaxy_open = _86; _43 galaxy_expansion = 0;
_43 input_cooldown = 0; _44 click_consumed = _86;
struct Window { _43 id; _30 title[16]; _43 x, y, w, h; _44 open, minimized, fullscreen; _89 color; _30 content[2048]; _43 cursor_pos; };
struct Planet { _43 ang; _43 dist; _30 name[8]; _43 cur_x, cur_y; }; 
struct Star { _43 x, y, z, type, speed; };
_43 win_z[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
Window windows[13]; 
Planet planets[5];
Star stars[200];
_43 drag_win = -1; _43 drag_off_x = 0; _43 drag_off_y = 0; _43 resize_win = -1; _44 z_blocked = _86;
_72 _89 rng_seed = 123456789;
_89 random() { rng_seed = (rng_seed * 1103515245 + 12345) & 0x7FFFFFFF; _96 rng_seed; }
_43 int_sqrt(_43 n) { _43 x=n, y=1; _114(x>y){x=(x+y)/2; y=n/x;} _96 x; }
_71 _43 sin_lut[256] = { 1, 2, 4, 7, 9, 12, 14, 17, 19, 21, 24, 26, 28, 30, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 56, 58, 60, 61, 63, 64, 66, 67, 68, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 79, 80, 81, 81, 82, 82, 83, 83, 83, 84, 84, 84, 84, 84, 84, 84, 83, 83, 83, 82, 82, 81, 81, 80, 79, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 68, 67, 66, 64, 63, 61, 60, 58, 56, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 30, 28, 26, 24, 21, 19, 17, 14, 12, 9, 7, 4, 2, 1, -1, -2, -4, -7, -9, -12, -14, -17, -19, -21, -24, -26, -28, -30, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, -55, -56, -58, -60, -61, -63, -64, -66, -67, -68, -70, -71, -72, -73, -74, -75, -76, -77, -78, -79, -79, -80, -81, -81, -82, -82, -83, -83, -83, -84, -84, -84, -84, -84, -84, -84, -83, -83, -83, -82, -82, -81, -81, -80, -79, -79, -78, -77, -76, -75, -74, -73, -72, -71, -70, -68, -67, -66, -64, -63, -61, -60, -58, -56, -55, -53, -51, -49, -47, -45, -43, -41, -39, -37, -35, -33, -30, -28, -26, -24, -21, -19, -17, -14, -12, -9, -7, -4, -2, -1 };
_43 Cos(_43 a) { _96 sin_lut[(a + 64) % 256]; }
_43 Sin(_43 a) { _96 sin_lut[a % 256]; }
_50 str_cpy(_30* d, _71 _30* s) { _114(*s) *d++ = *s++; *d=0; }
_43 str_len(_71 _30* s) { _43 l=0; _114(*s++)l++; _96 l; }
_44 is_over(_43 mx, _43 my, _43 ox, _43 oy, _43 r) { _96 (mx-ox)*(mx-ox) + (my-oy)*(my-oy) < r*r; }
_44 is_over_rect(_43 mx, _43 my, _43 x, _43 y, _43 w, _43 h) { _96 (mx >= x AND mx <= x+w AND my >= y AND my <= y+h); }
/// TEXT ENGINE HELPER FÜR FENSTER
bool str_equal(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (*(const unsigned char*)s1 == *(const unsigned char*)s2);
}
bool str_starts(const char* full, const char* prefix) {
    while(*prefix) { if(*prefix++ != *full++) return false; }
    return true;
}
void print_win(Window* win, const char* text) {
    while (*text && win->cursor_pos < 2000) { win->content[win->cursor_pos++] = *text++; }
    win->content[win->cursor_pos] = 0;
}
void hex_to_str(uint32_t val, char* buf) {
    const char hex_chars[] = "0123456789ABCDEF"; buf[0] = '0'; buf[1] = 'x'; buf[10] = '\0';
    for(int i = 7; i >= 0; i--) { buf[i + 2] = hex_chars[val & 0xF]; val >>= 4; }
}
/// ==========================================
/// BARE METAL FIX: CFS & AHCI GLOBALS
/// ==========================================
uint32_t active_ahci_bar5 = 0;
uint32_t active_sata_port = 0;
uint32_t detected_ports[8]; 
int detected_port_count = 0;
int selected_drive_idx = -1;
// ==========================================
/// OS1 ORIGINAL CFS SYSTEM (16-Bit LBA & Size)
/// ==========================================
struct CFS_DIR_ENTRY { 
    uint8_t type;          /// 1 Byte: 0 = Leer, 1 = Datei
    char filename[11];     /// 11 Byte: Name (8.3 Format)
    uint16_t start_lba;    /// 2 Byte: Start Sektor (-00-)
    uint16_t file_size;    /// 2 Byte: Dateigröße in KB (-00-)
} __attribute__((packed)); /// Exakt 16 Bytes pro Eintrag!
struct FileEntry { uint8_t exists; char name[12]; uint16_t size; uint16_t start_lba; };
FileEntry cfs_files[8];
uint32_t active_file_lba = 0;
uint32_t active_file_idx = 0;
_44 dsk_mgr_opened = _86; /// BARE METAL FIX: Speichert, ob wir im Datei-Explorer sind!
/// NEU: Speicher für die Laufwerksgröße
uint32_t drive_total_gb = 0;
uint32_t drive_used_kb = 0;
bool is_mounted = false;
void int_to_str(uint32_t n, char* s) { if(n==0){s[0]='0';s[1]=0;return;} int i=0; uint32_t t=n; while(t>0){t/=10;i++;} s[i]=0; while(n>0){s[--i]=(n%10)+'0';n/=10;} }
/// ==========================================
/// BARE METAL FIX: 64-BIT KEYBOARD IDT
/// ==========================================
struct IDTEntry { 
    uint16_t offset_low; 
    uint16_t selector; 
    uint8_t ist; 
    uint8_t type_attr; 
    uint16_t offset_mid; 
    uint32_t offset_high; 
    uint32_t zero; 
} __attribute__((packed));

struct IDTPtr { uint16_t limit; uint64_t base; } __attribute__((packed));
IDTEntry idt[256]; IDTPtr idt_ptr;

void set_idt_gate(int n, uint64_t handler) { 
    idt[n].offset_low = handler & 0xFFFF; 
    idt[n].selector = 0x08; 
    idt[n].ist = 0; 
    idt[n].type_attr = 0x8E; 
    idt[n].offset_mid = (handler >> 16) & 0xFFFF; 
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF; 
    idt[n].zero = 0; 
}
void remap_pic() { outb(0x20, 0x11); outb(0xA0, 0x11); outb(0x21, 0x20); outb(0xA1, 0x28); outb(0x21, 0x04); outb(0xA1, 0x02); outb(0x21, 0x01); outb(0xA1, 0x01); outb(0x21, 0xFD); outb(0xA1, 0xFF); }
struct interrupt_frame;
/// Globale Variable für die Markierung im Fenster
int sys_selected_item = 0; 
int sys_max_items = 5; /// Passe diese Zahl an deine Anzahl von Menüpunkten an!
uint8_t key_scancode = 0;
bool key_ready = false;
__attribute__((interrupt, target("general-regs-only"))) 
void dummy_isr(struct interrupt_frame* frame) { 
    outb(0x20, 0x20); /// Dem Mainboard sagen: "Alles gut, wir ignorieren das."
}
__attribute__((interrupt, target("general-regs-only"))) 
void keyboard_isr(struct interrupt_frame* frame) { 
    key_scancode = inb(0x60); 
    key_ready = true; 
    outb(0x20, 0x20); 
}
_30 get_ascii_qwertz(_184 sc) {
    _30 k_low[] = { 0,27,'1','2','3','4','5','6','7','8','9','0',0,0,'\b','\t','q','w','e','r','t','z','u','i','o','p',0,0,'\n',0,'a','s','d','f','g','h','j','k','l',0,0,0,0,0,'y','x','c','v','b','n','m',',','.','-',0,0,0,' ' };
    _15 (sc < sizeof(k_low)) _96 k_low[sc]; _96 0;
}
/// ==========================================
/// BARE METAL FIX: AHCI SATA DRIVER (32-BIT)
/// ==========================================
/// Die Hardware-Struktur eines SATA-Ports im Arbeitsspeicher
struct HBA_PORT {
    uint32_t clb, clbu, fb, fbu, is, ie, cmd, res0, tfd, sig, ssts, sctl, serr, sact, ci, sntf, fbs, res1[11], vendor[4];
};
/// Das Haupt-Gehirn des AHCI Controllers
struct HBA_MEM {
    uint32_t cap, ghc, is, pi, vs, ccc_ctl, ccc_pts, em_loc, em_ctl, cap2, bohc;
    uint8_t res[0xA0 - 0x2C];
    uint8_t vendor[0x100 - 0xA0];
    HBA_PORT ports[32];
};
uint32_t pci_read(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, address);
    return inl(0xCFC);
}
void pci_write(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset, uint32_t val) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, address);
    outl(0xCFC, val);
}
/// ==========================================
/// BARE METAL FIX: KUGELSICHERER AHCI TREIBER
/// ==========================================
struct HBA_CMD_HEADER { uint16_t flags; uint16_t prdtl; uint32_t prdbc; uint32_t ctba; uint32_t ctbau; uint32_t res1[4]; };
struct HBA_PRDT_ENTRY { 
    uint32_t dba; 
    uint32_t dbau; 
    uint32_t res1; /// Hier von rsv0 auf res1 geändert
    uint32_t dbc; 
};
struct HBA_CMD_TBL { uint8_t cfis[64]; uint8_t acmd[16]; uint8_t rsv[48]; HBA_PRDT_ENTRY prdt_entry[1]; };
void ahci_init_port(HBA_PORT* port, int port_no) {
    HBA_MEM* hba = (HBA_MEM*)active_ahci_bar5;
    hba->ghc |= (1 << 31); /// AHCI Global Enable
    hba->is = 0xFFFFFFFF;
    port->cmd &= ~1; int w=0; while((port->cmd & (1<<15)) && w++<100000);
    port->cmd &= ~(1<<4); w=0; while((port->cmd & (1<<14)) && w++<100000);
    uint32_t base = 0x00800000 + (port_no * 0x10000);
    for(int i=0; i<0x10000; i++) ((uint8_t*)base)[i] = 0;
    port->clb = base; port->clbu = 0;
    port->fb = base + 0x400; port->fbu = 0;
    HBA_CMD_HEADER* cmdh = (HBA_CMD_HEADER*)port->clb;
    for(int i=0; i<32; i++) { 
        cmdh[i].prdtl = 1; 
        cmdh[i].ctba = base + 0x1000 + (i * 0x100); 
        cmdh[i].ctbau = 0; 
    }
    port->ie = 0xFFFFFFFF; port->serr = 0xFFFFFFFF; port->is = 0xFFFFFFFF;   
    port->cmd |= (1<<4); port->cmd |= 1; 
}
int ahci_rw(uint32_t lba, uint32_t buffer_addr, int is_write) {
    if(active_ahci_bar5 == 0) return 0;
    HBA_PORT* port = &((HBA_MEM*)active_ahci_bar5)->ports[active_sata_port];
    port->is = 0xFFFFFFFF; 
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)port->clb;
    cmdheader[0].flags = 5 | (is_write ? (1 << 6) : 0) | (1 << 16); 
    cmdheader[0].prdtl = 1;
    cmdheader[0].prdbc = 0;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)cmdheader[0].ctba;
    for(int i=0; i<128; i++) ((uint8_t*)cmdtbl)[i] = 0;
    cmdtbl->prdt_entry[0].dba = buffer_addr; cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = 511; 
    cmdtbl->prdt_entry[0].res1 = 0; /// Geändert auf res1
    uint8_t* fis = (uint8_t*)cmdtbl->cfis;
    fis[0] = 0x27; fis[1] = 0x80; fis[2] = is_write ? 0xCA : 0xC8; 
    fis[4] = lba & 0xFF; fis[5] = (lba >> 8) & 0xFF; fis[6] = (lba >> 16) & 0xFF;
    fis[7] = 0x40 | ((lba >> 24) & 0x0F); fis[12] = 1; 
    int spin = 0; while((port->tfd & (0x80 | 0x08)) && spin++ < 1000000);
    if(spin >= 1000000) return 0;
    port->ci = 1; 
    while(1) { if((port->ci & 1) == 0) break; if(port->is & (1 << 30)) return 0; }
    if(port->tfd & 0x01) return 0;
    return 1;
}
/// Namen auf Sectors (Mehrzahl) geändert
int ahci_read_sectors(uint32_t lba, uint32_t buffer_addr) { return ahci_rw(lba, buffer_addr, 0); }
int ahci_write_sectors(uint32_t lba, uint32_t buffer_addr) { return ahci_rw(lba, buffer_addr, 1); }
int ahci_identify(uint32_t buffer_addr) {
    if(active_ahci_bar5 == 0) return 0;
    HBA_PORT* port = &((HBA_MEM*)active_ahci_bar5)->ports[active_sata_port];
    port->is = 0xFFFFFFFF; port->serr = 0xFFFFFFFF;
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)port->clb;
    cmdheader[0].flags = 5; cmdheader[0].prdtl = 1;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)cmdheader[0].ctba;
    for(int i=0; i<80; i++) ((uint8_t*)cmdtbl)[i] = 0;
    cmdtbl->prdt_entry[0].dba = buffer_addr; cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].res1 = 0; /// Geändert auf res1
    cmdtbl->prdt_entry[0].dbc = 511;
    uint8_t* fis = (uint8_t*)cmdtbl->cfis;
    fis[0] = 0x27; fis[1] = 0x80; fis[2] = 0xEC; 
    int spin = 0; while((port->tfd & (0x80 | 0x08)) && spin < 1000000) spin++;
    port->ci = 1;
    while(1) { if((port->ci & 1) == 0) break; if(port->is & (1<<30)) return 0; }
    return 1;
}
/// ==========================================
/// BARE METAL FIX: SYSTEM CONTROL & INFO
/// ==========================================
char user_name[32] = "COSMOS"; 
char cpu_brand[49] = "SCANNING CPU...";
uint8_t sys_lang = 0; 
uint8_t sys_theme = 0;
/// NEU: Textspeicher für die klickbare Hardware-Liste
char hw_storage[256] = "PRESS TO SCAN";
char hw_net[256]     = "PRESS TO SCAN";
char hw_gpu[256]     = "PRESS TO SCAN";
char hw_usb[256]     = "PRESS TO SCAN";
void get_cpu_brand() { 
    uint32_t a, b, c, d; 
    
    /// BARE METAL FIX: Auch die erste Abfrage muss abgesichert werden!
    __asm__ volatile (
        "pushq %%rbx \n\t"
        "cpuid \n\t"
        "popq %%rbx \n\t"
        : "=a"(a)
        : "0"(0x80000000)
        : "rcx", "rdx"
    );
    
    if(a < 0x80000004) { str_cpy(cpu_brand,"GENERIC X86"); return; } 
    
    char* s = cpu_brand; 
    for(uint32_t i=0x80000002; i<=0x80000004; i++){
        /// BARE METAL FIX: Der kugelsichere 64-Bit CPUID Aufruf!
        __asm__ volatile (
            "pushq %%rbx \n\t"
            "cpuid \n\t"
            "movl %%ebx, %1 \n\t"
            "popq %%rbx \n\t"
            : "=a"(a), "=r"(b), "=c"(c), "=d"(d)
            : "0"(i)
        );
        *(uint32_t*)s=a; s+=4; *(uint32_t*)s=b; s+=4; *(uint32_t*)s=c; s+=4; *(uint32_t*)s=d; s+=4;
    } 
    cpu_brand[48]=0;
    
    int write_idx = 0; int space_count = 0;
    for(int i=0; i<48; i++) {
        char ch = cpu_brand[i]; if(ch == 0) break;
        if(ch >= 'a' && ch <= 'z') ch -= 32; 
        if((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
            cpu_brand[write_idx++] = ch; space_count = 0;
        } else if(ch == ' ' || ch == '@') {
            if(space_count == 0 && write_idx > 0) cpu_brand[write_idx++] = ' ';
            space_count++;
        }
    }
    cpu_brand[write_idx] = 0;
}
void system_reboot() { outb(0x64, 0xFE); }
void system_shutdown() {
    outw(0xB004, 0x2000); /// QEMU / Bochs Power-Off
    outw(0x4004, 0x3400); /// VirtualBox Power-Off
    while(1) asm volatile("cli; hlt"); /// CPU anhalten
}
/// BARE METAL FIX: Echter Hardware-Scanner (Scannt ALLE Funktionen 0-7)
void scan_pci_class(uint8_t target_class, char* out_buf, const char* prefix) {
    for(uint16_t b=0; b<256; b++) {
        for(uint16_t s=0; s<32; s++) {
            for(uint16_t f=0; f<8; f++) { /// <-- WICHTIG: Laptop-Chips liegen oft auf Func 1-7!
                uint32_t vd = pci_read(b,s,f,0);
                if((vd & 0xFFFF) != 0xFFFF) {
                    uint32_t cls = pci_read(b,s,f,8);
                    if(((cls >> 24) & 0xFF) == target_class) {
                        str_cpy(out_buf, prefix);
                        int len = str_len(out_buf);
                        out_buf[len++] = ' '; out_buf[len++] = '[';
                        char hex[12]; hex_to_str(vd, hex);
                        for(int i=0; i<10; i++) out_buf[len++] = hex[i];
                        out_buf[len++] = ']'; out_buf[len] = 0;
                        return; /// Erster Treffer wird genommen!
                    }
                }
            }
        }
    }
    str_cpy(out_buf, "NOT FOUND ON PCI BUS");
}
void scan_pci_drives(Window* dsk_win) {
    print_win(dsk_win, "COSMOS AHCI SCANNER V6\n--------------------------\n");
    detected_port_count = 0; selected_drive_idx = -1;
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint16_t slot = 0; slot < 32; slot++) {
            for (uint16_t func = 0; func < 8; func++) {
                uint32_t vd = pci_read(bus, slot, func, 0);
                if ((vd & 0xFFFF) != 0xFFFF) {
                    uint32_t class_sub = pci_read(bus, slot, func, 8);
                    if (((class_sub >> 24) & 0xFF) == 0x01 && ((class_sub >> 16) & 0xFF) == 0x06) {
                        /// BARE METAL FIX: PCI Bus Mastering zwingend aktivieren!
                        uint32_t cmd = pci_read(bus, slot, func, 0x04);
                        /// Bit 1: Memory Space, Bit 2: Bus Master (DMA) erzwingen
                        pci_write(bus, slot, func, 0x04, cmd | 0x06);
                        uint32_t bar5 = pci_read(bus, slot, func, 0x24) & 0xFFFFFFF0;
                        active_ahci_bar5 = bar5;
                        HBA_MEM* hba = (HBA_MEM*)bar5;
                        for(int i = 0; i < 32; i++) {
                            if(hba->pi & (1 << i)) {
                                uint32_t ssts = hba->ports[i].ssts;
                                if((ssts & 0x0F) == 3 && ((ssts >> 8) & 0x0F) == 1) {
                                    if(detected_port_count < 8) {
                                        detected_ports[detected_port_count++] = i;
                                        print_win(dsk_win, "PORT ");
                                        char p_str[2] = {(char)('0' + i), 0}; print_win(dsk_win, p_str);
                                        /// Echte Hardware-Signatur auslesen!
                                        if(hba->ports[i].sig == 0x00000101) print_win(dsk_win, ": SATA HDD\n");
                                        else if(hba->ports[i].sig == 0xEB140101) print_win(dsk_win, ": CD/DVD ROM\n");
                                        else print_win(dsk_win, ": UNKNOWN\n");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (detected_port_count == 0) print_win(dsk_win, "NO DRIVES FOUND.\n");
    else print_win(dsk_win, "\nSELECT A DRIVE TO MOUNT.\n");
}
_50 focus_window(_43 id) { 
    _43 found_at = -1; 
    _39(_43 i=0; i<13; i++) _15(win_z[i] EQ id) found_at = i; 
    _15(found_at EQ -1) _96;
    _39(_43 i=found_at; i<12; i++) win_z[i] = win_z[i+1]; 
    win_z[12] = id; 
}

/// GANZ OBEN IN DER DATEI (Globaler Speicher für den Cursor)
_43 v_cx = 400;
_43 v_cy = 300;

/// ==========================================
/// BARE METAL FIX: SAFE KEYBOARD POLLING (NO MOUSE!)
/// ==========================================
_50 handle_input() { 
    mouse_just_pressed = _86;
    
    /// Wir lesen nur maximal 8 Bytes pro Frame, um Überlastung zu vermeiden.
    for(int i = 0; i < 8; i++) {
        _184 st = inb(0x64);
        
        /// Ist überhaupt ein Byte im Puffer? (Bit 0)
        if ((st & 1) == 0) break; 
        
        _184 d = inb(0x60);
        
        /// Ist es ein Maus-Byte? (Bit 5). Wenn ja -> MÜLLSCHLUCKER! (Ignorieren!)
        if (st & 0x20) continue; 
        
        /// Es ist ein reines Tastatur-Byte!
        key_scancode = d;
        key_ready = _128;
    }
}

/// ==========================================
/// 5. GRAPHICS ENGINE & SHADER 
/// ==========================================
_50 Put(_43 x, _43 y, _89 c) { _15(x<0 OR x>=800 OR y<0 OR y>=600) _96; bb[y*800+x]=c; }
_50 PutAlpha(_43 x, _43 y, _89 c) { _15(x<0 OR x>=800 OR y<0 OR y>=600) _96; _89 bg = bb[y*800+x]; _89 s1 = ((c & 0xFEFEFE) >> 1) + ((bg & 0xFEFEFE) >> 1); bb[y*800+x] = ((s1 & 0xFEFEFE) >> 1) + ((bg & 0xFEFEFE) >> 1); }
_50 Swap() { _39(_43 i=0; i<800*600; i++) fb[i] = bb[i]; }
_50 DrawRoundedRect(_43 x, _43 y, _43 rw, _43 rh, _43 r, _89 c) { _39(_43 iy=0;iy<rh;iy++)_39(_43 ix=0;ix<rw;ix++){ _44 corn=_86; _15(ix<r AND iy<r AND (r-ix)*(r-ix)+(r-iy)*(r-iy)>r*r) corn=_128; _15(ix>rw-r AND iy<r AND (ix-(rw-r))*(ix-(rw-r))+(r-iy)*(r-iy)>r*r) corn=_128; _15(ix<r AND iy>rh-r AND (r-ix)*(r-ix)+(iy-(rh-r))*(iy-(rh-r))>r*r) corn=_128; _15(ix>rw-r AND iy>rh-r AND (ix-(rw-r))*(ix-(rw-r))+(iy-(rh-r))*(iy-(rh-r))>r*r) corn=_128; _15(!corn) Put(x+ix,y+iy,c); } }
_50 DrawGlassRect(_43 x, _43 y, _43 rw, _43 rh, _43 r, _89 c) { 
    _43 cr = (c >> 16) & 0xFF; _43 cg = (c >> 8) & 0xFF; _43 cb = c & 0xFF;
    _39(_43 iy=0; iy<rh; iy++) {
        _39(_43 ix=0; ix<rw; ix++) {
            _44 corn=_86; 
            _15(ix<r AND iy<r AND (r-ix)*(r-ix)+(r-iy)*(r-iy)>r*r) corn=_128; 
            _15(ix>rw-r AND iy<r AND (ix-(rw-r))*(ix-(rw-r))+(r-iy)*(r-iy)>r*r) corn=_128; 
            _15(ix<r AND iy>rh-r AND (r-ix)*(r-ix)+(iy-(rh-r))*(iy-(rh-r))>r*r) corn=_128; 
            _15(ix>rw-r AND iy>rh-r AND (ix-(rw-r))*(ix-(rw-r))+(iy-(rh-r))*(iy-(rh-r))>r*r) corn=_128;
            _15(!corn) {
                _43 sx = x + ix; _43 sy = y + iy;
                _15(sx >= 0 AND sx < 800 AND sy >= 0 AND sy < 600) {
                    _89 bg = bb[sy * 800 + sx];
                    _43 bg_r = (bg >> 16) & 0xFF; _43 bg_g = (bg >> 8) & 0xFF; _43 bg_b = bg & 0xFF;
                    _43 f_r = (bg_r + cr) >> 1; _43 f_g = (bg_g + cg) >> 1; _43 f_b = (bg_b + cb) >> 1;
                    bb[sy * 800 + sx] = (f_r << 16) | (f_g << 8) | f_b;
                }
            }
        }
    }
}
_50 DrawChar(_43 x, _43 y, _30 c, _89 col, _44 bold) { 
    _72 _71 _184 f_u[] = { 0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22, 0x7F,0x41,0x41,0x22,124, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x49,0x49,0x7A, 0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E, 0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F, 0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43 };
    _72 _71 _184 f_n[] = { 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31, 0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03, 0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E };
    _71 _184* ptr = 0;
    _15(c >= 'A' AND c <= 'Z') ptr = &f_u[(c-'A')*5]; _41 _15(c >= '0' AND c <= '9') ptr = &f_n[(c-'0')*5];
    _41 _15(c EQ ':') { _72 _184 s[]={0,0x36,0x36,0,0}; ptr=s; }
    _41 _15(c EQ '.') { _72 _184 s[]={0,0x60,0x60,0,0}; ptr=s; }
    _15(!ptr) _96;
    _89 glow_col = (col < 0x555555) ? 0xFFFFFF : 0x000000;
    _39(_43 m=0;m<5;m++){ 
        _184 l=ptr[m]; 
        _39(_43 n=0;n<7;n++) {
            _15((l>>n)&1) { 
                PutAlpha(x+m-1, y+n-1, glow_col); PutAlpha(x+m, y+n-1, glow_col); PutAlpha(x+m+1, y+n-1, glow_col);
                PutAlpha(x+m-1, y+n,   glow_col);                                 PutAlpha(x+m+1, y+n,   glow_col);
                PutAlpha(x+m-1, y+n+1, glow_col); PutAlpha(x+m, y+n+1, glow_col); PutAlpha(x+m+1, y+n+1, glow_col);
                _15(bold) PutAlpha(x+m+2, y+n, glow_col);
            } 
        } 
    }
    _39(_43 m=0;m<5;m++){ _184 l=ptr[m]; _39(_43 n=0;n<7;n++) _15((l>>n)&1) { Put(x+m, y+n, col); _15(bold) Put(x+m+1, y+n, col); } } 
}
_50 Text(_43 x, _43 y, _71 _30* s, _89 col, _44 bold) { 
    _15(!s) _96; _43 ox = x;
    _114(*s) { 
        _15(*s EQ '\n') { y += 15; x = ox; s++; continue; }
        DrawChar(x,y,*s++,col,bold); x+=(bold?7:6); 
    } 
}
_50 TextC(_43 cp, _43 y, _71 _30* s, _89 col, _44 bold) { _15(!s) _96; _43 l=0; _114(s[l])l++; Text(cp-(l*(bold?7:6))/2, y, s, col, bold); }
_50 DrawAeroCursor(_43 mx, _43 my) {
    _72 _71 _30* c_map[17] = {
        "*", "**", "*.*", "*..*", "*...*", "*....*", "*.....*", "*......*",
        "*.......*", "*........*", "*.........*", "*......****", "*...*..*",
        "*..* *..*", "*.* *..*", "** *..*", "        **"
    };
    _39(_43 y = 0; y < 17; y++) { _43 len = str_len(c_map[y]); _39(_43 x = 0; x < len; x++) _15(c_map[y][x] NEQ ' ') PutAlpha(mx + x + 3, my + y + 4, 0x000000); }
    _39(_43 y = 0; y < 17; y++) { _43 len = str_len(c_map[y]); _39(_43 x = 0; x < len; x++) { _15(c_map[y][x] EQ '*') Put(mx + x, my + y, 0x000000); _41 _15(c_map[y][x] EQ '.') Put(mx + x, my + y, 0xFFFFFF); } }
}
_50 DrawDenseGalaxy(_43 cx, _43 cy, _43 exp) {
    _15 (exp <= 5) _96; _43 max_radius = (400 * exp) / 320; _89 l_seed = 123456; 
    _39(_43 i = 0; i < 8000; i++) {
        l_seed = (l_seed * 1103515245 + 12345) & 0x7FFFFFFF; _43 rand_val1 = l_seed % max_radius;
        l_seed = (l_seed * 1103515245 + 12345) & 0x7FFFFFFF; _43 rand_val2 = l_seed % 30;
        l_seed = (l_seed * 1103515245 + 12345) & 0x7FFFFFFF; _43 rand_val3 = l_seed % 256; 
        _43 d = rand_val1; _15(d < 50) continue; 
        _43 scatter_x = (l_seed % 7) - 3; l_seed = (l_seed * 1103515245 + 12345) & 0x7FFFFFFF; _43 scatter_y = (l_seed % 7) - 3;
        _43 angle = 0; _43 intensity_mod = 1;
        _15(i % 3 EQ 0) { _43 target_d = 60 + ((d / 50) * 50); d = target_d + (rand_val2 - 15); angle = (rand_val3 - (frame/4) + 256) % 256; intensity_mod = 2; }
        _41 { d = (d * d) / max_radius; _15(d < 50) d = 50 + (l_seed % 20); angle = (((i % 2) * 128) + (d / 2) - (frame / 3) + 256) % 256; }
        _43 final_d = d + (rand_val2 - 15); _43 final_a = (angle + (l_seed % 10) - 5 + 256) % 256;
        _43 px = cx + (Cos(final_a) * final_d) / 84 + scatter_x; _43 py = cy + (Sin(final_a) * final_d * 3 / 4) / 84 + scatter_y;
        _15(px < 0 OR px >= 800 OR py < 0 OR py >= 600) continue;
        _43 r = 0, g = 0, b = 0;
        _15(d < 100) { r = 255; g = 180 - d; b = 60; } _41 _15(d < 180) { r = 160 - (d - 100); g = 50; b = 255; } _41 { r = 20; g = 30; b = 255 - (d - 180); }
        _89 bg = bb[py * 800 + px]; _43 bg_r = (bg >> 16) & 0xFF; _43 bg_g = (bg >> 8) & 0xFF; _43 bg_b = bg & 0xFF;
        _43 intensity = 255 - (int_sqrt(rand_val2*rand_val2) * 8); _15(intensity < 0) intensity = 0; intensity = (intensity * intensity_mod) / 2;
        _15(i % 100 EQ 0) { r = 255; g = 255; b = 255; intensity = 255; }
        _43 f_r = bg_r + (r * intensity / 256); _15(f_r > 255) f_r = 255; _43 f_g = bg_g + (g * intensity / 256); _15(f_g > 255) f_g = 255; _43 f_b = bg_b + (b * intensity / 256); _15(f_b > 255) f_b = 255;
        bb[py * 800 + px] = (f_r << 16) | (f_g << 8) | f_b;
    }
}
_50 DrawGoldenSun(_43 cx, _43 cy, _43 radius) {
    _43 r2 = radius * radius; _43 glow_radius = radius + 15; _43 glow_r2 = glow_radius * glow_radius;
    _39(_43 y = -glow_radius; y <= glow_radius; y++) {
        _39(_43 x = -glow_radius; x <= glow_radius; x++) {
            _43 dist_sq = x*x + y*y; _43 screen_x = cx + x; _43 screen_y = cy + y;
            _15(screen_x < 0 OR screen_x >= 800 OR screen_y < 0 OR screen_y >= 600) _101;
            _15(dist_sq <= r2) {
                _43 nz = int_sqrt(r2 - dist_sq) * 255 / radius; _43 hx = x + (radius / 3); _43 hy = y + (radius / 3);
                _43 highlight = 0; _15(int_sqrt(hx*hx + hy*hy) < radius) { highlight = 255 - (int_sqrt(hx*hx + hy*hy) * 255 / radius); highlight = (highlight * highlight) / 255; }
                _43 r = (nz * 255) / 255; _43 g = (nz * 170) / 255; _43 b = (nz * 20) / 255;  
                r += highlight; _15(r > 255) r = 255; g += highlight; _15(g > 255) g = 255; b += (highlight / 2); _15(b > 255) b = 255;
                bb[screen_y * 800 + screen_x] = (r << 16) | (g << 8) | b;
            } _41 _15 (dist_sq <= glow_r2 AND dist_sq > r2) {
                _43 alpha = 255 - ((int_sqrt(dist_sq) - radius) * 255 / (glow_radius - radius)); alpha = (alpha * alpha) / 255;
                _15(alpha > 0) {
                    _89 bg = bb[screen_y * 800 + screen_x];
                    _43 final_r = (255 * alpha + ((bg >> 16) & 0xFF) * (255 - alpha)) / 255; _43 final_g = (120 * alpha + ((bg >> 8) & 0xFF) * (255 - alpha)) / 255; _43 final_b = (0 * alpha + (bg & 0xFF) * (255 - alpha)) / 255;
                    bb[screen_y * 800 + screen_x] = (final_r << 16) | (final_g << 8) | final_b;
                }
            }
        }
    }
}
_50 DrawOrganicPlanet(_43 cx, _43 cy, _43 radius, _89 base_col) {
    _43 r2 = radius * radius; _43 glow_radius = radius + 8; _43 glow_r2 = glow_radius * glow_radius;
    _43 base_r = (base_col >> 16) & 0xFF; _43 base_g = (base_col >> 8) & 0xFF; _43 base_b = base_col & 0xFF;
    _39(_43 y = -glow_radius; y <= glow_radius; y++) {
        _39(_43 x = -glow_radius; x <= glow_radius; x++) {
            _43 dist_sq = x*x + y*y; _43 screen_x = cx + x; _43 screen_y = cy + y;
            _15(screen_x < 0 OR screen_x >= 800 OR screen_y < 0 OR screen_y >= 600) _101;
            _15(dist_sq <= r2) {
                _43 nz = int_sqrt(r2 - dist_sq) * 255 / radius; _43 edge_dist = 255 - nz;
                _43 light_x = x + (radius / 2); _43 light_y = y + (radius / 2); _43 l_dist_sq = light_x*light_x + light_y*light_y; _43 diffuse = 0;
                _15(l_dist_sq < r2) diffuse = 255 - (int_sqrt(l_dist_sq) * 255 / radius);
                _43 noise = (((x + radius) * 17) + ((y + radius) * 31)) % 20; _43 banding = (Sin(((y + radius) * 100) / radius) + 64) / 8;
                _43 r = (base_r * nz) / 255; _43 g = (base_g * nz) / 255; _43 b = (base_b * nz) / 255;
                r += (diffuse * base_r) / 256; g += (diffuse * base_g) / 256; b += (diffuse * base_b) / 256;
                _43 rim = (edge_dist * edge_dist) / 255; r += (rim * base_r) / 512; g += (rim * base_g) / 512; b += (rim * base_b) / 512;
                r = (r * (220 + noise + banding)) / 256; g = (g * (220 + noise + banding)) / 256; b = (b * (220 + noise + banding)) / 256;
                _15(r > 255) r = 255; _15(g > 255) g = 255; _15(b > 255) b = 255;
                bb[screen_y * 800 + screen_x] = (r << 16) | (g << 8) | b;
            } _41 _15 (dist_sq <= glow_r2 AND dist_sq > r2) {
                _43 alpha = 255 - ((int_sqrt(dist_sq) - radius) * 255 / (glow_radius - radius)); alpha = (alpha * alpha) / 255;
                _15(alpha > 0) {
                    _89 bg = bb[screen_y * 800 + screen_x];
                    _43 final_r = (base_r * alpha + ((bg >> 16) & 0xFF) * (255 - alpha)) / 255; _43 final_g = (base_g * alpha + ((bg >> 8) & 0xFF) * (255 - alpha)) / 255; _43 final_b = (base_b * alpha + (bg & 0xFF) * (255 - alpha)) / 255;
                    bb[screen_y * 800 + screen_x] = (final_r << 16) | (final_g << 8) | final_b;
                }
            }
        }
    }
}
/// ==========================================
/// BARE METAL FIX: CMD Processor & App Toggles
/// ==========================================
/// Forward-Deklarationen (sagen C++, dass diese Dinge existieren)
_50 focus_window(_43 id);
extern char cpu_brand[49];
void system_reboot();
extern void system_init_usb();
extern _43 xhci_bot_get_capacity(_184 slot_id); /// BARE METAL FIX: Das Orakel-Radar für SCSI anmelden!
_50 toggle_app(_43 id) {
    Window* win = &windows[id];
    _15(win->open AND !win->minimized AND win_z[12] EQ win->id) { win->minimized = _128; } 
    _41 { win->open = _128; win->minimized = _86; focus_window(win->id); }
}
char cmd_input_buf[64] = {0};
int cmd_input_idx = 0;
void process_cmd(char* input, Window* cmd_win) {
    /// Befehl nochmal auf dem Bildschirm ausgeben (Echo)
    print_win(cmd_win, "C:\\> "); print_win(cmd_win, input); print_win(cmd_win, "\n");
    if(str_equal(input, "CLS")) {
        cmd_win->cursor_pos = 0; cmd_win->content[0] = 0;
    } 
    else if(str_starts(input, "ECHO ")) {
        print_win(cmd_win, input + 5); print_win(cmd_win, "\n");
    } 
    else if(str_equal(input, "DIR")) {
        print_win(cmd_win, "--- CFS DIRECTORY ---\n");
        int count = 0;
        for(int i=0; i<8; i++) {
            if(cfs_files[i].exists) {
                print_win(cmd_win, cfs_files[i].name);
                print_win(cmd_win, "   [FILE]\n");
                count++;
            }
        }
        if(count == 0) print_win(cmd_win, "NO FILES FOUND (MOUNT DRIVE FIRST)\n");
    } 
    else if(str_equal(input, "SYSINFO")) {
        print_win(cmd_win, "OS: COSMOS V2 (32-BIT PROTECTED MODE)\nCPU: ");
        print_win(cmd_win, cpu_brand); print_win(cmd_win, "\n");
    }
    else if(str_equal(input, "REBOOT")) {
        system_reboot();
    } 
    else if(input[0] != 0) {
        print_win(cmd_win, "UNKNOWN COMMAND OR BAD SYNTAX.\n");
    }
}

/// ==========================================
/// 6. DER HAUPT-EINSTIEG 
/// ==========================================
extern "C" void main(BootInfo* sys_info) {
    init_heap();
    fb = (_89*)(uint64_t)sys_info->framebuffer_addr;

    /// ==========================================
    /// 1. IDT (Interrupts) SAUBER AUFBAUEN
    /// ==========================================
    idt_ptr.limit = sizeof(IDTEntry) * 256 - 1;
    idt_ptr.base = (uint64_t)&idt[0];
    
    /// BARE METAL FIX: Alle Exceptions auf 0 lassen! Sonst crasht QEMU!
    for(int i = 0; i < 256; i++) set_idt_gate(i, 0);
    
    /// Nur die beiden echten Hardware-Interrupts anmelden:
    set_idt_gate(33, (uint64_t)keyboard_isr); /// IRQ 1 (Tastatur)
    set_idt_gate(39, (uint64_t)dummy_isr);    /// IRQ 7 (Geister-Signale vom Mainboard abfangen)
    
    remap_pic();
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
    //__asm__ volatile("sti");
    /// ==========================================
    /// DEIN ORIGINAL-CODE STARTET AB HIER WIEDER:
    /// ==========================================
    read_rtc();
    get_cpu_brand();
    usb_mouse_callback = update_mouse_position;
    init_mouse();
    /// Sterne generieren
    _39(_43 i=0; i<200; i++) {
        stars[i].x = (random() % 1599) - 799;
        stars[i].y = (random() % 1199) - 599;
        stars[i].z = (random() % 1000) + 1;
        stars[i].type = random() % 1000; 
        stars[i].speed = (random() % 4) + 2;
    }
    /// ==========================================
    /// BARE METAL FIX: FENSTER SAUBER INITIALISIEREN
    /// ==========================================
    _39(_43 i=0; i<13; i++) { windows[i].id = i; windows[i].open = _86; windows[i].minimized = _86; windows[i].cursor_pos = 0; windows[i].content[0] = 0; }
    str_cpy(windows[0].title, "NOTEPAD");  windows[0].x=100; windows[0].y=100; windows[0].w=400; windows[0].h=300; windows[0].color=0xEEEEEE; 
    str_cpy(windows[1].title, "APPS");     windows[1].x=150; windows[1].y=150; windows[1].w=350; windows[1].h=250; windows[1].color=0xDDDDDD; 
    str_cpy(windows[3].title, "SYSTEM");   windows[3].x=200; windows[3].y=50;  windows[3].w=350; windows[3].h=480; windows[3].color=0xFFD700; 
    str_cpy(windows[4].title, "DISK MGR"); windows[4].x=250; windows[4].y=200; windows[4].w=450; windows[4].h=350; windows[4].color=0x888888; 
    str_cpy(windows[5].title, "CMD");      windows[5].x=100; windows[5].y=300; windows[5].w=450; windows[5].h=250; windows[5].color=0x111111;
    /// BARE METAL FIX: Alle Orakel-Variablen sofort befüllen!
	scan_pci_drives(&windows[4]);
    
    /// 1. Puffer komplett nullen (RAM Müll vernichten!)
    mem_set(hw_storage, 0, 256);
    mem_set(hw_net, 0, 256);
    mem_set(hw_gpu, 0, 256);

    /// 2. Die EINZIGE Scan-Schleife ausführen (pci.cpp)
    pci_scan_all(); 
	//system_init_usb();
    /// 3. Bildschirm-Abschneider (Gegen das Worträtsel)
    hw_storage[35] = 0;
    hw_net[35] = 0;
    hw_gpu[35] = 0;
    hw_usb[35] = 0;
    /// Planeten initialisieren
    _43 clock_dirs[] = {213, 0, 42, 85, 128}; 
    _39(_43 i=0; i<5; i++) { 
        planets[i].ang = clock_dirs[i]; planets[i].dist = 10;
        planets[i].cur_x = 400; planets[i].cur_y = 300;
        str_cpy(planets[i].name, (i==0?(_30*)"TXT":i==1?(_30*)"APP":i==2?(_30*)"SYS":i==3?(_30*)"DSK":(_30*)"CMD")); 
    }
    _43 map_ids[]={0,1,3,4,5}; /// Verknüpfung Planet -> Fenster ID
    _114(1) {
		handle_input();
        //xhci_poll_events_and_mouse();
        _15(input_cooldown > 0) input_cooldown--;
        click_consumed = _86; z_blocked = _86; _44 mouse_handled = _86;
		/// ==========================================
        /// ABTEILUNG: TASTATUR (BACK TO OS1 STABILITY)
        /// ==========================================
        if (key_ready) {
            key_ready = _86; /// Flag sofort zurücksetzen
            _184 sc = key_scancode;

            _15(!(sc & 0x80)) { /// Taste wurde GEDEÜCKT (Make Code)
                
                /// 1. Globale Hotkeys (F-Tasten für Apps)
                _15(sc EQ 0x3B) toggle_app(0); 
                _15(sc EQ 0x3C) toggle_app(1); 
                _15(sc EQ 0x3D) toggle_app(3); 
                _15(sc EQ 0x3E) toggle_app(4); 
                _15(sc EQ 0x3F) toggle_app(5);

                /// 2. Welches Fenster ist ganz oben (Fokus)?
                _43 fw_id = win_z[12]; 
                Window* fw = &windows[fw_id];
                
                _15(fw AND fw->open AND !fw->minimized) {
                    /// --- TASTATUR STEUERT DAS SYSTEM FENSTER (ID 3) ---
                    _15(fw->id EQ 3) {
                        _15(sc EQ 0x50) { // Pfeil Runter
                            if (sys_selected_item < 4) sys_selected_item++; 
                        } 
                        
                        _15(sc EQ 0x48) { // Pfeil Hoch
                            if (sys_selected_item > 0) sys_selected_item--; 
                        } 
                        
                        _15(sc EQ 0x1C) { // ENTER-TASTE
                            
                            /// INDEX 0: DER GROSSE ROTE ORAKEL-BUTTON OBEN
                            if (sys_selected_item == 0) {
                                mirror_count = 0;
                                for(uint32_t b=0; b<256; b++) {
                                    for(uint32_t d=0; d<32; d++) {
                                        for(uint32_t f=0; f<8; f++) {
                                            uint32_t id = pci_read(b, d, f, 0);
                                            if((id & 0xFFFF) != 0xFFFF && id != 0 && mirror_count < 30) {
                                                mirror_list[mirror_count].bus = b;
                                                mirror_list[mirror_count].dev = d;
                                                mirror_list[mirror_count].func = f;
                                                mirror_list[mirror_count].vendor = id & 0xFFFF;
                                                mirror_list[mirror_count].device = id >> 16;
                                                
                                                uint32_t bar0_l = pci_read(b, d, f, 0x10);
                                                uint32_t bar0_h = pci_read(b, d, f, 0x14);
                                                
                                                uint32_t class_rev = pci_read(b, d, f, 0x08);
                                                uint32_t cls = (class_rev >> 24) & 0xFF;
                                                uint32_t sub = (class_rev >> 16) & 0xFF;
                                                
                                                /// Ist es eine echte 64-Bit Adresse?
                                                if((bar0_l & 0x06) == 0x04) { 
                                                    mirror_list[mirror_count].bar0 = ((uint64_t)bar0_h << 32) | (bar0_l & 0xFFFFFFF0);
                                                } else {
                                                    mirror_list[mirror_count].bar0 = bar0_l & 0xFFFFFFF0;
                                                }
                                                
                                                if(cls == 0x0C && sub == 0x03) str_cpy(mirror_list[mirror_count].name, "USB 3.0 (xHCI)");
                                                else if(cls == 0x01 && sub == 0x06) str_cpy(mirror_list[mirror_count].name, "SATA (AHCI)");
                                                else if(cls == 0x02) str_cpy(mirror_list[mirror_count].name, "NETWORK");
                                                else if(cls == 0x03) str_cpy(mirror_list[mirror_count].name, "GRAPHICS");
                                                else str_cpy(mirror_list[mirror_count].name, "SYSTEM DEVICE");
                                                
                                                mirror_count++;
                                            }
                                        }
                                    }
                                }
                                /// WICHTIG: Das Orakel darf sich NUR hier beim Enter-Druck öffnen!
                                show_oracle = true; 
                            }
                            
                            /// INDEX 1, 2, 3: DIE ANDEREN HARDWARE-SCANS (Nur bei Enter!)
                            else if (sys_selected_item == 1) { scan_pci_class(0x01, hw_storage, "CTRL"); }
                            else if (sys_selected_item == 2) { scan_pci_class(0x02, hw_net, "NIC"); }
                            else if (sys_selected_item == 3) { scan_pci_class(0x03, hw_gpu, "GPU"); }
                                
                            /// INDEX 4: DER USB HOST START! (Nur bei Enter!)
                            else if (sys_selected_item == 4) {
                                system_init_usb(); 
                            }
                        }
                    }
                    /// --- NOTEPAD (ID 0) PURE OS1 LOGIC ---
                    _15(fw->id EQ 0) { 
                        _15(sc EQ 0x0E) { _15(fw->cursor_pos > 0) { fw->cursor_pos--; fw->content[fw->cursor_pos] = 0; } } 
                        _41 _15(sc EQ 0x1C) { _15(fw->cursor_pos < 2000) { fw->content[fw->cursor_pos++] = '\n'; fw->content[fw->cursor_pos] = 0; } } 
                        _41 { 
                            _30 c = get_ascii_qwertz(sc);
                            if(c >= 'a' && c <= 'z') c -= 32;
                            _15(c AND fw->cursor_pos < 2000) { fw->content[fw->cursor_pos++] = c; fw->content[fw->cursor_pos] = 0; } 
                        }
                    }
                    
                    /// --- CMD (ID 5) ---
                    _15(fw->id EQ 5) { 
                        _15(sc EQ 0x0E) { _15(cmd_input_idx > 0) { cmd_input_idx--; cmd_input_buf[cmd_input_idx] = 0; } } 
                        _41 _15(sc EQ 0x1C) { process_cmd(cmd_input_buf, fw); cmd_input_idx = 0; cmd_input_buf[0] = 0; } 
                        _41 { 
                            _30 c = get_ascii_qwertz(sc);
                            if(c >= 'a' && c <= 'z') c -= 32;
                            _15(c AND cmd_input_idx < 60) { cmd_input_buf[cmd_input_idx++] = c; cmd_input_buf[cmd_input_idx] = 0; } 
                        }
                    }
                }
            }
        }
        _15(frame % 100 EQ 0) {
            read_rtc();
        }
        /// ==========================================
        /// FENSTER INTERAKTION (DRAG, RESIZE, BUTTONS)
        /// ==========================================
        _15(mouse_down) {
             _15(drag_win NEQ -1) { 
                 windows[drag_win].x = mouse_x - drag_off_x; 
                 windows[drag_win].y = mouse_y - drag_off_y; 
                 mouse_handled=_128; click_consumed=_128; 
             } 
             _41 _15(resize_win NEQ -1) { 
                 _43 nw = mouse_x - windows[resize_win].x; 
                 _43 nh = mouse_y - windows[resize_win].y; 
                 _15(nw > 100) windows[resize_win].w = nw; 
                 _15(nh > 100) windows[resize_win].h = nh; 
                 mouse_handled=_128; click_consumed=_128; 
             }
        } _41 { drag_win = -1; resize_win = -1; }
        _15(!mouse_handled) {
            _39(_43 i=12; i>=0; i--) { 
                _43 k = win_z[i]; Window* win=&windows[k];
                _15(win->open AND !win->minimized) {
                    _43 wx=(win->fullscreen?0:win->x); _43 wy=(win->fullscreen?0:win->y); _43 ww=(win->fullscreen?800:win->w); _43 wh=(win->fullscreen?600:win->h);
                    _15(mouse_x>=wx AND mouse_x<=wx+ww AND mouse_y>=wy AND mouse_y<=wy+wh) {
                        z_blocked = _128;
                        _15(mouse_just_pressed) {
                           click_consumed = _128; 
                           focus_window(k);
                           _43 bx = wx + ww/2; 
                           _15(mouse_y < wy+40) { 
                               _15(mouse_x > bx-70 AND mouse_x < bx-30) win->minimized=_128; 
                               _41 _15(mouse_x > bx-20 AND mouse_x < bx+40) win->fullscreen = !win->fullscreen; 
                               _41 _15(mouse_x > bx+45 AND mouse_x < bx+70) { win->open=_86; }
                               _41 { drag_win = k; drag_off_x = mouse_x - wx; drag_off_y = mouse_y - wy; }
                           }
                           _15(mouse_x > wx+ww-20 AND mouse_y > wy+wh-20) { resize_win = k; }
                        }
                        mouse_handled = _128; _37; 
                    }
                }
            }
        }
        /// ==========================================
        /// 1. DER ABSOLUT SCHWARZE WELTRAUM
        /// ==========================================
        _39(_43 i = 0; i < 800*600; i++) bb[i] = 0x000000;
        _39(_43 i=0; i<200; i++) {
            _43 t = stars[i].type;
            _15(t >= 995) stars[i].z -= 1; _41 _15(t >= 980) stars[i].z -= stars[i].speed; _41 _15(t >= 950) stars[i].z -= (stars[i].speed + 4); _41 stars[i].z -= stars[i].speed;
            _15(stars[i].z <= 0) { 
                stars[i].z = 1000; stars[i].x = (random() % 1599) - 799; stars[i].y = (random() % 1199) - 599;
                stars[i].type = random() % 1000; stars[i].speed = (random() % 4) + 2;
            }
            _43 sx = v_cx + (stars[i].x * 256) / stars[i].z; _43 sy = v_cy + (stars[i].y * 256) / stars[i].z;
            _15(sx >= 0 AND sx < 800 AND sy >= 0 AND sy < 600) {
                _15(t < 900) {
                    _43 intensity = 255 - (stars[i].z / 4); _15(intensity < 0) intensity = 0; _15(intensity > 255) intensity = 255;
                    _89 col = (intensity << 16) | (intensity << 8) | (intensity); Put(sx, sy, col);
                } _41 _15(t < 950) {
                    _43 pulse = (Sin((frame * 5) + i) + 256) / 2; _15(pulse > 255) pulse = 255;
                    _89 col = (pulse << 16) | (pulse << 8) | 255; Put(sx, sy, col);
                } _41 _15(t < 980) {
                    Put(sx, sy, 0xFFFFFF); 
                    _43 tail1_x = v_cx + (stars[i].x * 256) / (stars[i].z + 20); _43 tail1_y = v_cy + (stars[i].y * 256) / (stars[i].z + 20); Put(tail1_x, tail1_y, 0xFF8800); 
                    _43 tail2_x = v_cx + (stars[i].x * 256) / (stars[i].z + 40); _43 tail2_y = v_cy + (stars[i].y * 256) / (stars[i].z + 40); Put(tail2_x, tail2_y, 0xAA0000);
                } _41 _15(t < 995) {
                    _43 r = 3000 / stars[i].z; 
                    _15(r > 0 AND r < 40) {
                        _43 r2 = r*r; _43 focus_x = (sx - v_cx) * r / 400; _43 focus_y = (sy - v_cy) * r / 300;
                        _39(_43 cy_a=-r; cy_a<=r; cy_a++) _39(_43 cx_a=-r; cx_a<=r; cx_a++) _15(cx_a*cx_a+cy_a*cy_a <= r2) {
                            _43 pos_x = cx_a + r; _43 pos_y = cy_a + r; _43 noise = ((pos_x * 17 + pos_y * 31) % 40);
                            _43 lx = cx_a + focus_x; _43 ly = cy_a + focus_y; _43 l_dist = int_sqrt(lx*lx + ly*ly);
                            _43 diffuse = 30; _15(l_dist < r) diffuse += 225 - (l_dist * 225 / r);
                            _43 gray = ((50 + noise) * diffuse) / 256; _15(gray > 255) gray = 255;
                            Put(sx+cx_a, sy+cy_a, (gray<<16)|((gray*9)/10<<8)|((gray*8)/10)); 
                        }
                    }
                } _41 {
                    _43 r = 6000 / stars[i].z;
                    _15(r > 0 AND r < 60) {
                        _39(_43 j=0; j<30; j++) {
                            _43 ang1 = (j * 15 + (frame/3)) % 256; _43 dist = (j * r) / 30;
                            _43 gx1 = sx + (Cos(ang1)*dist)/84; _43 gy1 = sy + (Sin(ang1)*dist*3/4)/84; PutAlpha(gx1, gy1, 0xAA22AA); 
                            _43 ang2 = (ang1 + 128) % 256; _43 gx2 = sx + (Cos(ang2)*dist)/84; _43 gy2 = sy + (Sin(ang2)*dist*3/4)/84; PutAlpha(gx2, gy2, 0x2288AA); 
                        }
                        Put(sx, sy, 0xFFFFFF); 
                    }
                }
            }
        }
        /// Zentrifuge & Sonne rendern
        _15(!z_blocked AND mouse_just_pressed AND !click_consumed AND is_over(mouse_x, mouse_y, v_cx, v_cy, 50)) {
            galaxy_open = !galaxy_open; click_consumed = _128;
        }
        _15(galaxy_open AND galaxy_expansion < 320) galaxy_expansion += 8;
        _15(!galaxy_open AND galaxy_expansion > 0) galaxy_expansion -= 10;
        DrawDenseGalaxy(v_cx, v_cy, galaxy_expansion);
        DrawGoldenSun(v_cx, v_cy, 50);
        /// ==========================================
        /// LIVE RTC (DATUM UND UHRZEIT) IN DER SONNE
        /// ==========================================
        TextC(v_cx, v_cy-15, "COSMOS", 0x000000, _128);
        TextC(v_cx, v_cy+5,  "SYSTEM", 0x000000, _128);
        _30 ts[]="00:00"; 
        ts[0]='0'+rtc_h/10; ts[1]='0'+rtc_h%10; 
        ts[3]='0'+rtc_m/10; ts[4]='0'+rtc_m%10; 
        TextC(v_cx, v_cy+20, ts, 0x000000, _128);
        _30 ds[]="00.00.2000"; 
        ds[0]='0'+rtc_day/10; ds[1]='0'+rtc_day%10; 
        ds[3]='0'+rtc_mon/10; ds[4]='0'+rtc_mon%10; 
        ds[8]='0'+(rtc_year%100)/10; ds[9]='0'+rtc_year%10; 
        TextC(v_cx, v_cy+35, ds, 0x000000, _128);
        /// ==========================================
        /// 2. PLANETEN (MIT FENSTER-VERKNÜPFUNG)
        /// ==========================================
        _39(_43 i=0; i<5; i++) {
            Window* win = &windows[map_ids[i]];
            _43 target_x, target_y; _44 draw_moons = _86;
            _15(win->minimized) { 
                target_x = 250 + (i * 70); target_y = 560; 
            } _41 _15(win->open) { 
                _43 orbit_dist = 60 + i*50; 
                target_x = v_cx + (Cos(planets[i].ang) * orbit_dist) / 84; 
                target_y = v_cy + (Sin(planets[i].ang) * orbit_dist * 3/4) / 84; 
                draw_moons = _128; 
            } _41 {
                _15(galaxy_expansion >= 100) { 
                    _15(planets[i].dist < 60 + i*50) planets[i].dist += 2; 
                    _15(planets[i].dist > 50) { _15(frame % (8+i) EQ 0) planets[i].ang = (planets[i].ang + 1) % 256; } 
                } _41 { 
                    _15(planets[i].dist > 10) planets[i].dist -= 8; 
                }
                target_x = v_cx + (Cos(planets[i].ang) * planets[i].dist) / 84; 
                target_y = v_cy + (Sin(planets[i].ang) * planets[i].dist * 3/4) / 84;
            }
            planets[i].cur_x += (target_x - planets[i].cur_x) / 4; 
            planets[i].cur_y += (target_y - planets[i].cur_y) / 4;
            _15(galaxy_expansion > 10 OR win->minimized OR win->open) {
                _43 px = planets[i].cur_x; _43 py = planets[i].cur_y;
                _44 hov = is_over(mouse_x, mouse_y, px, py, 20);
                _89 p_col = 0x888888;         
                _15(i EQ 0) p_col = 0xA05566; _15(i EQ 1) p_col = 0x44AA88; 
                _15(i EQ 2) p_col = 0x6677CC; _15(i EQ 3) p_col = 0xCC9955; 
                _15(i EQ 4) p_col = 0x8899AA;
                _15(hov AND !z_blocked) {
                    _43 hr = ((p_col >> 16) & 0xFF) + 40; _15(hr>255) hr=255;
                    _43 hg = ((p_col >> 8) & 0xFF) + 40;  _15(hg>255) hg=255;
                    _43 hb = (p_col & 0xFF) + 40;         _15(hb>255) hb=255;
                    DrawOrganicPlanet(px, py, 22, (hr<<16)|(hg<<8)|hb);
                } _41 {
                    DrawOrganicPlanet(px, py, 20, p_col);
                }
                _15(draw_moons) { 
                    DrawOrganicPlanet(px-30, py, 5, 0x8899AA); 
                    DrawOrganicPlanet(px+30, py, 5, 0x8899AA); 
                }
                TextC(px, py-4, planets[i].name, 0xFFFFFF, _128);
                _15(!z_blocked AND mouse_just_pressed AND !click_consumed AND hov) { 
                    _15(win->minimized) win->minimized = _86; 
                    _41 { 
                        win->open = _128; 
                        focus_window(win->id); 
                    }
                    click_consumed = _128;
                }
            }
        }
        /// ==========================================
        /// 3. ACRYLIC GLASS WINDOW RENDERING
        /// ==========================================
        _39(_43 i=0; i<13; i++) {
            _43 k = win_z[i]; 
            Window* win = &windows[k];
            _15(!win->open OR win->minimized) continue;
            _43 wx=(win->fullscreen?0:win->x); 
            _43 wy=(win->fullscreen?0:win->y); 
            _43 ww=(win->fullscreen?800:win->w); 
            _43 wh=(win->fullscreen?600:win->h);
            /// Milchglas-Hintergrund für offene Fenster zeichnen
            DrawGlassRect(wx, wy, ww, wh, 12, win->color);
            /// Rahmen & Highlights
            DrawRoundedRect(wx+12, wy, ww-24, 1, 0, 0x999999);
            DrawRoundedRect(wx, wy+12, 1, wh-24, 0, 0x999999);
            _89 txt_color = (win->color > 0x888888) ? 0x000000 : 0xFFFFFF;
            Text(wx+15, wy+15, win->title, txt_color, _128);
            /// Fenster-Buttons
            _43 bx = wx + ww/2; 
            Text(bx-60, wy+15, "MIN", 0x555555, _128);
            Text(bx-10, wy+15, "FULL", 0x555555, _128); 
            Text(bx+50, wy+15, "X", 0x000000, _128);
			/// --- SYSTEM FENSTER ZEICHNEN (ID 3) ---
            _15(win->id EQ 3) {
                _43 mid = wx + ww/2;
                _43 btn_y = wy + 45;
                
                /// FIX: Nur Klicks zulassen, wenn das Fenster GANZ OBEN liegt!
                _44 is_active = (win_z[12] EQ win->id);
                
                /// 1. THEME & LANG TOGGLES
                _30 lang_lbl[20], theme_lbl[30];
                _15(sys_lang EQ 0) str_cpy(lang_lbl, "[ LANG: EN ]"); _41 str_cpy(lang_lbl, "[ SPR: DE ]");
                _15(sys_lang EQ 0) { _15(sys_theme EQ 0) str_cpy(theme_lbl, "[ THEME: COMPUTER ]"); _41 str_cpy(theme_lbl, "[ THEME: GENESIS ]"); } 
                _41 { _15(sys_theme EQ 0) str_cpy(theme_lbl, "[ THEMA: COMPUTER ]"); _41 str_cpy(theme_lbl, "[ THEMA: GENESIS ]"); }
                
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+5, btn_y, 140, 20)) { sys_lang = !sys_lang; input_cooldown = 15; }
                Text(wx+10, btn_y+4, lang_lbl, 0x000000, _128);
                
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+5, btn_y+30, 200, 20)) { sys_theme = !sys_theme; input_cooldown = 15; }
                Text(wx+10, btn_y+34, theme_lbl, 0x000000, _128);
                
                /// Leerer Bereich
                DrawRoundedRect(wx+20, wy+120, ww-40, 2, 0, 0xAAAAAA);
                
                /// 2. CPU & USER INFO
                TextC(mid, wy+140, user_name, 0x222222, _128); 
                TextC(mid, wy+160, cpu_brand, 0x0000FF, _128);
                
                /// 3. HARDWARE STATUS
                TextC(mid, wy+200, "HARDWARE STATUS", 0x000000, _128);
                Text(wx+30, wy+230, "CORE:", 0x555555, _128); Text(wx+130, wy+230, "32-BIT PROTECTED MODE", 0x00AA00, _128);
                Text(wx+30, wy+250, "MEM:", 0x555555, _128); Text(wx+130, wy+250, "4 GB ADDRESS SPACE", 0x00AA00, _128);
                
                _30 l_disk[20] = "STORAGE:"; _30 l_net[20] = "NETWORK:"; _30 l_gpu[20] = "GRAPHIC:"; _30 l_usb[20] = "USB HOST:";
                _15(sys_lang NEQ 0) { str_cpy(l_disk, "FESTPLATTE:"); str_cpy(l_net, "NETZWERK:"); str_cpy(l_gpu, "GRAFIK:"); }
                
                /// --- DYNAMISCHE FARBEN FÜR TASTATUR-FOKUS ---
                uint32_t c_st_lbl = 0x555555, c_st_val = 0x0044CC;
                uint32_t c_nt_lbl = 0x555555, c_nt_val = 0x0044CC;
                uint32_t c_gp_lbl = 0x555555, c_gp_val = 0x0044CC;
                uint32_t c_us_lbl = 0x555555, c_us_val = 0x0044CC;
                uint32_t btn_color = 0x444444;

                /// BARE METAL FIX: Index verschoben, Button ist ganz oben!
                if (sys_selected_item == 0) { btn_color = 0xAA0000; }
                if (sys_selected_item == 1) { c_st_lbl = 0xFF0000; c_st_val = 0xFF0000; }
                if (sys_selected_item == 2) { c_nt_lbl = 0xFF0000; c_nt_val = 0xFF0000; }
                if (sys_selected_item == 3) { c_gp_lbl = 0xFF0000; c_gp_val = 0xFF0000; }
                if (sys_selected_item == 4) { c_us_lbl = 0xFF0000; c_us_val = 0xFF0000; }
				/// ==========================================
				/// DER GROSSE ORAKEL-BUTTON (GANZ OBEN)
				/// ==========================================
				_43 btn_scan_x = wx + 20;
				_43 btn_scan_y = wy + 150; /// Wieder nach oben geschoben!
				
				uint32_t btn_oracle_color = 0x444444; 
				/// Wenn er mit den Pfeiltasten angewählt ist (Index 0), leuchtet er Rot!
				if (sys_selected_item == 0) { btn_oracle_color = 0xAA0000; }
				
				DrawRoundedRect(btn_scan_x, btn_scan_y, 250, 30, 4, btn_oracle_color);
				Text(btn_scan_x + 10, btn_scan_y + 8, "OPEN 64-BIT ORACLE", 0xFFFFFF, _128);
				
				/// Klick-Abfrage für die MAUS
				_44 mouse_klick_oracle = (mouse_just_pressed AND is_over_rect(mouse_x, mouse_y, btn_scan_x, btn_scan_y, 250, 30));
				
				_15(input_cooldown EQ 0 AND is_active AND mouse_klick_oracle) {
					/// Wenn die Maus klickt, simuliere einfach einen ENTER-Tastendruck auf Index 0
					sys_selected_item = 0; 
					key_scancode = 0x1C; 
					input_cooldown = 30;
				}

                /// KLICKBAR: STORAGE 
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+30, wy+270, 300, 20)) {
                    scan_pci_class(0x01, hw_storage, "CTRL");
                    input_cooldown = 15;
                }
                Text(wx+30, wy+275, l_disk, c_st_lbl, _128); Text(wx+130, wy+275, hw_storage, c_st_val, _128);
                
                /// KLICKBAR: NETWORK 
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+30, wy+295, 300, 20)) {
                    scan_pci_class(0x02, hw_net, "NIC");
                    input_cooldown = 15;
                }
                Text(wx+30, wy+300, l_net, c_nt_lbl, _128); Text(wx+130, wy+300, hw_net, c_nt_val, _128);
                
                /// KLICKBAR: GRAPHIC 
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+30, wy+320, 300, 20)) {
                    scan_pci_class(0x03, hw_gpu, "GPU");
                    input_cooldown = 15;
                }
                Text(wx+30, wy+325, l_gpu, c_gp_lbl, _128); Text(wx+130, wy+325, hw_gpu, c_gp_val, _128);
                
                /// KLICKBAR: USB (DAS IST JETZT DIE ZÜNDUNG!)
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+30, wy+345, 300, 20)) {
                    system_init_usb(); 
                    input_cooldown = 15;
                }
                Text(wx+30, wy+350, l_usb, c_us_lbl, _128); Text(wx+130, wy+350, hw_usb, c_us_val, _128);
                
                /// 4. POWER BUTTONS
                DrawRoundedRect(wx+30, wy+wh-50, 120, 30, 4, 0xAA0000); TextC(wx+90, wy+wh-40, "REBOOT", 0xFFFFFF, _128);
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+30, wy+wh-50, 120, 30)) { system_reboot(); }
                DrawRoundedRect(wx+ww-150, wy+wh-50, 120, 30, 4, 0x000000); TextC(wx+ww-90, wy+wh-40, "SHUT DOWN", 0xFFFFFF, _128);
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+ww-150, wy+wh-50, 120, 30)) { system_shutdown(); }
            }
			/// ==========================================
            /// DISK MANAGER (FENSTER ID 4) - NEUE FASSADE
            /// ==========================================
            _15(win->id EQ 4) {
                _44 is_active = (win_z[12] EQ win->id);
                uint32_t cfs_ram_addr = 0x00900000;
                /// ------------------------------------------
                /// VIEW 2: GEÖFFNETES LAUFWERK (DATEI-EXPLORER)
                /// ------------------------------------------
                _15(dsk_mgr_opened) {
                    DrawRoundedRect(wx+15, wy+45, 180, 55, 4, 0x222222); Text(wx+25, wy+50, "DRIVE CAPACITY:", 0xAAAAAA, _128);
                    /// BARE METAL FIX: Dynamische Einheit (MB oder GB)
                    char s_cap[10]; int_to_str(drive_total_gb, s_cap); char s_kb[10]; int_to_str(drive_used_kb, s_kb);
                    char* cap_lbl = (char*)((selected_drive_idx == 99) ? "MB TOTAL" : "GB TOTAL");
                    Text(wx+25, wy+65, s_cap, 0x00FF00, _128); Text(wx+55, wy+65, cap_lbl, 0x00FF00, _128);
                    Text(wx+25, wy+80, s_kb, 0xFF8800, _128); Text(wx+55, wy+80, "KB USED", 0xFF8800, _128);
                    DrawRoundedRect(wx+210, wy+45, 60, 25, 4, 0x444444); TextC(wx+240, wy+53, "BACK", 0xFFFFFF, _128);
                    _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+210, wy+45, 60, 25)) {
                        dsk_mgr_opened = _86; input_cooldown = 15; 
                    }
                    DrawRoundedRect(wx+280, wy+45, 60, 25, 4, 0x444444); TextC(wx+310, wy+53, "HOME", 0xFFFFFF, _128);
                    DrawRoundedRect(wx+210, wy+75, 80, 25, 4, 0xAA5500); TextC(wx+250, wy+83, "+ FILE", 0xFFFFFF, _128);
                    _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+210, wy+75, 80, 25)) {
                        /// BARE METAL FIX: Wipe directory buffer before reading
                        for(int i=0; i<512; i++) ((char*)cfs_ram_addr)[i] = 0;
                        
                        if(selected_drive_idx != 99 && disk_read_auto(1, cfs_ram_addr)) {
                            CFS_DIR_ENTRY* dir = (CFS_DIR_ENTRY*)(cfs_ram_addr + 16);
                            for(int i=0; i<8; i++) {
                                if(dir[i].type == 0) {
                                    dir[i].type = 1; str_cpy(dir[i].filename, "NEU.TXT"); dir[i].file_size = 0; dir[i].start_lba = 4000 + i;
                                    ahci_write_sectors(1, cfs_ram_addr);
                                    cfs_files[i].exists = 1; str_cpy(cfs_files[i].name, "NEU.TXT"); cfs_files[i].size = 0; cfs_files[i].start_lba = 4000 + i;
                                    break;
                                }
                            }
                        }
                        input_cooldown = 15;
                    }
                    DrawRoundedRect(wx+300, wy+75, 80, 25, 4, 0x5555AA); TextC(wx+340, wy+83, "+ FOLDER", 0xFFFFFF, _128);
                    /// 4. Dateiliste (Unten)
                    _43 y_off = wy + 120;
                    Text(wx+15, y_off - 15, "--- CFS FILE SYSTEM ---", 0xFFFFFF, _128);
                    
                    _39(_43 i=0; i<8; i++) {
                        _15(cfs_files[i].exists) {
                            _44 is_hov = is_over_rect(mouse_x, mouse_y, wx+15, y_off, 200, 20);
                            DrawRoundedRect(wx+15, y_off, 16, 16, 2, is_hov ? 0x00AAFF : 0x0088FF);
                            Text(wx+40, y_off+4, cfs_files[i].name, is_hov ? 0x00FF00 : 0xFFFFFF, _86);
                            
                            char s_buf[10]; int_to_str(cfs_files[i].size, s_buf); 
                            Text(wx+140, y_off+4, s_buf, 0xAAAAAA, _86); Text(wx+160, y_off+4, "B", 0xAAAAAA, _86);
                            
                            /// ==========================================
                            /// BARE METAL FIX: DER OPEN BUTTON FÜR DATEIEN
                            /// ==========================================
                            DrawRoundedRect(wx+190, y_off, 40, 16, 2, 0x0055AA);
                            Text(wx+198, y_off+4, "OPEN", 0xFFFFFF, _86);
                            
                           /// KLICK AUF OPEN!
                            _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+190, y_off, 40, 16)) {
                                uint32_t file_ram_addr = 0x00A00000;
                                
                                /// BARE METAL FIX: Wipe the raw disk buffer
                                for(int i=0; i<4096; i++) ((char*)file_ram_addr)[i] = 0;
                                
                                if(disk_read_auto(cfs_files[i].start_lba, file_ram_addr)) {
                                    Window* notepad = &windows[0]; 
                                    char* file_data = (char*)file_ram_addr; 
                                    notepad->cursor_pos = 0;
                                    
                                    /// BARE METAL FIX: Wipe the ENTIRE Notepad buffer before copying!
                                    for(int c=0; c < 2000; c++) notepad->content[c] = 0;
                                    
                                    /// BARE METAL FIX: Safely copy ONLY the actual file size
                                    int f_size = cfs_files[i].size;
                                    for(int c=0; c < f_size && c < 2000; c++) {
                                        notepad->content[notepad->cursor_pos++] = file_data[c];
                                    }
                                    
                                    /// Guarantee null termination at the exact end of the file
                                    notepad->content[notepad->cursor_pos] = 0;
                                    
                                    active_file_lba = cfs_files[i].start_lba; 
                                    active_file_idx = i; 
                                    str_cpy(notepad->title, cfs_files[i].name);
                                    notepad->open = _128; notepad->minimized = _86; 
                                    focus_window(0);
                                }
                                input_cooldown = 15;
                            }
                            y_off += 25;
                        }
                    }
				}
                /// ------------------------------------------
                /// VIEW 1: HAUPTMENÜ (LAUFWERKS-ÜBERSICHT)
                /// ------------------------------------------
                _41 {
                    /// 1. LAUFWERKS-AUSWAHL (LINKS)
                    _43 list_y = wy + 60;
                    Text(wx+15, list_y - 15, "AVAILABLE DRIVES:", 0xAAAAAA, _128);
                    _39(_43 i=0; i < detected_port_count; i++) {
                        _43 port_num = detected_ports[i];
                        _44 is_sel = (selected_drive_idx == i);
                        DrawRoundedRect(wx+15, list_y, 120, 25, 4, is_sel ? 0x0088FF : 0x333333);
                        _30 d_name[] = "PORT 0"; d_name[5] = '0' + port_num; Text(wx+25, list_y+5, d_name, 0xFFFFFF, _128);
                        HBA_MEM* hba = (HBA_MEM*)active_ahci_bar5;
                        if(hba->ports[port_num].sig == 0x00000101) Text(wx+80, list_y+5, "HDD", 0x00FF00, _128);
                        else if(hba->ports[port_num].sig == 0xEB140101) Text(wx+80, list_y+5, "CD", 0xAAAAAA, _128);
                        _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+15, list_y, 120, 25)) {
                            selected_drive_idx = i; active_sata_port = port_num; is_mounted = false; 
                            HBA_MEM* hba_ptr = (HBA_MEM*)active_ahci_bar5; ahci_init_port(&hba_ptr->ports[port_num], port_num);
                            input_cooldown = 15;
                        }
                        list_y += 30;
                    }
                    _15(hw_usb[0] EQ 'E' AND hw_usb[1] EQ 'N' AND hw_usb[2] EQ 'D') { 
                        _44 is_usb_sel = (selected_drive_idx == 99);
                        DrawRoundedRect(wx+15, list_y, 120, 25, 4, is_usb_sel ? 0x0088FF : 0x333333);
                        Text(wx+25, list_y+5, "USB", 0xFFFFFF, _128); Text(wx+80, list_y+5, "BOT", 0x00FFFF, _128); 
                        _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+15, list_y, 120, 25)) {
                            selected_drive_idx = 99; is_mounted = false; input_cooldown = 15;
                        }
                        list_y += 30;
                    }
                    /// 2. BUTTONS: MOUNT & FORMAT
                    _89 btn_col = (selected_drive_idx == -1) ? 0x444444 : 0x00AA00;
                    DrawRoundedRect(wx+150, wy+60, 80, 25, 4, btn_col); TextC(wx+190, wy+68, "MOUNT", 0xFFFFFF, _128);
                    DrawRoundedRect(wx+240, wy+60, 80, 25, 4, (selected_drive_idx == -1) ? 0x444444 : 0xAA0000); TextC(wx+280, wy+68, "FORMAT", 0xFFFFFF, _128);
                    /// KLICK: FORMAT
                    _15(selected_drive_idx != -1 AND input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+240, wy+60, 80, 25)) {
                        win->cursor_pos = 0; /// UI fix
                        /// HIER WAR DIE BLOCKADE! Raus damit!
                        char* buf = (char*)cfs_ram_addr; for(int i=0; i<512; i++) buf[i] = 0;
                        buf[0] = 'C'; buf[1] = 'F'; buf[2] = 'S';
                        /// BARE METAL FIX: Wir nutzen den neuen Adapter!
                        if (disk_write_auto(1, cfs_ram_addr)) {
                            is_mounted = false; print_win(win, "\n[OK] CFS FORMATTED ON SECTOR 1.\n");
                        } else { 
                            print_win(win, "\n[ERR] FORMAT WRITE FAILED.\n"); 
                        }
                        input_cooldown = 15;
                    }
                    /// KLICK: MOUNT
                    _15(selected_drive_idx != -1 AND input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+150, wy+60, 80, 25)) {
                        /// BARE METAL FIX: Textfeld vor dem Mounten leeren!
                        win->cursor_pos = 0;
                        _15(selected_drive_idx == 99) {
                            print_win(win, "\n[USB] SENDE SCSI COMMAND...\n");
                            drive_total_gb = xhci_bot_get_capacity(1); 
                            _15(drive_total_gb > 0) {
                                is_mounted = _128; drive_used_kb = 0; 
                                print_win(win, "[OK] USB CAPACITY READ.\n");
                            } _41 { 
                                print_win(win, "[ERR] USB CAPACITY TIMEOUT.\n"); 
                            }
                        } 
                        _41 {
                            /// ... (Dein restlicher SATA Mount Code) ...
                            if(ahci_identify(cfs_ram_addr)) {
                                uint32_t lba_low = *(uint32_t*)(cfs_ram_addr + 200);
                                drive_total_gb = lba_low / 2097152; if(drive_total_gb == 0) drive_total_gb = 1; 
                            } else { drive_total_gb = 0; }
                            
                            /// BARE METAL FIX: Wipe before reading directory
                            for(int i=0; i<512; i++) ((char*)cfs_ram_addr)[i] = 0;
                            
                            if(disk_read_auto(1, cfs_ram_addr)) {
                                char* buf = (char*)cfs_ram_addr;
                                if(buf[0] == 'C' && buf[1] == 'F' && buf[2] == 'S') {
                                    is_mounted = true; drive_used_kb = 0;
                                    CFS_DIR_ENTRY* dir = (CFS_DIR_ENTRY*)(cfs_ram_addr + 16);
                                    for(int i=0; i<8; i++) {
                                        if(dir[i].type == 1) {
                                            cfs_files[i].exists = 1; 
                                            
                                            /// BARE METAL FIX: Force null termination on filename
                                            str_cpy(cfs_files[i].name, dir[i].filename);
                                            cfs_files[i].name[15] = 0; /// Assuming max length 15
                                            
                                            cfs_files[i].size = dir[i].file_size; cfs_files[i].start_lba = dir[i].start_lba;
                                            drive_used_kb += dir[i].file_size; 
                                        } else { cfs_files[i].exists = 0; }
                                    }
                                    print_win(win, "\n[OK] CFS MOUNTED.\n");
                                } else { print_win(win, "\n[ERR] NO CFS SIGNATURE FOUND.\n"); }
                            } else { print_win(win, "\n[ERR] READ FAILED.\n"); }
                        }
                        input_cooldown = 15;
                    }
                    /// 3. KAPAZITÄT, OPEN DRIVE & DEBUG TEXT
                    _15(is_mounted) {
                        _43 box_y = wy + 100;
                        DrawRoundedRect(wx+150, box_y, 170, 55, 4, 0x222222); Text(wx+160, box_y+5, "DRIVE CAPACITY:", 0xAAAAAA, _128);
                        /// BARE METAL FIX: Dynamische Einheit!
                        char s_cap[10]; int_to_str(drive_total_gb, s_cap); char s_kb[10]; int_to_str(drive_used_kb, s_kb);
                        char* cap_lbl = (char*)((selected_drive_idx == 99) ? "MB TOTAL" : "GB TOTAL");
                        Text(wx+160, box_y+20, s_cap, 0x00FF00, _128); Text(wx+190, box_y+20, cap_lbl, 0x00FF00, _128);
                        Text(wx+160, box_y+35, s_kb, 0xFF8800, _128); Text(wx+190, box_y+35, "KB USED", 0xFF8800, _128);
                        DrawRoundedRect(wx+150, box_y+65, 170, 30, 4, 0x0055AA); TextC(wx+235, box_y+73, "OPEN DRIVE", 0xFFFFFF, _128);
                        _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_active AND is_over_rect(mouse_x, mouse_y, wx+150, box_y+65, 170, 30)) {
                            dsk_mgr_opened = _128;
                            input_cooldown = 15;
                        }
                        /// DEBUG TEXT UNTER DEM BUTTON
                        Text(wx+150, box_y+110, win->content, txt_color, _86);
                    }
                    _41 {
                        Text(wx+150, wy+160, win->content, txt_color, _86);
                    }
                }
			}
			/// ==========================================
            /// NOTEPAD (ID 0) - BARE METAL FIX (SICHTBAR!)
            /// ==========================================
            _15(win->id EQ 0) {
                /// BARE METAL FIX: Harte Farbe (Weiß) für Text!
                _89 safe_txt_color = 0xFFFFFF; 
                Text(wx+15, wy+45, win->content, safe_txt_color, _86);
                
                /// BARE METAL FIX: Sichtbarer Block-Cursor!
                _15(win_z[12] EQ win->id AND (frame / 20) % 2 EQ 0) {
                    _43 cursor_off_x = 0; _43 cursor_off_y = 0;
                    _39(_43 c_idx = 0; c_idx < win->cursor_pos; c_idx++) { 
                        _15(win->content[c_idx] EQ '\n') { cursor_off_y += 15; cursor_off_x = 0; } 
                        _41 cursor_off_x += 6; 
                    }
                    DrawRoundedRect(wx + 15 + cursor_off_x, wy + 45 + cursor_off_y, 6, 10, 0, safe_txt_color);
                }
                
                /// SAVE BUTTON (Idiotensicher)
                DrawRoundedRect(wx+ww-80, wy+15, 60, 20, 3, 0x005500); TextC(wx+ww-50, wy+21, "SAVE", 0xFFFFFF, _128);
                _15(input_cooldown EQ 0 AND mouse_just_pressed AND is_over_rect(mouse_x, mouse_y, wx+ww-80, wy+15, 60, 20)) {
                    str_cpy(win->title, "SAVING..."); 
                    
                    uint32_t file_ram_addr = 0x09000000; char* file_data = (char*)file_ram_addr;
                    for(int i=0; i<512; i++) file_data[i] = 0; 
                    for(int i=0; i < win->cursor_pos; i++) file_data[i] = win->content[i];
                    
                    /// BARE METAL FIX: Wir erzwingen das Speichern auf Sektor 500!
                    /// Egal ob eine Datei geöffnet war oder nicht!
                    _43 test_lba = 500; 
                    if(active_file_lba > 0) test_lba = active_file_lba;
                    
                    if(disk_write_auto(test_lba, file_ram_addr)) {
                        str_cpy(win->title, "NOTEPAD - SAVED!");
                    } else {
                        str_cpy(win->title, "NOTEPAD - WRITE ERROR!");
                    }
                    input_cooldown = 15;
                }
			}
            
            /// ==========================================
            /// CMD (ID 5) - BARE METAL FIX (SICHTBAR!)
            /// ==========================================
            _15(win->id EQ 5) {
                _89 cmd_color = 0x00FF00; /// Hacker-Grün
                Text(wx+15, wy+45, win->content, cmd_color, _86);
                
                _43 lines = 0; _39(_43 i=0; i<win->cursor_pos; i++) { if(win->content[i] == '\n') lines++; }
                _43 prompt_y = wy + 45 + (lines * 15);
                
                Text(wx+15, prompt_y, "C:\\> ", cmd_color, _128);
                Text(wx+55, prompt_y, cmd_input_buf, cmd_color, _128);
                
                /// Cursor blinken lassen
                _15(win_z[12] EQ win->id AND (frame / 20) % 2 EQ 0) {
                    DrawRoundedRect(wx + 55 + (cmd_input_idx * 7), prompt_y, 6, 10, 0, cmd_color);
                }
            }
        }
		/// ==========================================
        /// BARE METAL FIX: ORACLE RENDERER (64-BIT HUD)
        /// ==========================================
        if (show_oracle) {
            DrawRoundedRect(30, 30, 740, 540, 5, 0x111111);
            DrawRoundedRect(30, 30, 740, 30, 5, 0x333333);
            Text(200, 38, "64-BIT HARDWARE ORACLE (SAFE READ ONLY)", 0x00FF00, _128);
            
            Text(50, 70, "B:D:F", 0xAAAAAA, _128);
            Text(130, 70, "VENDOR DEVICE", 0xAAAAAA, _128);
            Text(280, 70, "64-BIT BASE ADDR (BAR0)", 0xAAAAAA, _128);
            Text(550, 70, "DEVICE CLASS", 0xAAAAAA, _128);
            
            int ry = 95;
            for(int i=0; i<mirror_count; i++) {
                char sb[10], sv[10], sd[10];
                int_to_str(mirror_list[i].bus, sb);
                hex_to_str(mirror_list[i].vendor, sv);
                hex_to_str(mirror_list[i].device, sd);
                
                Text(50, ry, sb, 0x888888, _86);
                Text(130, ry, sv, 0x00FF00, _86);
                Text(190, ry, sd, 0x00FF00, _86);
                
                /// Die 64-Bit Adresse sicher in zwei Strings teilen und zusammenbauen
                char h1[15], h2[15];
                hex_to_str((uint32_t)(mirror_list[i].bar0 >> 32), h1);
                hex_to_str((uint32_t)(mirror_list[i].bar0 & 0xFFFFFFFF), h2);
                char s_bar[35];
                str_cpy(s_bar, h1);
                str_cat(s_bar, " ");
                str_cat(s_bar, h2);
                
                uint32_t col = (mirror_list[i].bar0 == 0) ? 0xFF0000 : 0xFFFFFF;
                Text(280, ry, s_bar, col, _86);
                
                Text(550, ry, mirror_list[i].name, 0xCCCCCC, _86);
                ry += 14;
            }
            
            /// Mit ESC schließen (Scancode 1)
            if (key_scancode == 0x01) {
                show_oracle = false;
                key_scancode = 0; /// Taste konsumieren
            }
        }
        DrawAeroCursor(mouse_x, mouse_y);
        Swap(); 
        frame++;
    }
}