PUBLIC __CKSSEFIXES_TESLandEditorFix

.code

	__CKSSEFIXES_TESLandEditorFix PROC

	mov [rsp + 18h], rbx
	mov [rsp + 20h], rsi
	push r14

	movss xmm0, dword ptr [rsp + 40h]
	mov r11d, r9d
	movss xmm2, dword ptr [rsp + 38h]
	xor r10d, r10d
	mov rax, 142E63450h
	movss xmm1, dword ptr [rax]
	xor eax, eax;
	subss xmm0, xmm2
	mov esi, dword ptr [rsp + 30h]
	mov rbx, r8
	mov r9, qword ptr [rsp + 48h]
	shr r11d, 2h
	mov r14d, edx
	divss xmm1, xmm0

	cmp esi, 4h
	jb label_1

	mov [rsp + 10h], rbp
	lea r8d, [r10 + 2h]
	mov [rsp + 18h], rdi
	lea ebp, [rsi - 3h]
	lea rdx, [r9 + 4h]

label_10:
	test r9, r9;
	je short label_2
	movzx eax, word ptr [rdx - 4h]
	jmp short label_3
label_2:
	mov eax, r10d
label_3:
	imul eax, r11d

	movss xmm0, dword ptr [rbx + rax * 4h]
	subss xmm0, xmm2
	mulss xmm0, xmm1
	cvttss2si rax, xmm0
	mov byte ptr [rcx], al
	add rcx, r14
	test r9, r9
	je short label_4
	movzx eax, word ptr [rdx - 2h]
	jmp short label_5
label_4:
	lea eax, [r8 - 1h]
label_5:
	imul eax, r11d

	movss xmm0, dword ptr [rbx + rax * 4h]
	subss xmm0, xmm2
	mulss xmm0, xmm1
	cvttss2si rax, xmm0
	mov byte ptr [rcx], al
	add rcx, r14
	test r9, r9
	je short label_6
	movzx eax, word ptr [rdx]
	jmp short label_7
label_6:
	mov eax, r8d
label_7:
	imul eax, r11d

	movss xmm0, dword ptr [rbx + rax * 4h]
	subss xmm0, xmm2
	mulss xmm0, xmm1
	cvttss2si rax, xmm0
	mov byte ptr [rcx], al
	add rcx, r14
	test r9, r9

	je short label_8
	movzx eax, word ptr [rdx + 2h]
	jmp short label_9
label_8:
	lea eax, [r8 + 1h]
label_9:
	imul eax, r11d

	add rdx, 8h
	add r10d, 4h
	add r8d, 4h

	movss xmm0, dword ptr [rbx + rax * 4h]
	subss xmm0, xmm2
	mulss xmm0, xmm1
	cvttss2si rax, xmm0
	mov byte ptr [rcx], al
	add rcx, r14
	
	cmp r10d, ebp
	jb label_10

	mov rdi, [rsp + 18h]
	mov rbp, [rsp + 10h]
label_1:

	cmp r10d, esi
	jae label_11

	lea rdx, [r9 + r10 * 2h]
label_14:

	test r9, r9
	je label_12
	movzx eax, word ptr[rdx]
	jmp short label_13
label_12:
	mov eax, r10d
label_13:
	imul eax, r11d
	add rdx, 2h
	inc r10d

	movss xmm0, dword ptr [rbx + rax * 4h]
	subss xmm0, xmm2
	mulss xmm0, xmm1
	cvttss2si rax, xmm0
	mov byte ptr [rcx], al
	add rcx, r14

	cmp r10d, esi
	jb label_14

label_11:

	mov rbx, [rsp + 20h]
	mov rsi, [rsp + 28h]
	pop r14
	ret

	__CKSSEFIXES_TESLandEditorFix ENDP

end

