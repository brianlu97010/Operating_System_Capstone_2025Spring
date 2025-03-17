#include "fdt.h"
#include "malloc.h"
#include "string.h"
#include "types.h"
#include "muart.h"

static inline unsigned int be_to_le32(unsigned int be32_val){
    char* bytes = (char*)&be32_val;         // Casting to char* for byte-by-byte access
    return (unsigned int)(bytes[0]<<24) | (unsigned int)(bytes[1]<<16) | (unsigned int)(bytes[2]<<8) | (unsigned int)(bytes[3]);
}

/* Traverse the device tree and call the callback function for each node
 *
 * Tree structure will be looks like : 
 * FDT_BEGIN_NODE token
 *    The node’s name as a null-terminated string
 *    [zeroed padding bytes to align to a 4-byte boundary]
 * 
 * FDT_PROP token
 *    len - nameoff - property value
 *    [zeroed padding bytes to align to a 4-byte boundary]
 * 
 * FDT_END_NODE token
*/
void fdt_traverse(const void* fdt, fdt_callback_t callback_func, void* data){
    // points to the loading address of .dtb
    const fdt_header* header = fdt;     
    
    // Validate the FDT header
    if (be_to_le32(header->magic) != FDT_MAGIC){
        muart_puts("Invalid magic numer !\r\n");
        return;
    }
    
    // points to begining of the struct block, *struct_ptr represent a token which is a big-endian 32-bit integer 
    const unsigned int* struct_ptr = (const unsigned int*)((const char*)fdt + be_to_le32(header->off_dt_struct));   

    // Use to identify the node name is exactly we want
    int depth = 0;     

    // Iterate all token in struct block until the token is FDT_END
    while( be_to_le32(*struct_ptr) != FDT_END ){
        unsigned int token = be_to_le32(*struct_ptr++); 

        switch (token)
        {
            case FDT_BEGIN_NODE:{
                const char* node_name = (const char*)(struct_ptr);

                // Call the callback function
                if(!callback_func(fdt, struct_ptr, node_name, depth, data)){
                    // Find the node we want, stop iterating
                    return;
                }

                // Skip node name : advance the pointer, point to next token
                size_t name_len = strlen(node_name) + 1;    // + 1 is added to include the null terminator
                struct_ptr = (const unsigned int*)((const char*)struct_ptr + ((name_len + 3) & ~3));    // padding 0x0 to aligned on a 32-bit boundary
                depth++;
                break;  
            }

            case FDT_PROP:{
                // len -> nameoff -> prop_value are followed by FDT_PROP token
                // Both the fields in this structure are 32-bit big-endian integers.
                unsigned int len = be_to_le32(*struct_ptr++);       // the length of the property’s value in bytes
                unsigned int nameoff = be_to_le32(*struct_ptr++);   // n offset into the strings block at which the property’s name is stored as a null-terminated string.
                
                // Skip property value : advance the pointer, point to next token
                struct_ptr = (const unsigned int*)((const char*)struct_ptr + ((len + 3) & ~3));    // padding 0x0 to aligned on a 32-bit boundary
                break; 
            }
            case FDT_NOP: 
                break;
            
            case FDT_END_NODE:{
                // Back to previous level
                depth--;
                break;
            }

            default:{
                muart_puts("Error: some token is not expected ! \r\n");
                break;
            }
        }
    }
}

const void* fdt_get_property(const void* fdt, const void* node_ptr, const char* expect_name){
    const fdt_header* header = fdt;
    const unsigned int* struct_ptr = node_ptr;
    const char* strings_ptr = (const char*)fdt + be_to_le32(header->off_dt_strings); 

    // Skip node name
    const char* node_name = (const char*)struct_ptr;
    size_t name_len = strlen(node_name) + 1;    // + 1 is added to include the null terminator
    struct_ptr = (const unsigned int*)((const char*)struct_ptr + ((name_len + 3) & ~3));    // padding 0x0 to aligned on a 32-bit boundary
    // Now, struct_ptr points to next token FDT_PROP or FDT_END_NODE

    // Check the properties
    while( be_to_le32(*struct_ptr) == FDT_PROP ){
        struct_ptr++;   // points to FDT_PROP extra data : len

        unsigned int len = be_to_le32(*struct_ptr++);
        unsigned int nameoff = be_to_le32(*struct_ptr++);   // now points to property value
        const char* prop_name = strings_ptr + nameoff;      // Access property name from strings block
        const void* prop_value = struct_ptr;

        // Check the property name is the expected name
        if( strcmp(prop_name, expect_name) == 0 ){
            return prop_value;
        }

        // Advance to next token
        struct_ptr = (const unsigned int*)((const char*)struct_ptr + ((len + 3) & ~3));    // padding 0x0 to aligned on a 32-bit boundary
    }
}

int initramfs_callback(const void *fdt, const void *node_ptr, const char *node_name, int depth, void *data){
    initramfs_context_t* cxt = (initramfs_context_t*)data;

    //  Check if the node is the "/chosen" node
    if( (depth == 1) && (strcmp(node_name, "chosen") == 0) ){
        const unsigned int* prop_val_ptr = fdt_get_property(fdt, node_ptr, "linux,initrd-start");

        if( prop_val_ptr ){
            cxt->address = be_to_le32(*prop_val_ptr);
            cxt->found = 1;

            // Stop traversing since we find the node we want
            return 0;
        }
    }

    // Continue traversing
    return 1;
}

unsigned int get_initramfs_address(const void* fdt){
    // Initialize the context in initramfs
    muart_puts("Initializing the context in initramfs ! \r\n");
    initramfs_context_t initramfs_cxt;
    initramfs_cxt.found = 0;
    initramfs_cxt.address = 0; 

    muart_puts("Starting traverse the device tree ! \r\n");
    // Traverse the device tree to get the address of initramfs 
    fdt_traverse(fdt, initramfs_callback, &initramfs_cxt);

    if(initramfs_cxt.found){
        return initramfs_cxt.address;
    }
    else{
        muart_puts("Error : cxt not found \r\n");
        // Some error occurs
        return 1;
    }
}