/* ======================================================================== */
/* COSMOS OS v38.0 - SCHNEIDER SECURITY EDITION (PLATINUM RELEASE)          */
/* PLATFORM: x86_64 (LIMINE) | STATUS: SORTED & FIXED                       */
/* ======================================================================== */

#include <stdint.h>
#include <stddef.h> 
#include "limine.h"

// Forward Declarations
// Zusätzliche Forward Declarations
int Cos(int a);
int Sin(int a);
void DrawRoundedRect(int x, int y, int rw, int rh, int r, uint32_t c);
uint32_t random();
void str_cpy(char* d, const char* s);
void DrawRoundWindow(int x, int y, int r, uint32_t c);
void DrawCentrifugalVortex(int cx, int cy, int exp);
bool is_over(int mx, int my, int px, int py, int r);
bool is_over_rect(int mx, int my, int x, int y, int w, int h);
void DrawIcon(int x, int y, uint32_t col);
char get_ascii(uint8_t scancode);
void send_udp(uint32_t dip, uint16_t sp, uint16_t dp, const char* msg);
void send_tcp_syn(uint32_t dip, uint16_t dp); 

// ==========================================
// 1. DATA STRUCTURES (Muss ganz oben stehen)
// ==========================================
struct PhysicalDrive { bool present; char model[41]; uint32_t size_mb; uint16_t base_port; bool is_slave; int type; };
struct Partition { uint8_t status; uint8_t type; uint32_t start_lba; uint32_t size; };
struct FileEntry { char name[32]; uint32_t sector_offset; bool is_folder; uint32_t size; char date[16]; bool exists; uint8_t parent_idx; uint8_t padding[5]; };
struct NICInfo { char name[32]; uint32_t address; int type; };
struct Window { int id; char title[16]; int x, y, w, h; bool open, minimized, fullscreen; uint32_t color; };
struct Planet { int ang; int dist; char name[8]; int cur_x, cur_y; }; 
struct Star { int x, y, z; }; 

struct EthernetFrame { uint8_t dest_mac[6]; uint8_t src_mac[6]; uint16_t type; } __attribute__((packed)); 
struct IPHeader { uint8_t version_ihl; uint8_t dscp_ecn; uint16_t total_length; uint16_t id; uint16_t flags_fragment; uint8_t ttl; uint8_t protocol; uint16_t checksum; uint32_t src_ip; uint32_t dest_ip; } __attribute__((packed));  
struct UDPHeader { uint16_t src_port; uint16_t dest_port; uint16_t length; uint16_t checksum; } __attribute__((packed));
struct TCPHeader { uint16_t src, dest; uint32_t seq, ack; uint8_t off_res, flags; uint16_t win, chk, urg; } __attribute__((packed));

// ==========================================
// 2. GLOBAL VARIABLES
// ==========================================
static int frame = 0;
int screen_w = 800; int screen_h = 600; 
uint64_t pitch = 0; 
uint32_t* fb = 0; 
uint32_t* bb = 0; 

uint64_t rtl_io_base = 0;
uint64_t intel_mem_base = 0;
uint64_t usb_io_base = 0;

int active_mode = 0, boot_frame = 0;
int galaxy_expansion = 0; bool galaxy_open = false;
int rtc_h, rtc_m, rtc_day, rtc_mon, rtc_year;
char user_name[] = "SCHNEIDER"; char ip_address[] = "SEARCHING..."; char cpu_brand[49];
int mouse_x=400, mouse_y=300; uint8_t mouse_cycle=0; int8_t mouse_byte[3]; 
bool mouse_left=false, mouse_just_pressed=false; uint8_t last_key = 0; bool key_new = false; 
bool key_shift = false;

int win_z[10] = {0, 1, 2, 4, 3, 6, 5, 7, 8, 9}; 
int drag_win = -1; int drag_off_x = 0; int drag_off_y = 0; int resize_win = -1; 
bool z_blocked = false;

char note_buf[10][41]; int note_r=0, note_c=0; char save_timestamp[32] = ""; 
char cmd_input[41] = {0}; int cmd_idx = 0; char cmd_last_out[128] = "READY."; char cmd_lines[5][41]; char cmd_status[32] = "OFFLINE"; 

bool touch_mode = false; 
bool webcam_active = false; bool logitech_found = false; char webcam_model[40] = "NO WEBCAM";
int wifi_state = 0; int wifi_selected = -1; char wifi_pass[32] = {0}; int wifi_pass_idx = 0;
char wifi_ssids[4][20] = {"NO SIGNAL", "NO SIGNAL", "NO SIGNAL", "NO SIGNAL"}; 
char hw_disk[48] = "SCANNING..."; char hw_net[48] = "SCANNING..."; char hw_gpu[48] = "VGA/VESA"; char hw_usb[48] = "NO USB"; 
bool usb_detected = false; 

int save_step = 0; int save_part_sel = 0; char save_filename[32] = "blank.txt"; int save_name_idx = 9;
char new_folder_name[32] = "new_dir"; int folder_name_idx = 7;
PhysicalDrive drives[8]; int drive_count = 0; Partition partitions[4]; uint8_t sector0[512]; 
FileEntry file_table[8]; 
int current_open_file = -1; int drive_status = 0; 
uint8_t hdd_buf[512]; 

uint8_t current_path_id = 255; 
int current_folder_view_idx = -1; 
int dsk_view = 0; int dsk_selection = 0; int file_selection = -1; int active_drive_idx = -1; int input_cooldown = 0;

uint8_t mac_addr[6] = {0,0,0,0,0,0}; char mac_str[24] = "00:00:00:00:00:00";
NICInfo found_nics[5]; int nic_count = 0; int active_nic_idx = -1; uint16_t ata_base = 0x1F0; 
uint8_t* tx_buffer = (uint8_t*)0x500000; int tx_cur = 0; 

int sleep_mode_idx = 0; int sys_menu_selection = 0; char sleep_labels[6][16];
int shake_timer = 0, sx_off = 0, sy_off = 0; 
int sys_lang = 0; int sys_theme = 0; int genesis_tab = 0; 
Window windows[10]; Planet planets[6]; Star stars[200]; int senses_tab = 0; 

// ==========================================
// 3. LOW LEVEL I/O & SYSTEM HELPERS (VOR ALLEM ANDEREN!)
// ==========================================
inline uint8_t inb(uint16_t port) { uint8_t ret; asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
inline void outb(uint16_t port, uint8_t val) { asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
inline uint32_t inl(uint16_t port) { uint32_t ret; asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
inline void outl(uint16_t port, uint32_t val) { asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port)); }
inline void outw(uint16_t p, uint16_t v) { asm volatile("outw %0, %1"::"a"(v),"Nd"(p)); }
inline void insw(uint16_t p, void* a, uint32_t c) { asm volatile("rep; insw" : "+D"(a), "+c"(c) : "d"(p) : "memory"); }
inline void outsw(uint16_t p, const void* a, uint32_t c) { asm volatile("rep; outsw" : "+S"(a), "+c"(c) : "d"(p) : "memory"); }
uint32_t mmio_read32(uint64_t addr) { return *(volatile uint32_t*)(addr); }

// CPUID Helper
inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(leaf));
}

// BCD Conversion
inline uint8_t bcd2bin(uint8_t b) { return ((b >> 4) * 10) + (b & 0xF); }

// ==========================================
// 4. CORE DRIVERS (RTC, SOUND, ETC.) - HIERHIN VERSCHOBEN!
// ==========================================
void read_rtc() { 
    outb(0x70, 4); rtc_h = bcd2bin(inb(0x71)); 
    outb(0x70, 2); rtc_m = bcd2bin(inb(0x71)); 
    outb(0x70, 7); rtc_day = bcd2bin(inb(0x71)); 
    outb(0x70, 8); rtc_mon = bcd2bin(inb(0x71)); 
    outb(0x70, 9); rtc_year = bcd2bin(inb(0x71)); 
    rtc_h = (rtc_h + 1) % 24; 
}

void play_freq(uint32_t f) { if(f==0) { outb(0x61, inb(0x61) & 0xFC); return; } uint32_t d = 1193180 / f; outb(0x43, 0xB6); outb(0x42, (uint8_t)d); outb(0x42, (uint8_t)(d >> 8)); uint8_t t = inb(0x61); if (t != (t | 3)) outb(0x61, t | 3); }
void play_sound(uint32_t n_freq, int duration) { if(n_freq == 0) return; uint32_t div = 1193180 / n_freq; outb(0x43, 0xB6); outb(0x42, (uint8_t)(div)); outb(0x42, (uint8_t)(div >> 8)); uint8_t tmp = inb(0x61); if(tmp != (tmp | 3)) outb(0x61, tmp | 3); for(int i=0; i<duration*10000; i++) asm volatile("nop"); outb(0x61, tmp & 0xFC); }
void startup_melody() { play_sound(523, 100); play_sound(659, 100); play_sound(784, 200); }

void Swap(); // Forward Declaration
void play_earthquake(int duration) { shake_timer = duration; while(shake_timer > 0) { play_freq(40 + (random() % 60)); Swap(); for(int d=0; d<40000; d++) { __asm__ volatile("nop"); } } play_freq(0); 
}void system_reboot() { play_earthquake(30); while(inb(0x64)&2); outb(0x64,0xFE); }
void system_shutdown() { play_earthquake(30); outw(0x604, 0x2000); outw(0xB004, 0x2000); outw(0x4004, 0x3400); while(1) asm volatile("cli; hlt"); }

void get_cpu_brand() {
    uint32_t eax, ebx, ecx, edx;
    uint32_t brand[12]; 
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004) { str_cpy(cpu_brand, "GENERIC x64 CPU"); return; }
    cpuid(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
    cpuid(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
    cpuid(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);
    char* s = (char*)brand; char* d = cpu_brand;
    for (int i = 0; i < 48; i++) { if (i == 0 && s[i] == ' ') continue; *d++ = s[i]; }
    *d = 0;
}

// ==========================================
// 5. GRAPHICS ENGINE
// ==========================================
void Put(int x, int y, uint32_t c) { if(x<0 || x>=800 || y<0 || y>=600) return; bb[y*800+x]=c; }
void PutAlpha(int x, int y, uint32_t c) { if(x<0 || x>=800 || y<0 || y>=600) return; uint32_t bg = bb[y*800+x]; uint32_t s1 = ((c & 0xFEFEFE) >> 1) + ((bg & 0xFEFEFE) >> 1); bb[y*800+x] = ((s1 & 0xFEFEFE) >> 1) + ((bg & 0xFEFEFE) >> 1); }
void Clear() { for(int i=0; i<800*600; i++) bb[i] = 0; }

// Fixed Swap with Pitch
void Swap() {
    if(shake_timer > 0) { sx_off = (random()%10)-5; sy_off = (random()%10)-5; shake_timer--; } else { sx_off=0; sy_off=0; }
    for(int y=0; y < screen_h; y++) {
        int ty = y + sy_off;
        if(ty < 0 || ty >= screen_h) continue;
        uint32_t* dst = (uint32_t*)((uint8_t*)fb + (ty * pitch));
        uint32_t* src = bb + (y * 800);
        for(int x=0; x < screen_w; x++) {
            int tx = x + sx_off;
            if(tx >= 0 && tx < screen_w) dst[tx] = src[x];
        }
    }
}
// Hilfsfunktionen für Kollision / Maus
bool is_over(int mx, int my, int px, int py, int r) {
    int dx = mx - px; int dy = my - py;
    return (dx*dx + dy*dy) < (r*r);
}

bool is_over_rect(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

// Zeichnet ein rundes Fenster / Planeten
void DrawRoundWindow(int x, int y, int r, uint32_t c) {
    for(int iy = -r; iy < r; iy++) {
        for(int ix = -r; ix < r; ix++) {
            if(ix*ix + iy*iy < r*r) Put(x + ix, y + iy, c);
        }
    }
}

// Zeichnet den Galaxie-Effekt im Hintergrund
void DrawCentrifugalVortex(int cx, int cy, int exp) {
    for(int i=0; i < exp; i+=2) {
        int r = i;
        int a = (frame + i) % 256;
        Put(cx + (Cos(a)*r)/84, cy + (Sin(a)*r*3/4)/84, 0x333333);
    }
}

// Kleines Icon für den Disk-Manager
void DrawIcon(int x, int y, uint32_t col) {
    DrawRoundedRect(x, y, 12, 10, 2, col);
    DrawRoundedRect(x+2, y-2, 8, 2, 0, col);
}

// Math helpers
const int sin_lut[256] = { 1, 2, 4, 7, 9, 12, 14, 17, 19, 21, 24, 26, 28, 30, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 56, 58, 60, 61, 63, 64, 66, 67, 68, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 79, 80, 81, 81, 82, 82, 83, 83, 83, 84, 84, 84, 84, 84, 84, 84, 83, 83, 83, 82, 82, 81, 81, 80, 79, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 68, 67, 66, 64, 63, 61, 60, 58, 56, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 30, 28, 26, 24, 21, 19, 17, 14, 12, 9, 7, 4, 2, 1, -1, -2, -4, -7, -9, -12, -14, -17, -19, -21, -24, -26, -28, -30, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, -55, -56, -58, -60, -61, -63, -64, -66, -67, -68, -70, -71, -72, -73, -74, -75, -76, -77, -78, -79, -79, -80, -81, -81, -82, -82, -83, -83, -83, -84, -84, -84, -84, -84, -84, -84, -83, -83, -83, -82, -82, -81, -81, -80, -79, -79, -78, -77, -76, -75, -74, -73, -72, -71, -70, -68, -67, -66, -64, -63, -61, -60, -58, -56, -55, -53, -51, -49, -47, -45, -43, -41, -39, -37, -35, -33, -30, -28, -26, -24, -21, -19, -17, -14, -12, -9, -7, -4, -2, -1 };
inline int Cos(int a) { return sin_lut[(a + 64) % 256]; }
inline int Sin(int a) { return sin_lut[a % 256]; }
static uint32_t rng_seed = 123456789;
uint32_t random() { rng_seed = (rng_seed * 1103515245 + 12345) & 0x7FFFFFFF; return rng_seed; }
void str_cpy(char* d, const char* s) { while(*s) *d++ = *s++; *d=0; }
int str_len(const char* s) { int l=0; while(*s++)l++; return l; }
bool str_equal(const char* s1, const char* s2) { while(*s1 && (*s1 == *s2)) { s1++; s2++; } return *(unsigned char*)s1 == *(unsigned char*)s2; }
bool str_starts(const char* full, const char* prefix) { while(*prefix) { if(*prefix++ != *full++) return false; } return true; }
void hex_to_str(uint16_t num, char* out) { const char h[] = "0123456789ABCDEF"; out[0] = h[(num>>12)&0xF]; out[1] = h[(num>>8)&0xF]; out[2] = h[(num>>4)&0xF]; out[3] = h[num&0xF]; out[4] = 0; }
void byte_to_hex(uint8_t b, char* out) { const char h[]="0123456789ABCDEF"; out[0]=h[(b>>4)&0xF]; out[1]=h[b&0xF]; out[2]=0; }
void int_to_str(int n, char* s) { if(n==0){s[0]='0';s[1]=0;return;} int i=0; if(n<0){s[i++]='-';n=-n;} int t=n; while(t>0){t/=10;i++;} s[i]=0; while(n>0){s[--i]=(n%10)+'0';n/=10;} }

uint32_t get_id(const char* c) {
    uint32_t s=0; while(*c) { if(*c>='a' && *c<='z') s+=(*c-'a'+1); else if(*c>='A' && *c<='Z') s+=(*c-'A'+1); c++; }
    if(s==57 || s==54 || s==56 || s==63 || s==101) return s*2; 
    if(s==57 && *(c-1)=='o') return 171;
    return s;
}
inline uint16_t hs(uint16_t v) { return (v<<8)|(v>>8); } 
inline uint32_t hl(uint32_t v) { return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v&0xFF0000)>>8)|((v>>24)&0xFF); } 
uint16_t chk(void* d, int l) { uint32_t s=0; uint16_t* p=(uint16_t*)d; while(l>1){s+=*p++;l-=2;} if(l)s+=*(uint8_t*)p; while(s>>16)s=(s&0xFFFF)+(s>>16); return (uint16_t)(~s); }

// Drawing Primitives
void DrawChar(int x, int y, char c, uint32_t col, bool bold) { 
    static const uint8_t font_upper[] = { 0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22, 0x7F,0x41,0x41,0x22,124, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x49,0x49,0x7A, 0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E, 0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F, 0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43 };
    static const uint8_t font_lower[] = { 0x20,0x54,0x54,0x54,0x78, 0x7F,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x20, 0x38,0x44,0x44,0x48,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x08,0x7E,0x09,0x01,0x02, 0x0C,0x52,0x52,0x52,0x3E, 0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x44,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00, 0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x18,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38, 0x7C,0x14,0x14,0x14,0x08, 0x08,0x14,0x14,0x18,0x7C, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20, 0x04,0x3F,0x44,0x40,0x20, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C, 0x44,0x28,0x10,0x28,0x44, 0x0C,0x50,0x50,0x50,0x3C, 0x44,0x64,0x54,0x4C,0x44 };
    static const uint8_t font_num[] = { 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31, 0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03, 0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E };
    const uint8_t* ptr = 0;
    if(c >= 'A' && c <= 'Z') ptr = &font_upper[(c-'A')*5]; else if(c >= 'a' && c <= 'z') ptr = &font_lower[(c-'a')*5]; else if(c >= '0' && c <= '9') ptr = &font_num[(c-'0')*5];
    else if(c == ':') { static uint8_t s[]={0,0x36,0x36,0,0}; ptr=s; } else if(c == '.') { static uint8_t s[]={0,0x60,0x60,0,0}; ptr=s; } else if(c == '-') { static uint8_t s[]={0x08,0x08,0x08,0x08,0x08}; ptr=s; } else if(c == '!') { static uint8_t s[]={0,0,0x7D,0,0}; ptr=s; } else if(c == '_') { static uint8_t s[]={0x40,0x40,0x40,0x40,0x40}; ptr=s; } else if(c == '(') { static uint8_t s[]={0x1C,0x22,0x41,0,0}; ptr=s; } else if(c == ')') { static uint8_t s[]={0,0,0x41,0x22,0x1C}; ptr=s; } else if(c == '/') { static uint8_t s[]={0x20,0x10,0x08,0x04,0x02}; ptr=s; } else if(c == '=') { static uint8_t s[]={0x14,0x14,0x14,0x14,0x14}; ptr=s; } else if(c == '?') { static uint8_t s[]={0x20,0x40,0x45,0x48,0x30}; ptr=s; } else if(c == '"') { static uint8_t s[]={0,0x03,0,0x03,0}; ptr=s; } else if(c == ',') { static uint8_t s[]={0,0x50,0x30,0,0}; ptr=s; } else if(c == '|') { static uint8_t s[]={0x7F,0x00,0x00,0x00,0x00}; ptr=s; }
    if(!ptr) return; 
    for(int m=0;m<5;m++){ 
        uint8_t l=ptr[m]; 
        for(int n=0;n<7;n++) {
            if((l>>n)&1) { Put(x+m,y+n,col); if(bold) Put(x+m+1,y+n,col); } 
        }
    } 
}

void Text(int x, int y, const char* s, uint32_t col, bool bold) { if(!s) return; while(*s) { DrawChar(x,y,*s++,col,bold); x+=(bold?7:6); } }
void TextC(int cp, int y, const char* s, uint32_t col, bool bold) { if(!s) return; int l=0; while(s[l])l++; Text(cp-(l*(bold?7:6))/2, y, s, col, bold); }
void DrawRoundedRect(int x, int y, int rw, int rh, int r, uint32_t c) { for(int iy=0;iy<rh;iy++)for(int ix=0;ix<rw;ix++){ bool corn=false; if(ix<r && iy<r && (r-ix)*(r-ix)+(r-iy)*(r-iy)>r*r) corn=true; if(ix>rw-r && iy<r && (ix-(rw-r))*(ix-(rw-r))+(r-iy)*(r-iy)>r*r) corn=true; if(ix<r && iy>rh-r && (r-ix)*(r-ix)+(iy-(rh-r))*(iy-(rh-r))>r*r) corn=true; if(ix>rw-r && iy>rh-r && (ix-(rw-r))*(ix-(rw-r))+(iy-(rh-r))*(iy-(rh-r))>r*r) corn=true; if(!corn) Put(x+ix,y+iy,c); } }
void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t c) { int minx=x1, maxx=x1, miny=y1, maxy=y1; if(x2<minx)minx=x2; if(x2>maxx)maxx=x2; if(y2<miny)miny=y2; if(y2>maxy)maxy=y2; if(x3<minx)minx=x3; if(x3>maxx)maxx=x3; if(y3<miny)miny=y3; if(y3>maxy)maxy=y3; for(int y=miny; y<=maxy; y++) { for(int x=minx; x<=maxx; x++) { int w1 = (x2-x1)*(y-y1) - (y2-y1)*(x-x1); int w2 = (x3-x2)*(y-y2) - (y3-y2)*(x-x2); int w3 = (x1-x3)*(y-y3) - (y1-y3)*(x-x3); if((w1>=0 && w2>=0 && w3>=0) || (w1<=0 && w2<=0 && w3<=0)) Put(x,y,c); } } }
void DrawFace(int x, int y, int mx, int my) { DrawRoundWindow(x, y, 60, 0x555555); int ex1 = x - 20; int ey = y - 10; int ex2 = x + 20; int off_x = (mx - x) / 20; if(off_x > 5) off_x = 5; if(off_x < -5) off_x = -5; int off_y = (my - y) / 20; if(off_y > 5) off_y = 5; if(off_y < -5) off_y = -5; DrawRoundedRect(ex1-8, ey-5, 16, 10, 2, 0xFFFFFF); DrawRoundedRect(ex2-8, ey-5, 16, 10, 2, 0xFFFFFF); Put(ex1 + off_x, ey + off_y, 0x000000); Put(ex1 + off_x+1, ey + off_y, 0x000000); Put(ex2 + off_x, ey + off_y, 0x000000); Put(ex2 + off_x+1, ey + off_y, 0x000000); DrawRoundedRect(x-15, y+25, 30, 2, 0, 0x000000); }
void DrawWaveform(int x, int y, int w, int f) { for(int i=0; i<w; i+=2) { int h = Sin(i*5 + f*10) / 4; if(random()%10 > 7) h += (random()%10)-5; Put(x+i, y+20+h, 0x000000); } }

void boot_logo() {
    uint32_t g=0xFFD700; int c=400, y=200;
    TextC(c,y,"#####################################",g,1);
    TextC(c,y+20," SCHNEIDER SYSTEM v2.0 (SECURE) ",0xFFFFFF,1);
    TextC(c,y+40," ID-HASHING: ACTIVE | NET: SIGNED ",0xAAAAAA,0);
    TextC(c,y+80," >> SYSTEM READY << ",g,1);
    TextC(c,y+100,"#####################################",g,1);
}

void focus_window(int id) { int found_at = -1; for(int i=0; i<10; i++) if(win_z[i] == id) found_at = i; if(found_at == -1) return; for(int i=found_at; i<9; i++) win_z[i] = win_z[i+1]; win_z[9] = id; }

// ==========================================
// 6. HARDWARE & PERIPHERALS
// ==========================================
void mouse_wait(uint8_t type) { uint32_t t = 100000; while(t--) { if(type==0 && (inb(0x64)&1)) return; if(type==1 && !(inb(0x64)&2)) return; } }
void mouse_write(uint8_t w) { mouse_wait(1); outb(0x64, 0xD4); mouse_wait(1); outb(0x60, w); }
uint8_t mouse_read() { mouse_wait(0); return inb(0x60); }
void init_mouse() { mouse_wait(1); outb(0x64,0xA8); mouse_wait(1); outb(0x64,0x20); mouse_wait(0); uint8_t s=inb(0x60)|2; mouse_wait(1); outb(0x64,0x60); mouse_wait(1); outb(0x60,s); mouse_write(0xF6); mouse_read(); mouse_write(0xF4); mouse_read(); }
void handle_input() { 
    key_new = false; mouse_just_pressed = false; uint8_t st = inb(0x64); 
    if(st & 1) { 
        uint8_t d = inb(0x60); 
        if(st & 0x20) { 
            if(mouse_cycle==0 && (d&8)) { mouse_byte[0]=d; mouse_cycle++; } else if(mouse_cycle==1) { mouse_byte[1]=d; mouse_cycle++; } else if(mouse_cycle==2) { mouse_byte[2]=d; mouse_cycle=0; mouse_x+=(int8_t)mouse_byte[1]; mouse_y-=(int8_t)mouse_byte[2]; if(mouse_x<0)mouse_x=0; if(mouse_x>799)mouse_x=799; if(mouse_y<0)mouse_y=0; if(mouse_y>599)mouse_y=599; bool cur_left=(mouse_byte[0]&1); if(cur_left && !mouse_left) mouse_just_pressed=true; mouse_left=cur_left; if(active_mode == 1) active_mode = 2; } 
        } else { 
            if(d == 0x2A || d == 0x36) key_shift = true; else if(d == 0xAA || d == 0xB6) key_shift = false; else if(!(d & 0x80)) { last_key = d; key_new = true; } 
            if(active_mode == 1) active_mode = 2; 
        } 
    } 
}

char get_ascii(uint8_t q) {
    static const char m[] = {0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
    if(q >= sizeof(m)) return 0;
    char c = m[q];
    if(key_shift && c >= 'a' && c <= 'z') return c - 32;
    return c;
}

// Netzwerk-Dummies (damit der Linker nicht meckert)
void send_udp(uint32_t dip, uint16_t sp, uint16_t dp, const char* msg) { (void)dip; (void)sp; (void)dp; (void)msg; }
void send_tcp_syn(uint32_t dip, uint16_t dp) { (void)dip; (void)dp; }

uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) { uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000)); outl(0xCF8, address); return inl(0xCFC); }

void rtl8139_init(uint64_t io_addr) { 
    rtl_io_base = io_addr & ~3; 
    outb(rtl_io_base + 0x52, 0x00); outb(rtl_io_base + 0x37, 0x10); 
    while((inb(rtl_io_base + 0x37) & 0x10) != 0) { } 
    outb(rtl_io_base + 0x37, 0x0C); 
    char* p = mac_str; for(int i=0; i<6; i++) { mac_addr[i] = inb(rtl_io_base + i); byte_to_hex(mac_addr[i], p); p+=2; if(i<5) *p++ = ':'; } *p = 0; str_cpy(cmd_status, "REALTEK ACTIVE"); str_cpy(ip_address, "DHCP (RTL)..."); 
}
void intel_e1000_init(uint64_t mmio_addr) { intel_mem_base = mmio_addr & 0xFFFFFFF0; if(intel_mem_base > 0) { uint32_t ral = mmio_read32(intel_mem_base + 0x5400); uint32_t rah = mmio_read32(intel_mem_base + 0x5404); mac_addr[0] = (uint8_t)(ral); mac_addr[1] = (uint8_t)(ral >> 8); mac_addr[2] = (uint8_t)(ral >> 16); mac_addr[3] = (uint8_t)(ral >> 24); mac_addr[4] = (uint8_t)(rah); mac_addr[5] = (uint8_t)(rah >> 8); char* p = mac_str; for(int i=0; i<6; i++) { byte_to_hex(mac_addr[i], p); p+=2; if(i<5) *p++ = ':'; } *p = 0; str_cpy(cmd_status, "INTEL NIC ACTIVE"); str_cpy(ip_address, "DHCP (INTEL)..."); } }
void uhci_init(uint64_t io_addr) { usb_io_base = io_addr & ~3; outw(usb_io_base, 4); for(int i=0;i<10000;i++); outw(usb_io_base, 0); outw(usb_io_base, 1); if(inb(usb_io_base + 2) & 0) str_cpy(cmd_status, "USB INIT FAIL"); else { str_cpy(cmd_status, "USB HOST RUNNING"); str_cpy(ip_address, "USB SCANNING..."); str_cpy(wifi_ssids[0], "FritzBox (ENC)"); str_cpy(wifi_ssids[1], "Telekom-AB12"); str_cpy(wifi_ssids[2], "Linksys-Home"); str_cpy(wifi_ssids[3], "Open-Wifi"); } }

void nic_select_next() { if(nic_count == 0) return; active_nic_idx++; if(active_nic_idx >= nic_count) active_nic_idx = 0; str_cpy(hw_net, found_nics[active_nic_idx].name); if(found_nics[active_nic_idx].type == 1) rtl8139_init(found_nics[active_nic_idx].address); else if(found_nics[active_nic_idx].type == 2) intel_e1000_init(found_nics[active_nic_idx].address); else if(found_nics[active_nic_idx].type == 3) { str_cpy(mac_str, "5C:F6:DC:32:C6:47"); if(usb_io_base != 0) uhci_init(usb_io_base); else str_cpy(cmd_status, "USB IO ERR"); windows[7].open = true; windows[7].minimized = false; wifi_state = 0; } }

void pci_scan_all() {
    nic_count = 0;
    for(uint8_t bus = 0; bus < 6; bus++) { 
        for(uint8_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0);
            if((id & 0xFFFF) != 0xFFFF) { 
                uint32_t class_rev = pci_read(bus, dev, 0, 0x08); uint8_t cls = (class_rev >> 24) & 0xFF; uint8_t sub = (class_rev >> 16) & 0xFF; uint16_t vendor = id & 0xFFFF; 
                if(cls == 0x01 || cls == 0x04) { uint32_t bar0 = pci_read(bus, dev, 0, 0x10); if((bar0 & 1) && (bar0 > 1)) { ata_base = (bar0 & 0xFFFFFFFC); } if(cls == 0x01 && sub == 0x08) { if(drive_count < 8) { int d = drive_count; drives[d].present = true; drives[d].type = 2; str_cpy(drives[d].model, "NVMe SSD (PCIE)"); drives[d].size_mb = 0; drive_count++; } } }
                if(cls == 0x02) { uint32_t bar0 = pci_read(bus, dev, 0, 0x10); if(nic_count < 5) { found_nics[nic_count].address = bar0; if(vendor == 0x10EC) { str_cpy(found_nics[nic_count].name, "REALTEK NIC"); found_nics[nic_count].type=1; } else if(vendor == 0x8086) { str_cpy(found_nics[nic_count].name, "INTEL GIGABIT"); found_nics[nic_count].type=2; } else { str_cpy(found_nics[nic_count].name, "GENERIC NIC"); found_nics[nic_count].type=0; } nic_count++; } }
                if(cls == 0x03) { str_cpy(hw_gpu, "GPU DEV"); }
                if(cls == 0x0C && sub == 0x03) { usb_detected = true; str_cpy(hw_usb, "USB HOST"); uint32_t bar4 = pci_read(bus, dev, 0, 0x20); if(bar4 & 1) usb_io_base = bar4 & ~3; if(nic_count < 5) { str_cpy(found_nics[nic_count].name, "SAMSUNG WIFI USB"); found_nics[nic_count].type = 3; nic_count++; } }
                if(vendor == 0x046D) { logitech_found = true; webcam_active = true; str_cpy(webcam_model, "LOGITECH C270"); } 
            }
        }
    }
    if(nic_count > 0) { active_nic_idx = -1; nic_select_next(); } else str_cpy(hw_net, "NO NIC FOUND");
}

int ata_wait_busy(uint16_t base) { int t=100000; while(t--) { if(!(inb(base+7)&0x80)) return 0; } return 1; }
int ata_wait_drq(uint16_t base) { int t=100000; while(t--) { if(inb(base+7)&0x08) return 0; } return 1; }
void ata_identify(uint16_t base, bool slave, int slot) { if(inb(base+7) == 0xFF) return; outb(base+6, slave ? 0xB0 : 0xA0); outb(base+2, 0); outb(base+3, 0); outb(base+4, 0); outb(base+5, 0); outb(base+7, 0xEC); if(inb(base+7) == 0) return; if(ata_wait_busy(base)) return; if(inb(base+4) != 0 || inb(base+5) != 0) return; if(ata_wait_drq(base)) return; insw(base, hdd_buf, 256); if(slot < 8) { drives[slot].present = true; drives[slot].base_port = base; drives[slot].is_slave = slave; drives[slot].type = 0; char* model = drives[slot].model; for(int i=0; i<20; i++) { uint16_t w = ((uint16_t*)hdd_buf)[27+i]; model[i*2] = (w >> 8); model[i*2+1] = (w & 0xFF); } model[40] = 0; for(int i=39; i>0; i--) if(model[i]==' ') model[i]=0; else break; uint32_t lba28 = ((uint16_t*)hdd_buf)[60] | (((uint16_t*)hdd_buf)[61] << 16); drives[slot].size_mb = (lba28 / 2048); drive_count++; } }
void ata_read_sector(uint16_t base, bool slave, uint32_t lba, uint8_t* buffer) { if(base == 0) return; if(ata_wait_busy(base)) return; outb(base+6, 0xE0 | (slave << 4) | ((lba >> 24) & 0x0F)); outb(base+1, 0x00); outb(base+2, 1); outb(base+3, (uint8_t)lba); outb(base+4, (uint8_t)(lba >> 8)); outb(base+5, (uint8_t)(lba >> 16)); outb(base+7, 0x20); if(ata_wait_drq(base)) return; insw(base, buffer, 256); }
void ata_write_sector(uint16_t base, bool slave, uint32_t lba, uint8_t* buffer) { if(base == 0) return; if(ata_wait_busy(base)) return; outb(base+6, 0xE0 | (slave << 4) | ((lba >> 24) & 0x0F)); outb(base+1, 0x00); outb(base+2, 1); outb(base+3, (uint8_t)lba); outb(base+4, (uint8_t)(lba >> 8)); outb(base+5, (uint8_t)(lba >> 16)); outb(base+7, 0x30); if(ata_wait_drq(base)) return; outsw(base, buffer, 256); }
int ata_probe(uint16_t port) { ata_base = port; outb(ata_base + 6, 0xA0); outb(ata_base + 7, 0xEC); if(inb(ata_base + 7) == 0) return 0; if(!ata_wait_busy(ata_base)) return 0; if(inb(ata_base + 7) & 1) return 0; insw(ata_base, hdd_buf, 256); return 1; }
void ata_scan_drives() { drive_count = 0; for(int i=0; i<8; i++) drives[i].present = false; ata_identify(0x1F0, false, 0); ata_identify(0x1F0, true, 1); ata_identify(0x170, false, 2); ata_identify(0x170, true, 3); if(usb_detected && drive_count < 8) { int d = drive_count; drives[d].present = true; drives[d].type = 1; str_cpy(drives[d].model, "USB MASS STORAGE"); drives[d].size_mb = 16000; drive_count++; } if(drive_count > 0) str_cpy(hw_disk, drives[0].model); else str_cpy(hw_disk, "NO DRIVES"); }
void mbr_scan() { if(drives[0].present && drives[0].type == 0) { ata_read_sector(drives[0].base_port, drives[0].is_slave, 0, sector0); if(sector0[510] == 0x55 && sector0[511] == 0xAA) { str_cpy(hw_disk, "DRIVE 0: MBR FOUND"); for(int i=0; i<4; i++) { int off = 446 + (i * 16); partitions[i].status = sector0[off]; partitions[i].type = sector0[off+4]; partitions[i].start_lba = *(uint32_t*)&sector0[off+8]; partitions[i].size = *(uint32_t*)&sector0[off+12]; } } else str_cpy(hw_disk, "DRIVE 0: UNKNOWN"); } }

// ==========================================
// 7. FILESYSTEM (USES HARDWARE) - HIERHIN VERSCHOBEN
// ==========================================
void fs_flush_table() { 
    if(drive_status && drives[0].type == 0) { 
        for(int i=0; i<512; i++) hdd_buf[i] = 0; 
        int off = 0; 
        for(int i=0; i<8; i++) { 
            if(file_table[i].exists) { 
                hdd_buf[off] = 1; 
                char* p = file_table[i].name; for(int k=0;k<30;k++) if(*p) hdd_buf[off+1+k] = *p++; else hdd_buf[off+1+k] = 0; 
                *(uint32_t*)&hdd_buf[off+32] = file_table[i].size; 
                hdd_buf[off+36] = file_table[i].is_folder ? 1 : 0;
                char* d = file_table[i].date; for(int k=0;k<12;k++) if(*d) hdd_buf[off+37+k] = *d++; else hdd_buf[off+37+k]=0;
                hdd_buf[off+50] = file_table[i].parent_idx; 
            } off += 64; 
        } 
        ata_write_sector(drives[0].base_port, drives[0].is_slave, 200, hdd_buf); 
    } 
}
void fs_create_folder(const char* name) { 
    for(int i=0; i<8; i++) { 
        if(!file_table[i].exists) { 
            file_table[i].exists = true; str_cpy(file_table[i].name, name); file_table[i].is_folder = true; file_table[i].size = 0; 
            read_rtc(); char ds[]="00.00.00"; ds[0]='0'+rtc_day/10; ds[1]='0'+rtc_day%10; ds[3]='0'+rtc_mon/10; ds[4]='0'+rtc_mon%10; ds[6]='0'+(rtc_year/10)%10; ds[7]='0'+rtc_year%10; 
            str_cpy(file_table[i].date, ds); 
            file_table[i].parent_idx = current_path_id; 
            fs_flush_table(); return; 
        } 
    } 
}
void fs_save_file(const char* name, uint32_t size) { 
    if(active_drive_idx != -1 && drives[active_drive_idx].type == 0) { 
        for(int i=0; i<512; i++) hdd_buf[i] = 0; 
        int idx = 0; 
        for(int r=0; r<10; r++) { 
            for(int c=0; c<40; c++) { 
                if(note_buf[r][c]) hdd_buf[idx++] = note_buf[r][c]; else hdd_buf[idx++] = 32; 
            } 
        } 
        ata_write_sector(drives[active_drive_idx].base_port, drives[active_drive_idx].is_slave, 1000, hdd_buf); 
        int slot = -1; for(int i=0; i<8; i++) { if(file_table[i].exists && str_equal(file_table[i].name, name)) { slot = i; break; } } 
        if(slot == -1) { for(int i=0; i<8; i++) { if(!file_table[i].exists) { slot = i; break; } } } 
        if(slot != -1) { 
            file_table[slot].exists = true; str_cpy(file_table[slot].name, name); file_table[slot].is_folder = false; file_table[slot].size = size; file_table[slot].sector_offset = 1000; 
            read_rtc(); char ds[]="00.00.2000"; ds[0]='0'+rtc_day/10; ds[1]='0'+rtc_day%10; ds[3]='0'+rtc_mon/10; ds[4]='0'+rtc_mon%10; ds[6]='0'+(rtc_year/1000)%10; ds[7]='0'+(rtc_year/100)%10; ds[8]='0'+(rtc_year/10)%10; ds[9]='0'+rtc_year%10;
            str_cpy(file_table[slot].date, ds); 
            file_table[slot].parent_idx = current_path_id; 
            fs_flush_table(); 
        } 
    } 
}
void fs_save() { fs_save_file("QUICK.TXT", 512); } 
void fs_init() { 
    pci_scan_all(); drive_status = 0; 
    if(ata_probe(ata_base)) drive_status = 1; else if(ata_probe(0x1F0)) drive_status = 1; else if(ata_probe(0x170)) drive_status = 1; 
    ata_scan_drives(); 
    if(drive_count > 0) mbr_scan(); 
    bool found_table = false;
    if(drive_status && drives[0].type == 0) { 
        ata_read_sector(drives[0].base_port, drives[0].is_slave, 200, hdd_buf); 
        if(hdd_buf[0] != 0) {
            found_table = true;
            int off = 0; 
            for(int i=0; i<8; i++) { 
                if(hdd_buf[off] == 1) { 
                    file_table[i].exists = true; char* p = file_table[i].name; for(int k=0;k<30;k++) *p++ = hdd_buf[off+1+k]; *p=0; 
                    file_table[i].size = *(uint32_t*)&hdd_buf[off+32];
                    file_table[i].is_folder = (hdd_buf[off+36] == 1);
                    char* d = file_table[i].date; for(int k=0;k<12;k++) *d++ = hdd_buf[off+37+k]; *d=0; 
                    file_table[i].parent_idx = hdd_buf[off+50]; 
                } else {
                    file_table[i].exists = false;
                }
                off += 64; 
            }
        }
    }
    if(!found_table) {
        for(int i=0; i<8; i++) file_table[i].exists = false;
        file_table[0].exists=true; str_cpy(file_table[0].name, "boot.sys"); file_table[0].is_folder=false; file_table[0].size=512; str_cpy(file_table[0].date, "01.01.2023"); file_table[0].parent_idx = 255;
        file_table[1].exists=true; str_cpy(file_table[1].name, "kernel.bin"); file_table[1].is_folder=false; file_table[1].size=10240; str_cpy(file_table[1].date, "22.05.2023"); file_table[1].parent_idx = 255;
    } 
    str_cpy(wifi_ssids[0], "Home-WiFi (90%)"); str_cpy(wifi_ssids[1], "Telekom-AB12"); str_cpy(wifi_ssids[2], "Office-Net"); str_cpy(wifi_ssids[3], "Guest-Access"); 
}

// ==========================================
// KERNEL ENTRY POINT (LIMINE)
// ==========================================
// 1. FIX: Saubere Initialisierung ohne das kaputte Makro
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = { 0x671d7296d856516c, 0x3691275003158e04 },
    .revision = 0
};

extern "C" void _start(void) {
    // 2. FIX: Der richtige Variablenname lautet framebuffer_request (nicht fb_req)
    if (framebuffer_request.response == nullptr || framebuffer_request.response->framebuffer_count < 1) {
        while(1) asm("hlt");
    }

    // Auch hier den Namen anpassen!
    struct limine_framebuffer *lfb = framebuffer_request.response->framebuffers[0];

    // FIX: Expliziter Cast und Byte-Mathematik für sichere Pointer
    fb = (uint32_t*)lfb->address;
    bb = (uint32_t*)((uint8_t*)lfb->address + (lfb->pitch * lfb->height) + 0x100000); 
    pitch = lfb->pitch;      
    screen_w = lfb->width;
    screen_h = lfb->height;
    
    // Reihenfolge ist jetzt sicher: Funktionen sind oben definiert.
    init_mouse();
    get_cpu_brand();
    fs_init();
    startup_melody();
    
    str_cpy(sleep_labels[0], "SLEEP (LIGHT)"); str_cpy(sleep_labels[1], "SLEEP (DEEP)"); str_cpy(sleep_labels[2], "HIBERNATE");
    str_cpy(sleep_labels[3], "HYBRID SLEEP"); str_cpy(sleep_labels[4], "LOCK SCREEN"); str_cpy(sleep_labels[5], "LOG OFF");

    windows[0].id=0; str_cpy(windows[0].title, "NOTEPAD"); windows[0].x=100; windows[0].y=100; windows[0].w=400; windows[0].h=300; windows[0].color=0xEEEEEE;
    windows[1].id=1; str_cpy(windows[1].title, "APPS");    windows[1].x=150; windows[1].y=150; windows[1].w=350; windows[1].h=250; windows[1].color=0xDDDDDD;
    windows[2].id=2; str_cpy(windows[2].title, "SAVE AS..."); windows[2].x=200; windows[2].y=150; windows[2].w=300; windows[2].h=200; windows[2].color=0xDDDDDD;
    windows[3].id=3; str_cpy(windows[3].title, "SYSTEM");  windows[3].x=200; windows[3].y=50;  windows[3].w=350; windows[3].h=480; windows[3].color=0xFFD700;
    windows[4].id=4; str_cpy(windows[4].title, "DISK MGR"); windows[4].x=250; windows[4].y=200; windows[4].w=450; windows[4].h=350; windows[4].color=0x888888;
    windows[5].id=5; str_cpy(windows[5].title, "CMD");      windows[5].x=100; windows[5].y=300; windows[5].w=450; windows[5].h=250; windows[5].color=0x111111;
    windows[6].id=6; str_cpy(windows[6].title, "GENESIS DB"); windows[6].x=150; windows[6].y=100; windows[6].w=500; windows[6].h=400; windows[6].color=0x222222;
    windows[7].id=7; str_cpy(windows[7].title, "WIFI MGR"); windows[7].x=200; windows[7].y=150; windows[7].w=300; windows[7].h=300; windows[7].color=0x004488;
    windows[8].id=8; str_cpy(windows[8].title, "ACCESS");   windows[8].x=300; windows[8].y=100; windows[8].w=250; windows[8].h=300; windows[8].color=0x44AAAA;
    windows[9].id=9; str_cpy(windows[9].title, "SENSES");   windows[9].x=400; windows[9].y=100; windows[9].w=300; windows[9].h=350; windows[9].color=0x222222;

    int clock_dirs[] = {213, 0, 42, 85, 128}; 
    for(int i=0;i<5;i++){ planets[i].ang = clock_dirs[i]; planets[i].dist = 10; planets[i].cur_x = 400; planets[i].cur_y = 300; str_cpy(planets[i].name, (i==0?"TXT":i==1?"APP":i==2?"SYS":i==3?"DSK":"CMD")); }
    for(int i=0;i<200;i++) stars[i]={(i*19)%799-399, (i*29)%599-299, (i*13)+1};
    int cx = 400; int cy = 300; 
    bool is_modal_blocked = false; bool click_consumed = false; 

    // MAIN LOOP
    while(1) {
        handle_input(); Clear();
        int map_ids[]={0,1,3,4,5}; if(input_cooldown > 0) input_cooldown--; 
        is_modal_blocked = (windows[2].open && !windows[2].minimized);
        z_blocked = false; click_consumed = false; 

        bool mouse_handled = false;
        if(mouse_left) {
             if(drag_win != -1) { windows[drag_win].x = mouse_x - drag_off_x; windows[drag_win].y = mouse_y - drag_off_y; mouse_handled=true; click_consumed=true; } 
             else if(resize_win != -1) { int nw = mouse_x - windows[resize_win].x; int nh = mouse_y - windows[resize_win].y; if(nw > 100) windows[resize_win].w = nw; if(nh > 100) windows[resize_win].h = nh; mouse_handled=true; click_consumed=true; }
        } else { drag_win = -1; resize_win = -1; }

        if(!mouse_handled) {
            for(int i=9; i>=0; i--) { 
                int k = win_z[i]; Window* win=&windows[k];
                if(win->open && !win->minimized) {
                    int wx=(win->fullscreen?0:win->x); int wy=(win->fullscreen?0:win->y); int ww=(win->fullscreen?800:win->w); int wh=(win->fullscreen?600:win->h);
                    if(mouse_x>=wx && mouse_x<=wx+ww && mouse_y>=wy && mouse_y<=wy+wh) {
                        z_blocked = true;
                        if(mouse_just_pressed) {
                           click_consumed = true; 
                           if(!is_modal_blocked || k == 2) { 
                               focus_window(k);
                               int bx = wx + ww/2; 
                               if(mouse_y < wy+40) { 
                                   if(mouse_x > bx-70 && mouse_x < bx-30) win->minimized=true; 
                                   else if(mouse_x > bx-20 && mouse_x < bx+40) win->fullscreen = !win->fullscreen; 
                                   else if(mouse_x > bx+45 && mouse_x < bx+70) { win->open=false; if(win->id == 4) { dsk_view=0; current_path_id=255; } }
                                   else { drag_win = k; drag_off_x = mouse_x - wx; drag_off_y = mouse_y - wy; }
                               }
                               if(mouse_x > wx+ww-20 && mouse_y > wy+wh-20) { resize_win = k; }
                           }
                        }
                        mouse_handled = true; break; 
                    }
                }
            }
        }

        if(key_new) {
            int top_id = win_z[9];
            if(!is_modal_blocked) {
                if(last_key >= 0x3B && last_key <= 0x3F) { 
                    int idx = last_key - 0x3B; int wid = map_ids[idx]; 
                    if(windows[wid].open) { if(windows[wid].minimized) windows[wid].minimized = false; else windows[wid].minimized = true; } else { windows[wid].open = true; windows[wid].minimized = false; }
                    if(windows[wid].open && !windows[wid].minimized) { focus_window(wid); if(wid == 4) { dsk_view=0; dsk_selection=0; active_drive_idx=-1; input_cooldown=10; } }
                }
                if(last_key == 0x43) { int sid = 9; if(windows[sid].open) { if(windows[sid].minimized) windows[sid].minimized = false; else windows[sid].minimized = true; } else { windows[sid].open = true; windows[sid].minimized = false; } if(windows[sid].open && !windows[sid].minimized) focus_window(sid); }
            }
            if(top_id == 3 && windows[3].open && !windows[3].minimized){ if(last_key==0x48 && sys_menu_selection > 0) sys_menu_selection--; if(last_key==0x50 && sys_menu_selection < 8) sys_menu_selection++; if(last_key==0x1C){ if(sys_menu_selection==0) system_reboot(); else if(sys_menu_selection==8) system_shutdown(); else if(sys_menu_selection>1) sleep_mode_idx = sys_menu_selection-2; } }
            if(top_id == 4 && windows[4].open && !windows[4].minimized && !is_modal_blocked) {
                if(dsk_view == 0) { 
                    if(last_key==0x48 && dsk_selection > 0) dsk_selection--; 
                    if(last_key==0x50 && dsk_selection < drive_count-1) dsk_selection++; 
                    if(last_key==0x1C && drive_count > 0) { active_drive_idx = dsk_selection; dsk_view = 1; file_selection = 0; input_cooldown=10; } 
                } else if(dsk_view == 1) { 
                    if(last_key==0x48 && file_selection > -1) file_selection--; 
                    if(last_key==0x50 && file_selection < 4) file_selection++; 
                    if(last_key==0x1C) { 
                        if(file_selection == -1) { dsk_view = 0; active_drive_idx = -1; input_cooldown=10; } 
                        else if(file_table[file_selection].exists) { 
                            if(file_table[file_selection].is_folder) { dsk_view=2; current_folder_view_idx=file_selection; current_path_id=file_selection; }
                            else { 
                                windows[0].open = true; windows[0].minimized = false; focus_window(0); 
                                if(drive_status) { 
                                    for(int r=0;r<10;r++) for(int c=0;c<40;c++) note_buf[r][c]=0; note_r=0; note_c=0;
                                    ata_read_sector(drives[active_drive_idx].base_port, drives[active_drive_idx].is_slave, file_table[file_selection].sector_offset, hdd_buf); 
                                    int idx = 0; 
                                    for(int r=0; r<10; r++) { 
                                        for(int c=0; c<40; c++) { char ch = hdd_buf[idx++]; if(ch < 32 || ch > 126) ch = 0; note_buf[r][c] = ch; } note_buf[r][40] = 0; 
                                    } 
                                } 
                            }
                        } 
                    }
                }
            }
        }

        if(active_mode == 0) { 
            int bf=++boot_frame; 
            if(bf < 60) DrawRoundWindow(400, 300, bf * 3, 0xFFD700); 
            else if(bf < 220) { Clear(); boot_logo(); if(bf==70) play_earthquake(15); }
            else active_mode = 1; 
        } else {
            int v_cx = 400; int v_cy = 300; 
            for(int i=0;i<200;i++){ stars[i].z -= 2; if(stars[i].z <= 0) { stars[i].z = 1000; stars[i].x = (random()%799)-399; stars[i].y = (random()%599)-299; } Put(v_cx + (stars[i].x * 256) / stars[i].z, v_cy + (stars[i].y * 256) / stars[i].z, 0x444444); }
            
            if(galaxy_open && galaxy_expansion < 320) galaxy_expansion += 8; 
            if(!galaxy_open && galaxy_expansion > 0) galaxy_expansion -= 10;
            
            DrawCentrifugalVortex(v_cx, v_cy, galaxy_expansion); DrawRoundWindow(v_cx, v_cy, 50, 0xFFD700); TextC(v_cx, v_cy-15, "COSMOS", 0x000000, true);
            if(frame%200==0) read_rtc(); 
            char ts[]="00:00"; ts[0]='0'+rtc_h/10; ts[1]='0'+rtc_h%10; ts[3]='0'+rtc_m/10; ts[4]='0'+rtc_m%10; TextC(v_cx, v_cy+5, ts, 0x000000, true); char ds[]="00.00.2000"; ds[0]='0'+rtc_day/10; ds[1]='0'+rtc_day%10; ds[3]='0'+rtc_mon/10; ds[4]='0'+rtc_mon%10; ds[8]='0'+(rtc_year%100)/10; ds[9]='0'+rtc_year%10; TextC(v_cx, v_cy+20, ds, 0x000000, true);

            for(int i=0; i<5; i++) {
                Window* win=&windows[map_ids[i]];
                int target_x, target_y; bool draw_moons = false;
                if(win->minimized) { target_x = 250 + (i * 70); target_y = 560; } 
                else if(win->open) { int orbit_dist = 60 + i*50; target_x = v_cx + (Cos(planets[i].ang) * orbit_dist) / 84; target_y = v_cy + (Sin(planets[i].ang) * orbit_dist * 3/4) / 84; draw_moons = true; }
                else {
                    if(galaxy_expansion >= 100) { if(planets[i].dist < 60 + i*50) planets[i].dist += 2; if(planets[i].dist > 50) { if(frame % (8+i) == 0) planets[i].ang = (planets[i].ang + 1) % 256; } } else { if(planets[i].dist > 10) planets[i].dist -= 8; }
                    target_x = v_cx + (Cos(planets[i].ang) * planets[i].dist) / 84; target_y = v_cy + (Sin(planets[i].ang) * planets[i].dist * 3/4) / 84;
                }
                planets[i].cur_x += (target_x - planets[i].cur_x) / 4; planets[i].cur_y += (target_y - planets[i].cur_y) / 4;
                if(galaxy_expansion > 10 || win->minimized || win->open) {
                    int px = planets[i].cur_x; int py = planets[i].cur_y;
                    bool hov = is_over(mouse_x, mouse_y, px, py, 20); 
                    if(hov && !z_blocked) DrawRoundWindow(px, py, 22, 0xDDDDDD); else DrawRoundWindow(px, py, 20, 0x999999);
                    if(draw_moons) { DrawRoundWindow(px-30, py, 4, 0xAAAAAA); DrawRoundWindow(px+30, py, 4, 0xAAAAAA); }
                    TextC(px, py-4, planets[i].name, 0x000000, true); 
                    if(!z_blocked && mouse_just_pressed && !is_modal_blocked && !click_consumed && hov) { 
                        if(win->minimized) win->minimized = false; 
                        else { win->open = true; focus_window(win->id); if(win->id == 4) { dsk_view=0; current_path_id=255; input_cooldown=15; } }
                    }
                }
            }
            if(key_shift) { DrawRoundedRect(740, 570, 50, 20, 2, 0xFFFFFF); Text(745, 575, "SHIFT", 0x000000, true); }

            for(int i=0; i<10; i++) {
                int k = win_z[i]; Window* win=&windows[k]; if(!win->open || win->minimized) continue;
                bool blocked = (is_modal_blocked && k != 2); 
                int wx=(win->fullscreen?0:win->x); int wy=(win->fullscreen?0:win->y); int ww=(win->fullscreen?800:win->w); int wh=(win->fullscreen?600:win->h);
                uint32_t win_bg = win->color; if(blocked) win_bg = 0x999999; if(win_bg == 0) win_bg = 0xCCCCCC; if(is_modal_blocked && k != 2) win_bg = 0x888888; 
                DrawRoundedRect(wx, wy, ww, wh, 12, win_bg);
                Text(wx+15, wy+15, win->title, 0x000000, true);
                int bx = wx + ww/2; Text(bx-60, wy+15, "MIN", 0x555555, true); Text(bx-10, wy+15, "FULL", 0x555555, true); Text(bx+50, wy+15, "X", 0x000000, true);
                
                if(win->id==3) { 
                    int mid=wx+ww/2; int btn_y = wy + 45; char lang_lbl[20], theme_lbl[30];
                    if(sys_lang==0) str_cpy(lang_lbl, "[ LANG: EN ]"); else str_cpy(lang_lbl, "[ SPR: DE ]");
                    if(sys_lang==0) { if(sys_theme==0) str_cpy(theme_lbl, "[ THEME: COMPUTER ]"); else str_cpy(theme_lbl, "[ THEME: GENESIS ]"); } else { if(sys_theme==0) str_cpy(theme_lbl, "[ THEMA: COMPUTER ]"); else str_cpy(theme_lbl, "[ THEMA: GENESIS ]"); }
                    if(input_cooldown == 0 && mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+5, btn_y, 140, 20)) { sys_lang = !sys_lang; input_cooldown = 20; }
                    Text(wx+10, btn_y+4, lang_lbl, 0x000000, true); 
                    if(input_cooldown == 0 && mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+5, btn_y+30, 200, 20)) { sys_theme = !sys_theme; input_cooldown = 20; }
                    Text(wx+10, btn_y+34, theme_lbl, 0x000000, true); 
                    
                    TextC(mid, wy+45, "REBOOT", (sys_menu_selection==0)?0x555555:0x000000, true); 
                    for(int s=0; s<6; s++) { 
                        int ly = wy+85+s*15; bool hov = is_over_rect(mouse_x, mouse_y, mid-50, ly-2, 100, 12);
                        uint32_t sc = (sleep_mode_idx == s || sys_menu_selection == s+2) ? 0x888888 : 0x000000; if(hov) sc = 0xFFFFFF;
                        if(hov) DrawRoundedRect(mid-50, ly-2, 100, 14, 2, 0x000000); TextC(mid, ly, sleep_labels[s], sc, true); if(mouse_just_pressed && !blocked && hov) sleep_mode_idx = s;
                    }
                    TextC(mid, wy+210, user_name, 0x222222, true); TextC(mid, wy+225, ip_address, 0x222222, true); TextC(mid, wy+240, cpu_brand, 0x222222, false); DrawRoundedRect(wx+20, wy+260, ww-40, 2, 0, 0xAAAAAA);
                    const char* l_hw = sys_lang ? "HARDWARE GEFUNDEN" : "DETECTED HARDWARE"; TextC(mid, wy+270, l_hw, 0x000000, true);
                    char l_disk[30], l_net[30], l_gpu[30], l_usb[30];
                    if(sys_theme == 0) { str_cpy(l_disk, sys_lang ? "FESTPLATTE:" : "STORAGE:"); str_cpy(l_net, sys_lang ? "NETZWERK:" : "NETWORK:"); str_cpy(l_gpu, sys_lang ? "GRAFIK:" : "GRAPHICS:"); str_cpy(l_usb, sys_lang ? "USB HOST:" : "USB HOST:"); } else { str_cpy(l_disk, sys_lang ? "ERD-ANKER:" : "TERRA ANCHOR:"); str_cpy(l_net, sys_lang ? "KOSMOS NETZ:" : "COSMIC WEB:"); str_cpy(l_gpu, sys_lang ? "REALITAETS RENDERER:" : "REALITY RENDERER:"); str_cpy(l_usb, sys_lang ? "DATEN INJEKTOR:" : "DATA INJECTOR:"); }
                    Text(wx+30, wy+290, l_disk, 0x555555, false); Text(wx+170, wy+290, hw_disk, 0x222222, false); 
                    Text(wx+30, wy+305, l_net, 0x000000, false); if(mouse_just_pressed && !blocked && is_over(mouse_x, mouse_y, wx+50, wy+305, 30)) { nic_select_next(); }
                    Text(wx+170, wy+305, hw_net, 0x222222, false); Text(wx+30, wy+320, l_gpu, 0x555555, false);  Text(wx+170, wy+320, hw_gpu, 0x222222, false); Text(wx+30, wy+335, l_usb, 0x555555, false);  Text(wx+170, wy+335, hw_usb, 0x222222, false);
                    TextC(mid, wy+wh-40, "SHUT DOWN", (sys_menu_selection==8)?0x555555:0x000000, true); if(mouse_just_pressed && !blocked) { if(is_over(mouse_x,mouse_y,mid,wy+45,20)) system_reboot(); if(is_over(mouse_x,mouse_y,mid,wy+wh-40,20)) system_shutdown(); }
                }
                
                if(win->id == 8) {
                    TextC(wx+ww/2, wy+40, "ACCESSIBILITY", 0x000000, true);
                    bool t_hov = is_over_rect(mouse_x, mouse_y, wx+20, wy+80, 210, 40);
                    DrawRoundedRect(wx+20, wy+80, 210, 40, 5, touch_mode ? 0x000000 : (t_hov ? 0x999999 : 0xCCCCCC));
                    TextC(wx+ww/2, wy+92, touch_mode ? "TOUCH MODE: ON" : "TOUCH MODE: OFF", 0xFFFFFF, true);
                    if(mouse_just_pressed && !blocked && t_hov) touch_mode = !touch_mode;
                    Text(wx+30, wy+140, "LOGITECH HARDWARE:", 0x000000, false);
                    Text(wx+30, wy+160, (logitech_found || webcam_active) ? "DETECTED (C270)" : "NOT FOUND", (logitech_found || webcam_active) ? 0x000000 : 0x555555, true);
                    DrawRoundedRect(wx+20, wy+240, 210, 30, 5, 0x333333); TextC(wx+ww/2, wy+247, "OPEN SENSES UI", 0xFFFFFF, true);
                    if(mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+20, wy+240, 210, 30)) { windows[9].open=true; windows[9].minimized=false; focus_window(9); }
                }
                if(win->id == 9) {
                    Text(wx+20, wy+40, "VISION", senses_tab==0?0x000000:0x555555, true); Text(wx+100, wy+40, "VOICE", senses_tab==1?0x000000:0x555555, true);
                    if(mouse_just_pressed && !blocked) { if(is_over_rect(mouse_x, mouse_y, wx+20, wy+35, 60, 20)) senses_tab=0; if(is_over_rect(mouse_x, mouse_y, wx+100, wy+35, 60, 20)) senses_tab=1; }
                    if(senses_tab == 0) { DrawRoundedRect(wx+50, wy+80, 200, 150, 5, 0x000000); DrawFace(wx+150, wy+155, mouse_x, mouse_y); DrawRoundedRect(wx+80, wy+250, 140, 30, 5, 0x444444); TextC(wx+150, wy+257, "CALIBRATE", 0xFFFFFF, true); } 
                    else { DrawRoundedRect(wx+50, wy+80, 200, 100, 5, 0xAAAAAA); DrawWaveform(wx+50, wy+110, 200, frame); DrawRoundedRect(wx+80, wy+200, 140, 30, 5, 0x444444); TextC(wx+150, wy+207, "TEACH: 'OPEN'", 0xFFFFFF, true); }
                }

                if(win->id==4) {
                    DrawRoundedRect(wx+ww-100, wy+15, 50, 20, 5, 0x555555); Text(wx+ww-90, wy+18, "REF", 0xFFFFFF, true); 
                    if(mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+ww-100, wy+15, 50, 20)) { fs_init(); input_cooldown = 20; } 
                    DrawRoundedRect(wx+ww-160, wy+15, 50, 20, 5, 0x333333); Text(wx+ww-150, wy+18, "HOME", 0xFFFFFF, true); 
                    if(mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+ww-160, wy+15, 50, 20)) { dsk_view=0; current_folder_view_idx=-1; current_path_id=255; input_cooldown=20; }
                    if(dsk_view == 0) { 
                        Text(wx+20, wy+50, "DETECTED DRIVES (ATA/IDE):", 0x333333, true); int y_off = wy+80;
                        if(drive_count == 0) Text(wx+20, y_off, "NO DRIVES FOUND", 0x000000, true);
                        else {
                            for(int d=0; d<drive_count; d++) {
                                bool sel = (d == dsk_selection); DrawRoundedRect(wx+20, y_off, ww-40, 40, 5, sel ? 0x999999 : 0xCCCCCC); DrawIcon(wx+30, y_off+5, 0x555555); Text(wx+55, y_off+12, drives[d].model, sel ? 0xFFFFFF : 0x000000, true);
                                char sz_str[20]; str_cpy(sz_str, "SIZE: "); int mb = drives[d].size_mb; sz_str[6] = '0' + (mb/10000)%10; sz_str[7] = '0' + (mb/1000)%10; sz_str[8] = '0' + (mb/100)%10; sz_str[9] = 'M'; sz_str[10] = 'B'; sz_str[11] = 0; Text(wx+ww-120, y_off+12, sz_str, sel ? 0xFFFFFF : 0x333333, true);
                                if(input_cooldown == 0 && mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+20, y_off, ww-40, 40)) { dsk_selection = d; active_drive_idx = d; dsk_view = 1; file_selection = 0; input_cooldown=15; } y_off += 50;
                            }
                        } TextC(wx+ww/2, wy+wh-30, "[ ARROWS: SELECT | ENTER: OPEN ]", 0x555555, true);
                    } else if(dsk_view == 1) { 
                        bool back_sel = (file_selection == -1); DrawRoundedRect(wx+20, wy+50, 60, 20, 5, back_sel ? 0x999999 : 0xCCCCCC); Text(wx+25, wy+55, "BACK", back_sel ? 0xFFFFFF : 0x000000, true); 
                        if(input_cooldown == 0 && mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+20, wy+50, 60, 20)) { dsk_view = 0; active_drive_idx = -1; input_cooldown=15; }
                        Text(wx+120, wy+55, "/ROOT/SYSTEM/", 0x000000, true); Text(wx+45, wy+90, "NAME", 0x555555, true); Text(wx+200, wy+90, "SIZE", 0x555555, true); Text(wx+280, wy+90, "DATE", 0x555555, true);
                        int y_off = wy+110;
                        for(int f=0; f<8; f++) {
                            if(file_table[f].exists && file_table[f].parent_idx == 255) {
                                bool sel = (f == file_selection); if(sel) DrawRoundedRect(wx+20, y_off, ww-40, 20, 0, 0xAAAAAA); DrawIcon(wx+25, y_off, file_table[f].is_folder ? 0x888888 : 0xCCCCCC); Text(wx+45, y_off+5, file_table[f].name, 0x000000, true);
                                if(!file_table[f].is_folder) { char s_str[10]; int_to_str(file_table[f].size, s_str); Text(wx+200, y_off+5, s_str, 0x333333, true); Text(wx+280, y_off+5, file_table[f].date, 0x333333, true); }
                                if(input_cooldown == 0 && mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+20, y_off, ww-40, 20)) { 
                                    file_selection = f; 
                                    if(file_table[f].is_folder) { dsk_view = 2; current_folder_view_idx = f; current_path_id = f; input_cooldown=10; } 
                                    else { windows[0].open = true; windows[0].minimized = false; focus_window(0); if(drive_status) { for(int r=0;r<10;r++) for(int c=0;c<40;c++) note_buf[r][c]=0; note_r=0; note_c=0; ata_read_sector(drives[active_drive_idx].base_port, drives[active_drive_idx].is_slave, file_table[f].sector_offset, hdd_buf); int idx = 0; for(int r=0; r<10; r++) { for(int c=0; c<40; c++) { char ch = hdd_buf[idx++]; if(ch < 32 || ch > 126) ch = 0; note_buf[r][c] = ch; } note_buf[r][40] = 0; } } }
                                } y_off += 25;
                            }
                        }
                    } else if(dsk_view == 2) { 
                         DrawRoundedRect(wx+20, wy+50, 60, 20, 5, 0xAAAAAA); Text(wx+25, wy+55, "BACK", 0xFFFFFF, true);
                         if(mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+20, wy+50, 60, 20)) { dsk_view = 1; current_path_id = 255; input_cooldown=10; } 
                         if(current_folder_view_idx >= 0) {
                             char title[50]; str_cpy(title, "INSIDE FOLDER: "); char* p = title; while(*p) p++; char* n = file_table[current_folder_view_idx].name; while(*n) *p++ = *n++; *p=0; Text(wx+120, wy+55, title, 0x000000, true); Text(wx+20, wy+100, "FILES IN FOLDER:", 0x333333, true); int y_off = wy+120;
                             for(int f=0; f<8; f++) {
                                 if(file_table[f].exists && file_table[f].parent_idx == current_folder_view_idx) {
                                     DrawIcon(wx+25, y_off, 0xCCCCCC); Text(wx+45, y_off+5, file_table[f].name, 0x000000, true);
                                     if(input_cooldown == 0 && mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+20, y_off, ww-40, 20)) { windows[0].open = true; windows[0].minimized = false; focus_window(0); if(drive_status) { for(int r=0;r<10;r++) for(int c=0;c<40;c++) note_buf[r][c]=0; note_r=0; note_c=0; ata_read_sector(drives[active_drive_idx].base_port, drives[active_drive_idx].is_slave, file_table[f].sector_offset, hdd_buf); int idx = 0; for(int r=0; r<10; r++) { for(int c=0; c<40; c++) { char ch = hdd_buf[idx++]; if(ch < 32 || ch > 126) ch = 0; note_buf[r][c] = ch; } note_buf[r][40] = 0; } } } y_off += 25;
                                 }
                             }
                         }
                    }
                }

                if(win->id==0) { 
                    if(key_new && win_z[9] == 0) { if(last_key < 58) { char c = get_ascii(last_key); if(last_key==0x1C && note_r<9){note_r++; note_c=0;} else if(last_key==0x0E) { if(note_c>0) {note_c--; note_buf[note_r][note_c]=0;} else if(note_r>0) { note_r--; note_c=39; } } else if(c) { note_buf[note_r][note_c]=c; note_c++; if(note_c > 38) { note_c=0; if(note_r<9) note_r++; } } } }
                    for(int r=0;r<10;r++) Text(wx+20, wy+50+r*15, note_buf[r], 0x333333, true);
                    if((frame/20)%2==0 && win_z[9]==0) DrawChar(wx+20+(note_c*7), wy+50+note_r*15, '_', 0x111111, true);
                    int btn_y = wy+wh-25; Text(wx+20, btn_y, "[ SAVE ]", 0x333333, true); Text(wx+100, btn_y, "[ SAVE AS ]", 0x333333, true);
                    if(mouse_just_pressed && !blocked) {
                        if(is_over(mouse_x, mouse_y, wx+40, btn_y+5, 30)) { fs_save_file("QUICK.TXT", 512); read_rtc(); char ds[]="SAVED: 00.00.00 00:00"; ds[7]='0'+rtc_day/10; ds[8]='0'+rtc_day%10; ds[10]='0'+rtc_mon/10; ds[11]='0'+rtc_mon%10; ds[13]='0'+(rtc_year%100)/10; ds[14]='0'+rtc_year%10; ds[16]='0'+rtc_h/10; ds[17]='0'+rtc_h%10; ds[19]='0'+rtc_m/10; ds[20]='0'+rtc_m%10; str_cpy(save_timestamp, ds); input_cooldown=10; } 
                        if(is_over(mouse_x, mouse_y, wx+130, btn_y+5, 40)) { windows[2].open = true; windows[2].minimized = false; focus_window(2); save_step = 0; input_cooldown = 10; } 
                    }
                    Text(wx+ww-220, wy+wh-20, save_timestamp, 0x111111, true); 
                }
                
                if(win->id == 2) {
                    if(save_step == 0) { 
                        Text(wx+20, wy+40, "SELECT DESTINATION:", 0x000000, true); bool sel = (save_part_sel == 0); DrawRoundedRect(wx+20, wy+70, ww-40, 30, 5, sel ? 0x999999 : 0xDDDDDD); Text(wx+30, wy+80, "DRIVE 0: PART 1 (SYSTEM)", sel ? 0xFFFFFF : 0x000000, true);
                        if(input_cooldown == 0 && mouse_just_pressed && is_over_rect(mouse_x, mouse_y, wx+20, wy+70, ww-40, 30)) { save_part_sel = 0; input_cooldown = 10; }
                        Text(wx+20, wy+120, "FOLDERS:", 0x555555, true); char path_display[30] = "/ROOT/"; if(current_path_id != 255) { str_cpy(path_display, "/"); char* p=path_display+1; char* n=file_table[current_path_id].name; while(*n) *p++ = *n++; *p++='/'; *p=0; }
                        DrawRoundedRect(wx+20, wy+140, ww-40, 60, 2, 0xEEEEEE); Text(wx+30, wy+150, path_display, 0x000000, true);
                        DrawRoundedRect(wx+20, wy+180, 100, 25, 5, 0xCCCCCC); TextC(wx+70, wy+187, "NEW FOLDER", 0x000000, true);
                        if(input_cooldown == 0 && mouse_just_pressed && is_over_rect(mouse_x, mouse_y, wx+20, wy+180, 100, 25)) { save_step = 2; input_cooldown = 15; }
                        DrawRoundedRect(wx+ww-100, wy+wh-40, 80, 25, 5, 0x555555); TextC(wx+ww-60, wy+wh-32, "NEXT >", 0xFFFFFF, true);
                        if(input_cooldown == 0 && mouse_just_pressed && is_over_rect(mouse_x, mouse_y, wx+ww-100, wy+wh-40, 80, 25)) { save_step = 1; input_cooldown = 15; }
                    } else if(save_step == 1) { 
                        if(key_new && win_z[9] == 2) { if(last_key == 0x0E && save_name_idx > 0) save_filename[--save_name_idx] = 0; else if(save_name_idx < 30) { char c = get_ascii(last_key); if(c >= 32) { save_filename[save_name_idx++] = c; save_filename[save_name_idx]=0; } } }
                        Text(wx+20, wy+40, "ENTER FILENAME:", 0x000000, true); DrawRoundedRect(wx+20, wy+60, ww-40, 25, 2, 0xCCCCCC); Text(wx+25, wy+65, save_filename, 0x000000, true); if((frame/20)%2==0) DrawChar(wx+25+(save_name_idx*7), wy+65, '_', 0x000000, true);
                        DrawRoundedRect(wx+ww-100, wy+wh-40, 80, 25, 5, 0x555555); TextC(wx+ww-60, wy+wh-32, "SAVE", 0xFFFFFF, true);
                        if(input_cooldown == 0 && mouse_just_pressed && is_over_rect(mouse_x, mouse_y, wx+ww-100, wy+wh-40, 80, 25)) { fs_save_file(save_filename, 1024); win->open = false; input_cooldown = 20; }
                    } else if(save_step == 2) { 
                        if(key_new && win_z[9] == 2) { if(last_key == 0x0E && folder_name_idx > 0) new_folder_name[--folder_name_idx] = 0; else if(folder_name_idx < 30) { char c = get_ascii(last_key); if(c >= 32) { new_folder_name[folder_name_idx++] = c; new_folder_name[folder_name_idx]=0; } } }
                        Text(wx+20, wy+40, "FOLDER NAME:", 0x000000, true); DrawRoundedRect(wx+20, wy+60, ww-40, 25, 2, 0xCCCCCC); Text(wx+25, wy+65, new_folder_name, 0x000000, true); if((frame/20)%2==0) DrawChar(wx+25+(folder_name_idx*7), wy+65, '_', 0x000000, true);
                        DrawRoundedRect(wx+ww-100, wy+wh-40, 80, 25, 5, 0x555555); TextC(wx+ww-60, wy+wh-32, "CREATE", 0xFFFFFF, true);
                        if(input_cooldown == 0 && mouse_just_pressed && is_over_rect(mouse_x, mouse_y, wx+ww-100, wy+wh-40, 80, 25)) { fs_create_folder(new_folder_name); save_step = 0; input_cooldown = 15; }
                    }
                }
                
                if(win->id==1) { TextC(wx+ww/2, wy+40, "INSTALLED APPS", 0x222222, true); DrawIcon(wx+ww/2-15, wy+80, 0xFFD700); TextC(wx+ww/2, wy+135, "GENESIS DB", 0x333333, true); if(mouse_just_pressed && !blocked && is_over(mouse_x, mouse_y, wx+ww/2-15, wy+80, 40)) { windows[6].open=true; windows[6].minimized=false; focus_window(6); } DrawRoundedRect(wx+ww/2-50, wy+180, 100, 30, 5, 0xAAAAAA); TextC(wx+ww/2, wy+188, "ACCESS", 0xFFFFFF, true); if(mouse_just_pressed && !blocked && is_over_rect(mouse_x, mouse_y, wx+ww/2-50, wy+180, 100, 30)) { windows[8].open=true; windows[8].minimized=false; focus_window(8); } }
                
                if(win->id==5) {
                    if(key_new && win_z[9] == 5) {
                        if(last_key == 0x1C) { 
                            cmd_input[cmd_idx] = 0; for(int l=4; l>0; l--) str_cpy(cmd_lines[l], cmd_lines[l-1]); str_cpy(cmd_lines[0], cmd_input);
                            uint32_t cid = get_id(cmd_input);
                            if(str_equal(cmd_input, "DIR")) { str_cpy(cmd_last_out, ""); int p=0; for(int i=0;i<8;i++) { if(file_table[i].exists && file_table[i].parent_idx == 255) { const char* n=file_table[i].name; while(*n && p<38) cmd_last_out[p++]=*n++; if(p<38) cmd_last_out[p++]=' '; } } cmd_last_out[p]=0; } 
                            else if(str_equal(cmd_input, "HELP")) str_cpy(cmd_last_out, "CMD: DIR, NET, PING, IPCONFIG, CLS");
                            else if(str_equal(cmd_input, "CLS")) { for(int l=0;l<5;l++) cmd_lines[l][0]=0; str_cpy(cmd_last_out, "READY."); }
                            else if(str_equal(cmd_input, "REBOOT")) system_reboot();
                            else if(str_equal(cmd_input, "IPCONFIG")) { str_cpy(cmd_last_out, "MAC: "); char* p=cmd_last_out+5; char* m=mac_str; while(*m) *p++=*m++; *p=0; }
                            else if(str_equal(cmd_input, "CONNECT")) { send_udp(0xFFFFFFFF, 5000, 80, "COSMOS HELLO"); }
                            else if(str_starts(cmd_input, "TCP")) { send_tcp_syn(0xFFFFFFFF, 80); }
                            else if(str_starts(cmd_input, "GENESIS")) { if(cid == 156) { genesis_tab = 0; str_cpy(cmd_last_out, "ACCESS GRANTED [ID 156]"); } else { str_cpy(cmd_last_out, "AUTH FAILED"); } }
                            else if(str_starts(cmd_input, "PING")) { if(str_equal(cmd_status, "ONLINE") || str_starts(cmd_status, "ONLINE")) str_cpy(cmd_last_out, "REPLY FROM HOST: TIME=1ms"); else str_cpy(cmd_last_out, "FAIL: NO CONNECTION"); }
                            else str_cpy(cmd_last_out, "UNKNOWN COMMAND");
                            cmd_idx = 0;
                        } else if(last_key == 0x0E && cmd_idx > 0) { cmd_idx--; cmd_input[cmd_idx] = 0; }
                        else if(cmd_idx < 30) { char c = get_ascii(last_key); if(c >= 32) { cmd_input[cmd_idx++] = c; cmd_input[cmd_idx] = 0; } }
                    }
                    Text(wx+20, wy+50, "STATUS:", 0xAAAAAA, true); Text(wx+90, wy+50, cmd_status, (str_equal(cmd_status, "ONLINE")||str_starts(cmd_status,"ONLINE")) ? 0x00FF00 : 0xFF0000, true);
                    for(int l=0; l<4; l++) if(cmd_lines[3-l][0] != 0) Text(wx+20, wy+80+(l*15), cmd_lines[3-l], 0x666666, true);
                    Text(wx+20, wy+150, "> ", 0x00FF00, true); Text(wx+35, wy+150, cmd_last_out, 0x00FF00, true);
                    Text(wx+20, wy+170, "C:\\>", 0x00FF00, true); Text(wx+60, wy+170, cmd_input, 0x00FF00, true);
                    if((frame / 15) % 2 == 0) DrawChar(wx+60+(cmd_idx*7), wy+170, '_', 0x00FF00, true);
                }
                if(win->id==6) { 
                    Text(wx+20, wy+40, "[ARCH]", genesis_tab==0?0x000000:0x555555, true); Text(wx+90, wy+40, "[HARD]", genesis_tab==1?0x000000:0x555555, true); Text(wx+160, wy+40, "[CODE]", genesis_tab==2?0x000000:0x555555, true);
                    if(mouse_just_pressed && !blocked && mouse_y > wy+30 && mouse_y < wy+50) { if(mouse_x > wx+20 && mouse_x < wx+80) genesis_tab=0; if(mouse_x > wx+90 && mouse_x < wx+150) genesis_tab=1; if(mouse_x > wx+160 && mouse_x < wx+220) genesis_tab=2; }
                    DrawRoundedRect(wx+20, wy+60, ww-40, 2, 0, 0x333333);
                    if(genesis_tab==0) { Text(wx+30, wy+80, "LEVEL 7: HIGH-ARCHITEKT (GOTT-MODUS)", 0x000000, true); Text(wx+30, wy+100, "LEVEL 6: BLUEPRINT (PLANUNG)", 0xAAAAAA, true); Text(wx+30, wy+120, "LEVEL 5: HIGH-LEVEL (OBJ-C)", 0xAAAAAA, true); Text(wx+30, wy+140, "LEVEL 4: MANAGED (C# LOGIK)", 0xAAAAAA, true); Text(wx+30, wy+160, "LEVEL 3: COMPLEX (C++ REALITAET)", 0x000000, true); Text(wx+30, wy+180, "LEVEL 2: LOW-LEVEL (C BASIS)", 0xAAAAAA, true); Text(wx+30, wy+200, "LEVEL 1: BINAER HIGH (EXISTENZ)", 0x555555, true); Text(wx+30, wy+220, "LEVEL 0: BINAER LOW (NICHTS)", 0x555555, true); } 
                    else if(genesis_tab==1) { Text(wx+30, wy+80, "ERDE (TERRA)", 0x000000, true); Text(wx+30, wy+95, "FUNKTION: KERNEL / STROM", 0xAAAAAA, true); Text(wx+30, wy+120, "SONNE (SOLAR)", 0x000000, true); Text(wx+30, wy+135, "FUNKTION: CPU / LASER / SCHREIBEN", 0xAAAAAA, true); Text(wx+30, wy+160, "MOND (LUNAR)", 0xCCCCCC, true); Text(wx+30, wy+175, "FUNKTION: RAM / SPIEGEL / SPEICHER", 0xAAAAAA, true); } 
                    else if(genesis_tab==2) { Text(wx+30, wy+80, "class NewbornSoul : public Wanderer {", 0x000000, true); Text(wx+30, wy+95, "  if (!ConnectToAnchor(TERRA_CORE))", 0xCCCCCC, true); Text(wx+30, wy+110, "     System::Panic(NO_POWER);", 0xFF0000, true); Text(wx+30, wy+125, "  LoadTribeTemplate(zodiac);", 0xCCCCCC, true); Text(wx+30, wy+140, "  SolarSystem::Laser::Broadcast();", 0xCCCCCC, true); Text(wx+30, wy+155, "}", 0x000000, true); }
                }
                if(win->id==7) {
                    TextC(wx+ww/2, wy+20, "SCAN RESULT", 0xFFFFFF, true);
                    if(key_new && win_z[9] == 7 && wifi_state == 2) { 
                        if(last_key == 0x0E && wifi_pass_idx > 0) wifi_pass[--wifi_pass_idx] = 0; else if(wifi_pass_idx < 30) { char c = get_ascii(last_key); if(c >= 32) { wifi_pass[wifi_pass_idx++] = c; wifi_pass[wifi_pass_idx]=0; } }
                    }
                    if(wifi_state == 0) { TextC(wx+ww/2, wy+100, "SCANNING AIR...", 0xAAAAAA, true); if(frame % 50 == 0) wifi_state = 1; } 
                    else if(wifi_state == 1) { for(int i=0; i<4; i++) { int y_off = wy+60 + (i*30); bool hov = is_over(mouse_x, mouse_y, wx+ww/2, y_off+10, 100); DrawRoundedRect(wx+20, y_off, ww-40, 25, 5, hov ? 0x999999 : 0x555555); Text(wx+30, y_off+5, wifi_ssids[i], 0xFFFFFF, true); if(mouse_just_pressed && !blocked && hov) { wifi_selected = i; wifi_state = 2; } } } 
                    else if(wifi_state == 2) { 
                        Text(wx+30, wy+60, "ENTER PASSWORD:", 0xAAAAAA, true); DrawRoundedRect(wx+30, wy+80, ww-60, 30, 0, 0xFFFFFF); Text(wx+35, wy+85, wifi_pass, 0x000000, true); 
                        int btn_y = wy+130; bool hov = is_over(mouse_x, mouse_y, wx+ww/2, btn_y+15, 60); 
                        DrawRoundedRect(wx+ww/2-50, btn_y, 100, 30, 5, hov ? 0x999999 : 0x333333); TextC(wx+ww/2, btn_y+7, "CONNECT", 0xFFFFFF, true); 
                        if(mouse_just_pressed && !blocked && hov) { 
                            wifi_state = 3; 
                            send_udp(0xFFFFFFFF, 5000, 80, "COSMOS HELLO"); 
                        } 
                    }
                    else if(wifi_state == 3) { 
                        TextC(wx+ww/2, wy+100, "CONNECTED!", 0x000000, true); 
                        TextC(wx+ww/2, wy+120, wifi_ssids[wifi_selected], 0xAAAAAA, true); 
                        if(frame % 60 == 0) str_cpy(cmd_status, "ONLINE (SECURE ID 84)");
                    }
                } 

                DrawTriangle(wx+ww-15, wy+wh, wx+ww, wy+wh-15, wx+ww, wy+wh, 0x888888); 
            } 

            if(!z_blocked && mouse_just_pressed && !is_modal_blocked && !click_consumed && is_over(mouse_x, mouse_y, cx, cy, 40)) 
                galaxy_open = !galaxy_open;

        } 

        for(int i=-2;i<=2;i++){ Put(mouse_x+i,mouse_y,0xFF0000); Put(mouse_x,mouse_y+i,0xFF0000); }
        Swap(); frame++; 
    } 
}