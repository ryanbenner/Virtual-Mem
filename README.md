# Virtual Memory Manager

This project includes one primary files - vmmgr.c. The file contains methods to utilize a TLB and page tables in order to translate virtual addresses to physical addresses, and print the contents of each translation and address. There are various methods implemented in this project, such as an initialization of structures, TLB lookup, page table lookup, handling page faults, updating the TLB, and actually translating the addresses. It utilizies an addresses.txt file to provide the input and a BACKING_STORE.bin.

## Source Files

* vmmgr.c
* addresses.txt
* BACKING_STORE.bin
* README.md

## References

* man pages
* c documentation

## Known Errors

* n/a

## Build Insructions (2 Separate Terminals)

* gcc vmmgr.c -o vmmgr

## Execution Instructions (2 Separate Terminals)

* ./vmmgr addresses.txt

## Sample Output:

...
Logical address: 31260 Physical address: 5148 Value: 0
Logical address: 17071 Physical address: 5551 Value: -85
Logical address: 8940 Physical address: 5868 Value: 0
Logical address: 9929 Physical address: 6089 Value: 0
Logical address: 45563 Physical address: 6395 Value: 126
Logical address: 12107 Physical address: 6475 Value: -46

Total Translations: 1000
Page Faults: 538
Page Fault Rate: 53.800%
TLB Hits: 55
TLB Hit Rate: 5.500%

