#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 256
#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 256
#define PHYSICAL_FRAMES 128  /* Change to 128 for page replacement phase */

/* Global data structures */
int page_table[PAGE_TABLE_SIZE];
int tlb[TLB_SIZE][2];    /* [i][0]=page, [i][1]=frame] */
int tlb_usage[TLB_SIZE]; /* LRU timestamp */
int tlb_counter;
char physical_memory[PHYSICAL_FRAMES * PAGE_SIZE];
int free_frames[PHYSICAL_FRAMES];
int free_frame_count;
int fifo_queue[PHYSICAL_FRAMES];
int fifo_head, fifo_tail;
int loaded_pages[PHYSICAL_FRAMES];

int total_addresses = 0;
int tlb_hits = 0;
int page_faults = 0;

FILE *backing_store;

/* Function prototypes */ 
void init_structures(void); // initialize page table, TLB, free frames, and FIFO structures
int tlb_lookup(int page_number, int *frame_number); // check TLB for page, return 1 if hit and set frame, else 0
int page_table_lookup(int page_number); // return frame number from page table or -1 if absent
int handle_page_fault(int page_number); // load page into memory (or replace) and return frame number
void update_tlb(int page_number, int frame_number); // insert page to frame mapping into TLB using the LRU replacement
void translate(int logical_address); // bitwise address translation and print logical to physical + val
void print_stats(void); // output total translations, fault rates, and TLB hit rates

void init_structures(void) {
    // mark all pages as not here
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
        page_table[i] = -1;

    // clear the TLB entries, reset usage
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i][0] = -1;
        tlb[i][1] = -1;
        tlb_usage[i] = 0;
    }
    tlb_counter = 0;

    // initialize free frame stack (LIFO) to track unused phys frames
    for (int i = 0; i < PHYSICAL_FRAMES; i++)
        free_frames[i] = PHYSICAL_FRAMES - 1 - i;
    free_frame_count = PHYSICAL_FRAMES;

    // set FIFO queue pointers and mark frames empty
    fifo_head = fifo_tail = 0;
    for (int i = 0; i < PHYSICAL_FRAMES; i++)
        loaded_pages[i] = -1;
}

int tlb_lookup(int page_number, int *frame_number) {
    // search TLB for the page
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i][0] == page_number) {
            *frame_number = tlb[i][1];
            tlb_usage[i] = ++tlb_counter; // update LRU timestamp
            tlb_hits++; // increment hit count
            return 1; // TLB hit
        }
    }
    return 0; // TLB miss
}

int page_table_lookup(int page_number) {
    return page_table[page_number]; // return -1 if not loaded
}

int handle_page_fault(int page_number) {
    page_faults++; // increment fault count
    int frame_number;

    if (free_frame_count > 0) {
        // check for and use a free frame if available
        frame_number = free_frames[--free_frame_count];
    } else {
        // no free frame: pick oldest frame via FIFO
        int oldest_frame = fifo_queue[fifo_head];
        fifo_head = (fifo_head + 1) % PHYSICAL_FRAMES;

        int oldest_page = loaded_pages[oldest_frame];
        page_table[oldest_page] = -1; // make old page invalid
        // remove TLB entries for the old page
        for (int i = 0; i < TLB_SIZE; i++)
            if (tlb[i][0] == oldest_page)
                tlb[i][0] = tlb[i][1] = -1;

        frame_number = oldest_frame; // reuse the frame
    }

    // load the new page into the chosen frame
    loaded_pages[frame_number] = page_number;
    page_table[page_number] = frame_number;
    fifo_queue[fifo_tail] = frame_number;
    fifo_tail = (fifo_tail + 1) % PHYSICAL_FRAMES;

    // read page data from backing store
    fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
    fread(&physical_memory[frame_number * PAGE_SIZE],
          sizeof(char), PAGE_SIZE, backing_store);

    return frame_number;
}

void update_tlb(int page_number, int frame_number) {
    int lru_index = 0, free_index = -1;
    // find empty slot or LRU slot for replacement
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i][0] == -1) {
            free_index = i;
            break;
        }
        if (tlb_usage[i] < tlb_usage[lru_index])
            lru_index = i;
    }
    // if free index isnt -1, then index = free_index. else index = lru_index
    int index = (free_index != -1) ? free_index : lru_index;

    // new tlb mapping
    tlb[index][0] = page_number;
    tlb[index][1] = frame_number;
    tlb_usage[index] = ++tlb_counter;     // set fresh timestamp
}

void translate(int logical_address) {
    int page_number = (logical_address >> 8) & 0xFF; // high 8 bits
    int offset = logical_address & 0xFF;            // low 8 bits
    int frame_number;

    // try the TLB lookup first
    if (!tlb_lookup(page_number, &frame_number)) {
        int pt_frame = page_table_lookup(page_number);
        if (pt_frame == -1) {
            // page not in memory, page fault
            frame_number = handle_page_fault(page_number);
        } else {
            frame_number = pt_frame;     // page table hit
        }
        update_tlb(page_number, frame_number); // cache in TLB
    }

    // math to find physical address and retrieve val
    int physical_address = frame_number * PAGE_SIZE + offset;
    signed char value = physical_memory[physical_address];
    printf("Logical address: %d Physical address: %d Value: %d\n",
           logical_address, physical_address, value);
}

void print_stats(void) {
    printf("\nTotal Translations: %d\n", total_addresses);
    printf("Page Faults: %d\n", page_faults);
    printf("Page Fault Rate: %.3f%%\n",
           (page_faults / (double)total_addresses) * 100);
    printf("TLB Hits: %d\n", tlb_hits);
    printf("TLB Hit Rate: %.3f%%\n",
           (tlb_hits / (double)total_addresses) * 100);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *addr_file = fopen(argv[1], "r");
    if (!addr_file) {
        perror("Error opening address file");
        return EXIT_FAILURE;
    }

    backing_store = fopen("BACKING_STORE.bin", "rb");
    if (!backing_store) {
        perror("Error opening backing store");
        fclose(addr_file);
        return EXIT_FAILURE;
    }

    init_structures();

    int logical_address;
    while (fscanf(addr_file, "%d", &logical_address) == 1) {
        total_addresses++;
        translate(logical_address);
    }

    print_stats();

    fclose(addr_file);
    fclose(backing_store);
    return 0;
}
