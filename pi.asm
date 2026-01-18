; program to calculate pi

numDigits = 5
arrayLength = 10 * numDigits / 3
console = 0xfeff

; macro to copy a short from a to b
.macro copy_short a b:
	LOD a + 1
	STO b + 1
	LOD a
	STO b
.end

; macro to branch to c if short a is less than short b
.macro branch_short_less_than a b c:
	LOD a + 1
	SUB b + 1
	LOD a
	SBB b
	BRN c
.end

; macro to branch to c if short a is equal to short b
.macro branch_short_equal a b c:
	LOD a + 1
	SUB b + 1
	BNZ end
	LOD a
	SBB b
	BRZ c
end:
.end

; macro to increment a short
.macro increment_short x:
	LOD x + 1
	ADD #1
	STO x + 1
	LOD x
	ADC #0
	STO x
.end

; macro to decrement a short
.macro decrement_short x:
	LOD x + 1
	ADD #0xff
	STO x + 1
	LOD x
	ADC #0xff
	STO x
.end

; macro to add shorts a + b and store the result in c
.macro add_short a b c:
	LOD a + 1
	ADD b + 1
	STO c + 1
	LOD a
	ADC b
	STO c
.end

; macro to subtract shorts a - b and store the result in c
.macro subtract_short a b c:
	LOD a + 1
	SUB b + 1
	STO c + 1
	LOD a
	SBB b
	STO c
.end

; macro to multiply shorts a * b and store the result in c
.macro multiply_short a b c:
	LOD #0
	STO result + 1
	STO result
	copy_short a alpha
	copy_short b beta

loop:
	branch_short_less_than zero beta continue
	JMP end

continue:
	LOD beta + 1
	AND #1
	BRZ shift
	add_short alpha result result

shift:
	LSL alpha + 1
	STO alpha + 1
	RCL alpha
	STO alpha

	LSR beta
	STO beta
	RCR beta + 1
	STO beta + 1

	JMP loop

alpha:
	.byte 0
	.byte 0
beta:
	.byte 0
	.byte 0
result:
	.byte 0
	.byte 0

end:
	copy_short result c
.end

; macro to divide shorts a / b and store the result in c and the remainder in d
.macro divide_short a b c d:
	copy_short a remainder
	LOD #0
	STO quotient
	STO quotient + 1
loop:
	branch_short_less_than remainder b end
	subtract_short remainder b remainder
	increment_short quotient
	JMP loop

quotient:
	.byte 0
	.byte 0
remainder:
	.byte 0
	.byte 0

end:
	copy_short quotient c
	copy_short remainder d

.end


; macro to store the short value from x into short array a at index i
.macro short_to_array a i x:
	LSL i + 1
	STO pointer + 1
	RCL i
	STO pointer

	LOD #(a % 256)
	ADD pointer + 1
	STO pointer + 1
	LOD #(a / 256)
	ADC pointer
	STO pointer

	LOD x
	STO [pointer]
	increment_short pointer
	LOD x + 1
	STO [pointer]

	JMP end

pointer:
	.byte 0
	.byte 0
end:
.end

; macro to extract a short value from array a at index i and store it in x
.macro short_from_array a i x:LSL i + 1
	STO pointer + 1
	RCL i
	STO pointer

	LOD #(a % 256)
	ADD pointer + 1
	STO pointer + 1
	LOD #(a / 256)
	ADC pointer
	STO pointer

	LOD [pointer]
	STO x
	increment_short pointer
	LOD [pointer]
	STO x + 1

	JMP end

pointer:
	.byte 0
	.byte 0
end:
.end

; main program entry point

; initialize variables
	LOD #(numDigits % 256)
	STO n + 1
	LOD #(numDigits / 256)
	STO n
	LOD #(arrayLength % 256)
	STO len + 1
	LOD #(arrayLength / 256)
	STO len

	LOD #0
	STO nines + 1
	STO nines
	STO predigit + 1
	STO predigit

; initialize the array
	STO j + 1
	STO j

for_loop_1:
	branch_short_less_than j len for_loop_1_continue
	JMP for_loop_1_end

for_loop_1_continue:
	short_to_array a j two

	increment_short j
	JMP for_loop_1

for_loop_1_end:

; calculate the digits
LOD #0
STO j + 1
STO j

for_loop_2:
	branch_short_less_than j n for_loop_2_continue
	JMP for_loop_2_end

for_loop_2_continue:
	LOD #0
	STO q + 1
	STO q

	copy_short len i
for_loop_3:
	branch_short_less_than zero i for_loop_3_continue
	JMP for_loop_3_end

for_loop_3_continue:
	
	; x = 10 * a[i - 1] + q * i
	copy_short i temp
	decrement_short temp
	short_from_array a temp x
	multiply_short x ten x
	copy_short i temp
	multiply_short temp q temp
	add_short temp x x

	; a[i - 1] = x % (2 * i - 1)
	; q = x / (2 * i - 1)
	LSL i + 1
	STO temp + 1
	RCL i
	STO temp
	decrement_short temp
	divide_short x temp q temp
	decrement_short i
	short_to_array a i temp

	JMP for_loop_3

for_loop_3_end:

	; extract the digit
	divide_short q ten q a

	; handle carry cases
	LOD q + 1
	SUB #9
	BRZ case_9
	SUB #1
	BRZ case_10
	JMP case_default

case_9:
	increment_short nines
	JMP break

case_10:
	LOD predigit + 1
	ADD #49
	STO console

for_loop_4:
	branch_short_less_than zero nines for_loop_4_continue
	JMP for_loop_4_end

for_loop_4_continue:
	LOD #48
	STO console
	
	decrement_short nines
	JMP for_loop_4

for_loop_4_end:
	LOD #0
	STO predigit + 1
	STO predigit
	JMP break

case_default:
	LOD predigit + 1
	ADD #48
	STO console
	copy_short q predigit

for_loop_5:
	branch_short_less_than zero nines for_loop_5_continue
	JMP for_loop_5_end

for_loop_5_continue:
	LOD #(9 + 48)
	STO console

	decrement_short nines
	JMP for_loop_5

for_loop_5_end:

break:

	branch_short_equal j zero backspace
	branch_short_equal j one period
	JMP skip

backspace:
	LOD #8 ; backspace character
	STO console
	JMP skip

period:
	LOD #46 ; '.' character
	STO console

skip:

	increment_short j
	JMP for_loop_2

for_loop_2_end:

	; output the last digit
	LOD predigit + 1
	ADD #48
	STO console

	HLT

; variables and constants
temp:
	.byte 0
	.byte 0
zero:
	.byte 0
	.byte 0
one:
	.byte 0
	.byte 1
two:
	.byte 0
	.byte 2
ten:
	.byte 0
	.byte 10
n:	.byte 0
	.byte 0
len:
	.byte 0
	.byte 0
i:	.byte 0
	.byte 0
j:	.byte 0
	.byte 0
x:	.byte 0
	.byte 0
q:	.byte 0
	.byte 0
nines:
	.byte 0
	.byte 0
predigit:
	.byte 0
	.byte 0
a: