SECTIONS
{
    . = 0x0;
    /* Everything must be in one section to make relative loads as painless as possible */
    .text :
    {
        *(.text)
    }
    /DISCARD/ :
    {
        *(*)
    }
}