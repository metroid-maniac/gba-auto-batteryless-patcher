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
    
    adr r0, flash_save_sector
    mov r1, # 0x0e000000
    add r2, r1, # 0x00010000
sram_init_loop:
    ldrb r3, [r0], # 1
    strb r3, [r1], # 1
    cmp r1, r2
    blo sram_init_loop
    
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
    # Check if the each byte to write to sram is different - if it is, write it then set a flag
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

    # If the flag was not set, the function had no effect. Short circuit
    cmp r3, # 0
    beq write_sram_patched_exit

    # Install countdown irq handler and initialise countdown value
    adr r0, countdown_irq_handler
    mov r1, # 0x04
    lsl r1, # 24
    sub r1, # 0x10
    mov r2, # 255
    strh r2, [r1, # 0x0a]
    str r0, [r1, # 0x0c]
    # Set green swap as a visual indicator that the countdown has begun
    strh r2, [r1, # 0x12]
    
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
    ldreq pc, [r0, # -12]
    
    # if (--counter) then user handler
    ldrh r1, [r0, # - 6]
    subs r1, # 1
    strh r1, [r0, # - 6]
    ldreq pc, [r0, # -12]

    # countdown expired. time to flush sram to flash
    # first switch back into user mode with interrupts still masked so there is enough stack.
    # also disable sound.
    mrs r1, cpsr
    mrs r2, spsr
    ldrb r3, [r0, # 0x84]
    push {r0, r1, r2, r3, lr}
    mov r1, # 0x009f
    msr cpsr, r1
    
    pop {r0, r1, r2, r3, lr}
    msr cpsr, r1
    msr spsr, r2
    strb r3, [r0, # 0x84]
    
    # Disable green swap and reinstall idle irq
    strh r0, [r0, # 0x02]
    adr r1, idle_irq_handler
    str r1, [r0, # - 4]

idle_irq_handler:
    ldr pc, [r0, # -12]
    
.ascii "<3 from Maniac"

# patcher program will have to ensure this is actually aligned enough
.align 4
    flash_save_sector:
