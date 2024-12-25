# UART print hello example

# Load address and character
li $t1, 0x1000000
li $t2, 'h'

# Store character to memory at address
sb $t2, 0($t1)

li $t1, 0x1000000
li $t2, 'e'

sb $t2, 0($t1)

li $t1, 0x1000000
li $t2, 'l'

sb $t2, 0($t1)

li $t1, 0x1000000
li $t2, 'l'

sb $t2, 0($t1)

li $t1, 0x1000000
li $t2, 'o'

sb $t2, 0($t1)
