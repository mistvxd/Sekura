global user_entry
extern system_init
user_entry:
	push 0x1B
    push 0x501000
    pushfq
    push 0x23
    push 0x400000
    iretq