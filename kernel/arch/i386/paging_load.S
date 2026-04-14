.global paging_load_directory
.type paging_load_directory, @function

paging_load_directory:
    mov 4(%esp), %eax
    mov %eax, %cr3
    ret

.global paging_enable
.type paging_enable, @function

paging_enable:
    mov %cr0, %eax
    or  $0x80000000, %eax
    mov %eax, %cr0
    ret
