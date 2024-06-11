.intel_syntax noprefix

# Provides wrappers for first 201 linux x64 syscalls.

# According to this pdf: https://liujunming.top/pdf/syscall/Linux%20System%20Call%20Table%20for%20x86%2064%20%C2%B7%20Ryan%20A.%20Chapman.pdf
# This file also implements `_write` and `_exit` which are just jumps for their `sys_*` implementations.

.text

.global write
write: jmp sys_write 

.global exit
exit: jmp sys_exit 

.global sys_read
sys_read:
    mov rax, 0
    syscall
    ret

.global sys_write
sys_write:
    mov rax, 1
    syscall
    ret

.global sys_open
sys_open:
    mov rax, 2
    syscall
    ret

.global sys_close
sys_close:
    mov rax, 3
    syscall
    ret

.global sys_stat
sys_stat:
    mov rax, 4
    syscall
    ret

.global sys_fstat
sys_fstat:
    mov rax, 5
    syscall
    ret

.global sys_lstat
sys_lstat:
    mov rax, 6
    syscall
    ret

.global sys_poll
sys_poll:
    mov rax, 7
    syscall
    ret

.global sys_lseek
sys_lseek:
    mov rax, 8
    syscall
    ret

.global sys_mmap
sys_mmap:
    push r10
	mov r10, rcx
    mov rax, 9
    syscall
    pop r10
    ret

.global sys_mprotect
sys_mprotect:
    mov rax, 10
    syscall
    ret

.global sys_munmap
sys_munmap:
    mov rax, 11
    syscall
    ret

.global sys_brk
sys_brk:
    mov rax, 12
    syscall
    ret

.global sys_rt_sigaction
sys_rt_sigaction:
    push r10
	mov r10, rcx
    mov rax, 13
    syscall
    pop r10
    ret

.global sys_rt_sigprocmask
sys_rt_sigprocmask:
    push r10
	mov r10, rcx
    mov rax, 14
    syscall
    pop r10
    ret

.global sys_rt_sigreturn
sys_rt_sigreturn:
    mov rax, 15
    syscall
    ret

.global sys_ioctl
sys_ioctl:
    mov rax, 16
    syscall
    ret

.global sys_pread64
sys_pread64:
    push r10
	mov r10, rcx
    mov rax, 17
    syscall
    pop r10
    ret

.global sys_pwrite64
sys_pwrite64:
    push r10
	mov r10, rcx
    mov rax, 18
    syscall
    pop r10
    ret

.global sys_readv
sys_readv:
    mov rax, 19
    syscall
    ret

.global sys_writev
sys_writev:
    mov rax, 20
    syscall
    ret

.global sys_access
sys_access:
    mov rax, 21
    syscall
    ret

.global sys_pipe
sys_pipe:
    mov rax, 22
    syscall
    ret

.global sys_select
sys_select:
    push r10
	mov r10, rcx
    mov rax, 23
    syscall
    pop r10
    ret

.global sys_sched_yield
sys_sched_yield:
    mov rax, 24
    syscall
    ret

.global sys_mremap
sys_mremap:
    push r10
	mov r10, rcx
    mov rax, 25
    syscall
    pop r10
    ret

.global sys_msync
sys_msync:
    mov rax, 26
    syscall
    ret

.global sys_mincore
sys_mincore:
    mov rax, 27
    syscall
    ret

.global sys_madvise
sys_madvise:
    mov rax, 28
    syscall
    ret

.global sys_shmget
sys_shmget:
    mov rax, 29
    syscall
    ret

.global sys_shmat
sys_shmat:
    mov rax, 30
    syscall
    ret

.global sys_shmctl
sys_shmctl:
    mov rax, 31
    syscall
    ret

.global sys_dup
sys_dup:
    mov rax, 32
    syscall
    ret

.global sys_dup2
sys_dup2:
    mov rax, 33
    syscall
    ret

.global sys_pause
sys_pause:
    mov rax, 34
    syscall
    ret

.global sys_nanosleep
sys_nanosleep:
    mov rax, 35
    syscall
    ret

.global sys_getitimer
sys_getitimer:
    mov rax, 36
    syscall
    ret

.global sys_alarm
sys_alarm:
    mov rax, 37
    syscall
    ret

.global sys_setitimer
sys_setitimer:
    mov rax, 38
    syscall
    ret

.global sys_getpid
sys_getpid:
    mov rax, 39
    syscall
    ret

.global sys_sendfile
sys_sendfile:
    push r10
	mov r10, rcx
    mov rax, 40
    syscall
    pop r10
    ret

.global sys_socket
sys_socket:
    mov rax, 41
    syscall
    ret

.global sys_connect
sys_connect:
    mov rax, 42
    syscall
    ret

.global sys_accept
sys_accept:
    mov rax, 43
    syscall
    ret

.global sys_sendto
sys_sendto:
    push r10
	mov r10, rcx
    mov rax, 44
    syscall
    pop r10
    ret

.global sys_recvfrom
sys_recvfrom:
    push r10
	mov r10, rcx
    mov rax, 45
    syscall
    pop r10
    ret

.global sys_sendmsg
sys_sendmsg:
    mov rax, 46
    syscall
    ret

.global sys_recvmsg
sys_recvmsg:
    mov rax, 47
    syscall
    ret

.global sys_shutdown
sys_shutdown:
    mov rax, 48
    syscall
    ret

.global sys_bind
sys_bind:
    mov rax, 49
    syscall
    ret

.global sys_listen
sys_listen:
    mov rax, 50
    syscall
    ret

.global sys_getsockname
sys_getsockname:
    mov rax, 51
    syscall
    ret

.global sys_getpeername
sys_getpeername:
    mov rax, 52
    syscall
    ret

.global sys_socketpair
sys_socketpair:
    push r10
	mov r10, rcx
    mov rax, 53
    syscall
    pop r10
    ret

.global sys_setsockopt
sys_setsockopt:
    push r10
	mov r10, rcx
    mov rax, 54
    syscall
    pop r10
    ret

.global sys_getsockopt
sys_getsockopt:
    push r10
	mov r10, rcx
    mov rax, 55
    syscall
    pop r10
    ret

.global sys_clone
sys_clone:
    push r10
	mov r10, rcx
    mov rax, 56
    syscall
    pop r10
    ret

.global sys_fork
sys_fork:
    mov rax, 57
    syscall
    ret

.global sys_vfork
sys_vfork:
    mov rax, 58
    syscall
    ret

.global sys_execve
sys_execve:
    mov rax, 59
    syscall
    ret

.global sys_exit
sys_exit:
    mov rax, 60
    syscall
    ret

.global sys_wait4
sys_wait4:
    push r10
	mov r10, rcx
    mov rax, 61
    syscall
    pop r10
    ret

.global sys_kill
sys_kill:
    mov rax, 62
    syscall
    ret

.global sys_uname
sys_uname:
    mov rax, 63
    syscall
    ret

.global sys_semget
sys_semget:
    mov rax, 64
    syscall
    ret

.global sys_semop
sys_semop:
    mov rax, 65
    syscall
    ret

.global sys_semctl
sys_semctl:
    push r10
	mov r10, rcx
    mov rax, 66
    syscall
    pop r10
    ret

.global sys_shmdt
sys_shmdt:
    mov rax, 67
    syscall
    ret

.global sys_msgget
sys_msgget:
    mov rax, 68
    syscall
    ret

.global sys_msgsnd
sys_msgsnd:
    push r10
	mov r10, rcx
    mov rax, 69
    syscall
    pop r10
    ret

.global sys_msgrcv
sys_msgrcv:
    push r10
	mov r10, rcx
    mov rax, 70
    syscall
    pop r10
    ret

.global sys_msgctl
sys_msgctl:
    mov rax, 71
    syscall
    ret

.global sys_fcntl
sys_fcntl:
    mov rax, 72
    syscall
    ret

.global sys_flock
sys_flock:
    mov rax, 73
    syscall
    ret

.global sys_fsync
sys_fsync:
    mov rax, 74
    syscall
    ret

.global sys_fdatasync
sys_fdatasync:
    mov rax, 75
    syscall
    ret

.global sys_truncate
sys_truncate:
    mov rax, 76
    syscall
    ret

.global sys_ftruncate
sys_ftruncate:
    mov rax, 77
    syscall
    ret

.global sys_getdents
sys_getdents:
    mov rax, 78
    syscall
    ret

.global sys_getcwd
sys_getcwd:
    mov rax, 79
    syscall
    ret

.global sys_chdir
sys_chdir:
    mov rax, 80
    syscall
    ret

.global sys_fchdir
sys_fchdir:
    mov rax, 81
    syscall
    ret

.global sys_rename
sys_rename:
    mov rax, 82
    syscall
    ret

.global sys_mkdir
sys_mkdir:
    mov rax, 83
    syscall
    ret

.global sys_rmdir
sys_rmdir:
    mov rax, 84
    syscall
    ret

.global sys_creat
sys_creat:
    mov rax, 85
    syscall
    ret

.global sys_link
sys_link:
    mov rax, 86
    syscall
    ret

.global sys_unlink
sys_unlink:
    mov rax, 87
    syscall
    ret

.global sys_symlink
sys_symlink:
    mov rax, 88
    syscall
    ret

.global sys_readlink
sys_readlink:
    mov rax, 89
    syscall
    ret

.global sys_chmod
sys_chmod:
    mov rax, 90
    syscall
    ret

.global sys_fchmod
sys_fchmod:
    mov rax, 91
    syscall
    ret

.global sys_chown
sys_chown:
    mov rax, 92
    syscall
    ret

.global sys_fchown
sys_fchown:
    mov rax, 93
    syscall
    ret

.global sys_lchown
sys_lchown:
    mov rax, 94
    syscall
    ret

.global sys_umask
sys_umask:
    mov rax, 95
    syscall
    ret

.global sys_gettimeofday
sys_gettimeofday:
    mov rax, 96
    syscall
    ret

.global sys_getrlimit
sys_getrlimit:
    mov rax, 97
    syscall
    ret

.global sys_getrusage
sys_getrusage:
    mov rax, 98
    syscall
    ret

.global sys_sysinfo
sys_sysinfo:
    mov rax, 99
    syscall
    ret

.global sys_times
sys_times:
    mov rax, 100
    syscall
    ret

.global sys_ptrace
sys_ptrace:
    push r10
	mov r10, rcx
    mov rax, 101
    syscall
    pop r10
    ret

.global sys_getuid
sys_getuid:
    mov rax, 102
    syscall
    ret

.global sys_syslog
sys_syslog:
    mov rax, 103
    syscall
    ret

.global sys_getgid
sys_getgid:
    mov rax, 104
    syscall
    ret

.global sys_setuid
sys_setuid:
    mov rax, 105
    syscall
    ret

.global sys_setgid
sys_setgid:
    mov rax, 106
    syscall
    ret

.global sys_geteuid
sys_geteuid:
    mov rax, 107
    syscall
    ret

.global sys_getegid
sys_getegid:
    mov rax, 108
    syscall
    ret

.global sys_setpgid
sys_setpgid:
    mov rax, 109
    syscall
    ret

.global sys_getppid
sys_getppid:
    mov rax, 110
    syscall
    ret

.global sys_getpgrp
sys_getpgrp:
    mov rax, 111
    syscall
    ret

.global sys_setsid
sys_setsid:
    mov rax, 112
    syscall
    ret

.global sys_setreuid
sys_setreuid:
    mov rax, 113
    syscall
    ret

.global sys_setregid
sys_setregid:
    mov rax, 114
    syscall
    ret

.global sys_getgroups
sys_getgroups:
    mov rax, 115
    syscall
    ret

.global sys_setgroups
sys_setgroups:
    mov rax, 116
    syscall
    ret

.global sys_setresuid
sys_setresuid:
    mov rax, 117
    syscall
    ret

.global sys_getresuid
sys_getresuid:
    mov rax, 118
    syscall
    ret

.global sys_setresgid
sys_setresgid:
    mov rax, 119
    syscall
    ret

.global sys_getresgid
sys_getresgid:
    mov rax, 120
    syscall
    ret

.global sys_getpgid
sys_getpgid:
    mov rax, 121
    syscall
    ret

.global sys_setfsuid
sys_setfsuid:
    mov rax, 122
    syscall
    ret

.global sys_setfsgid
sys_setfsgid:
    mov rax, 123
    syscall
    ret

.global sys_getsid
sys_getsid:
    mov rax, 124
    syscall
    ret

.global sys_capget
sys_capget:
    mov rax, 125
    syscall
    ret

.global sys_capset
sys_capset:
    mov rax, 126
    syscall
    ret

.global sys_rt_sigpending
sys_rt_sigpending:
    mov rax, 127
    syscall
    ret

.global sys_rt_sigtimedwait
sys_rt_sigtimedwait:
    push r10
	mov r10, rcx
    mov rax, 128
    syscall
    pop r10
    ret

.global sys_rt_sigqueueinfo
sys_rt_sigqueueinfo:
    mov rax, 129
    syscall
    ret

.global sys_rt_sigsuspend
sys_rt_sigsuspend:
    mov rax, 130
    syscall
    ret

.global sys_sigaltstack
sys_sigaltstack:
    mov rax, 131
    syscall
    ret

.global sys_utime
sys_utime:
    mov rax, 132
    syscall
    ret

.global sys_mknod
sys_mknod:
    mov rax, 133
    syscall
    ret

.global sys_uselib
sys_uselib:
    mov rax, 134
    syscall
    ret

.global sys_personality
sys_personality:
    mov rax, 135
    syscall
    ret

.global sys_ustat
sys_ustat:
    mov rax, 136
    syscall
    ret

.global sys_statfs
sys_statfs:
    mov rax, 137
    syscall
    ret

.global sys_fstatfs
sys_fstatfs:
    mov rax, 138
    syscall
    ret

.global sys_sysfs
sys_sysfs:
    mov rax, 139
    syscall
    ret

.global sys_getpriority
sys_getpriority:
    mov rax, 140
    syscall
    ret

.global sys_setpriority
sys_setpriority:
    mov rax, 141
    syscall
    ret

.global sys_sched_setparam
sys_sched_setparam:
    mov rax, 142
    syscall
    ret

.global sys_sched_getparam
sys_sched_getparam:
    mov rax, 143
    syscall
    ret

.global sys_sched_setscheduler
sys_sched_setscheduler:
    mov rax, 144
    syscall
    ret

.global sys_sched_getscheduler
sys_sched_getscheduler:
    mov rax, 145
    syscall
    ret

.global sys_sched_get_priority_max
sys_sched_get_priority_max:
    mov rax, 146
    syscall
    ret

.global sys_sched_get_priority_min
sys_sched_get_priority_min:
    mov rax, 147
    syscall
    ret

.global sys_sched_rr_get_interval
sys_sched_rr_get_interval:
    mov rax, 148
    syscall
    ret

.global sys_mlock
sys_mlock:
    mov rax, 149
    syscall
    ret

.global sys_munlock
sys_munlock:
    mov rax, 150
    syscall
    ret

.global sys_mlockall
sys_mlockall:
    mov rax, 151
    syscall
    ret

.global sys_munlockall
sys_munlockall:
    mov rax, 152
    syscall
    ret

.global sys_vhangup
sys_vhangup:
    mov rax, 153
    syscall
    ret

.global sys_modify_ldt
sys_modify_ldt:
    mov rax, 154
    syscall
    ret

.global sys_pivot_root
sys_pivot_root:
    mov rax, 155
    syscall
    ret

.global sys__sysctl
sys__sysctl:
    mov rax, 156
    syscall
    ret

.global sys_prctl
sys_prctl:
    push r10
	mov r10, rcx
    mov rax, 157
    syscall
    pop r10
    ret

.global sys_arch_prctl
sys_arch_prctl:
    mov rax, 158
    syscall
    ret

.global sys_adjtimex
sys_adjtimex:
    mov rax, 159
    syscall
    ret

.global sys_setrlimit
sys_setrlimit:
    mov rax, 160
    syscall
    ret

.global sys_chroot
sys_chroot:
    mov rax, 161
    syscall
    ret

.global sys_sync
sys_sync:
    mov rax, 162
    syscall
    ret

.global sys_acct
sys_acct:
    mov rax, 163
    syscall
    ret

.global sys_settimeofday
sys_settimeofday:
    mov rax, 164
    syscall
    ret

.global sys_mount
sys_mount:
    push r10
	mov r10, rcx
    mov rax, 165
    syscall
    pop r10
    ret

.global sys_umount2
sys_umount2:
    mov rax, 166
    syscall
    ret

.global sys_swapon
sys_swapon:
    mov rax, 167
    syscall
    ret

.global sys_swapoff
sys_swapoff:
    mov rax, 168
    syscall
    ret

.global sys_reboot
sys_reboot:
    push r10
	mov r10, rcx
    mov rax, 169
    syscall
    pop r10
    ret

.global sys_sethostname
sys_sethostname:
    mov rax, 170
    syscall
    ret

.global sys_setdomainname
sys_setdomainname:
    mov rax, 171
    syscall
    ret

.global sys_iopl
sys_iopl:
    mov rax, 172
    syscall
    ret

.global sys_ioperm
sys_ioperm:
    mov rax, 173
    syscall
    ret

.global sys_create_module
sys_create_module:
    mov rax, 174
    syscall
    ret

.global sys_init_module
sys_init_module:
    mov rax, 175
    syscall
    ret

.global sys_delete_module
sys_delete_module:
    mov rax, 176
    syscall
    ret

.global sys_get_kernel_syms
sys_get_kernel_syms:
    mov rax, 177
    syscall
    ret

.global sys_query_module
sys_query_module:
    mov rax, 178
    syscall
    ret

.global sys_quotactl
sys_quotactl:
    push r10
	mov r10, rcx
    mov rax, 179
    syscall
    pop r10
    ret

.global sys_nfsservctl
sys_nfsservctl:
    mov rax, 180
    syscall
    ret

.global sys_getpmsg
sys_getpmsg:
    mov rax, 181
    syscall
    ret

.global sys_putpmsg
sys_putpmsg:
    mov rax, 182
    syscall
    ret

.global sys_afs_syscall
sys_afs_syscall:
    mov rax, 183
    syscall
    ret

.global sys_tuxcall
sys_tuxcall:
    mov rax, 184
    syscall
    ret

.global sys_security
sys_security:
    mov rax, 185
    syscall
    ret

.global sys_gettid
sys_gettid:
    mov rax, 186
    syscall
    ret

.global sys_readahead
sys_readahead:
    mov rax, 187
    syscall
    ret

.global sys_setxattr
sys_setxattr:
    push r10
	mov r10, rcx
    mov rax, 188
    syscall
    pop r10
    ret

.global sys_lsetxattr
sys_lsetxattr:
    push r10
	mov r10, rcx
    mov rax, 189
    syscall
    pop r10
    ret

.global sys_fsetxattr
sys_fsetxattr:
    push r10
	mov r10, rcx
    mov rax, 190
    syscall
    pop r10
    ret

.global sys_getxattr
sys_getxattr:
    push r10
	mov r10, rcx
    mov rax, 191
    syscall
    pop r10
    ret

.global sys_lgetxattr
sys_lgetxattr:
    push r10
	mov r10, rcx
    mov rax, 192
    syscall
    pop r10
    ret

.global sys_fgetxattr
sys_fgetxattr:
    push r10
	mov r10, rcx
    mov rax, 193
    syscall
    pop r10
    ret

.global sys_listxattr
sys_listxattr:
    mov rax, 194
    syscall
    ret

.global sys_llistxattr
sys_llistxattr:
    mov rax, 195
    syscall
    ret

.global sys_flistxattr
sys_flistxattr:
    mov rax, 196
    syscall
    ret

.global sys_removexattr
sys_removexattr:
    mov rax, 197
    syscall
    ret

.global sys_lremovexattr
sys_lremovexattr:
    mov rax, 198
    syscall
    ret

.global sys_fremovexattr
sys_fremovexattr:
    mov rax, 199
    syscall
    ret

.global sys_tkill
sys_tkill:
    mov rax, 200
    syscall
    ret

.global sys_time
sys_time:
    mov rax, 201
    syscall
    ret
