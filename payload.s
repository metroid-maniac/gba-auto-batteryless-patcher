.arm
.align 4

# the following values are exposed for the benefit of the patcher program.
    .word patched_entrypoint
    .word original_entrypoint_addr
    .word write_sram_patched

patched_entrypoint:
    mov r0, # 0x04000000
    adr r1, idle_irq_handler
    str r1, [r0, # -4]
    ldr pc, original_entrypoint_addr

original_entrypoint_addr:
# a good guess but can be changed by patcher
    .word 0x080000c0

.thumb
# r0 = src, r1 = dst, r2 = size. Check if change before writing, only install irq if change
# unoptimised as hell, but I don't care for now.
write_sram_patched:
    push {r4, r5}
    mov r3, # 0
    add r2, r0
write_sram_patched_loop:
    ldrb r4, [r0]
    ldrb r5, [r1]
    cmp r4, r5
    beq (.+6)
    mov r3, # 1
    strb r4, [r1]
    add r0, # 1
    add r1, # 1
    cmp r0, r2
    blo write_sram_patched_loop

    cmp r3, # 0
    beq write_sram_patched_exit

    adr r0, countdown_irq_handler
    mov r1, # 0x04
    lsl r1, # 24
    sub r1, # 0x10
    mov r2, # 255
    strh r2, [r1, # 0x0a]
    str r0, [r1, # 0x04]
    
write_sram_patched_exit:
    mov r0, # 0
    pop {r4, r5}
    bx lr

.arm
# IRQ handlers are called with 0x04000000 in r0 which is handy! 
countdown_irq_handler:
    # if not vblank IF then user handler
    ldr r1, [r0, # 0x200]
    tst r1, # 0x00010000
    ldrne pc, [r0, # -12]
    
    # if (--counter) then user handler
    ldrh r1, [r0, # - 6]
    subs r1, # 1
    strh r1, [r0, # - 6]
    ldreq pc, [r0, # -12]

    # countdown expired - flush sram to flash
    b .

idle_irq_handler:
    ldr pc, [r0, # -12]
    
.ascii "<3 from Maniac"

# patcher program will have to ensure this is actually aligned enough
.align 4
    flash_save_sector:
