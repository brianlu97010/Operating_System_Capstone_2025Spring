#ifndef _FDT_H
#define _FDT_H

#define FDT_MAGIC        0xd00dfeed     // big-endian
#define FDT_BEGIN_NODE   0x1            // marks the beginning of a node’s representation
#define FDT_END_NODE     0x2            // marks the end of a node’s representation.
#define FDT_PROP         0x3            // marks the beginning of the representation of one property in the devicetree
#define FDT_NOP          0x4            // No Operation
#define FDT_END          0x9            // marks the end of the structure block

typedef struct fdt_header{
    unsigned int magic;                 // the value 0xd00dfeed (big-endian)
    unsigned int totalsize;             // the total size in bytes of the devicetree data structure
    unsigned int off_dt_struct;         // the offset in bytes of the structure block from the beginning of the header.
    unsigned int off_dt_strings;        // the offset in bytes of the string block from the beginning of the header.
    unsigned int off_mem_rsvmap;        // the offset in bytes of the memory reservation block from the beginning of the header.
    unsigned int version;               // the version of the devicetree data structure
    unsigned int last_comp_version;     // the lowest version of the devicetree data structure with which the version used is backwards compatible. 
    unsigned int boot_cpuid_phys;       // the physical ID of the system’s boot CPU.
    unsigned int size_dt_strings;       // the length in bytes of the strings block section of the devicetree blob.
    unsigned int size_dt_struct;        // the length in bytes of the structure block section of the devicetree blob.
}fdt_header;

typedef struct initramfs_context_t{
    int found;
    unsigned int address;
}initramfs_context_t;

/* Define a callback function by function pointer */
// return 1 -> continue iterate)  return 0 -> (stop iterate)
typedef int (*fdt_callback_t)(const void *fdt, const void *node_ptr, const char *node_name, int depth, void *data);

/* Traverse the device tree and call the callback function for each node */
void fdt_traverse(const void* fdt, fdt_callback_t callback_func, void* data);

/* Get the specific property's value, return a pointer point to the property value */
const void* fdt_get_property(const void* fdt, const void* node_ptr, const char* expect_name);

/* Callback function to find the initramfs address in traversing device tree */
int initramfs_callback(const void *fdt, const void *node_ptr, const char *node_name, int depth, void *data);

/* The API for getting the initramfs address from the device tree, return the address of initramfs */
unsigned int get_initramfs_address(const void* fdt);

#endif