.intel_syntax noprefix

# Provides wrappers for first 201 linux x64 syscalls.

# According to this pdf: https://liujunming.top/pdf/syscall/Linux%20System%20Call%20Table%20for%20x86%2064%20%C2%B7%20Ryan%20A.%20Chapman.pdf
# This file also implements `_write` and `_exit` which are just jumps for their `_sys_*` implementations.

.text

.global _write
_write: jmp _sys_write 

.global _exit
_exit: jmp _sys_exit 

.global _sys_read
_sys_read:
    mov rax, 0
    syscall
    ret

.global _sys_write
_sys_write:
    mov rax, 1
    syscall
    ret

.global _sys_open
_sys_open:
    mov rax, 2
    syscall
    ret

.global _sys_close
_sys_close:
    mov rax, 3
    syscall
    ret

.global _sys_stat
_sys_stat:
    mov rax, 4
    syscall
    ret

.global _sys_fstat
_sys_fstat:
    mov rax, 5
    syscall
    ret

.global _sys_lstat
_sys_lstat:
    mov rax, 6
    syscall
    ret

.global _sys_poll
_sys_poll:
    mov rax, 7
    syscall
    ret

.global _sys_lseek
_sys_lseek:
    mov rax, 8
    syscall
    ret

.global _sys_mmap
_sys_mmap:
    push r10
	mov r10, rcx
    mov rax, 9
    syscall
    pop r10
    ret

.global _sys_mprotect
_sys_mprotect:
    mov rax, 10
    syscall
    ret

.global _sys_munmap
_sys_munmap:
    mov rax, 11
    syscall
    ret

.global _sys_brk
_sys_brk:
    mov rax, 12
    syscall
    ret

.global _sys_rt_sigaction
_sys_rt_sigaction:
    push r10
	mov r10, rcx
    mov rax, 13
    syscall
    pop r10
    ret

.global _sys_rt_sigprocmask
_sys_rt_sigprocmask:
    push r10
	mov r10, rcx
    mov rax, 14
    syscall
    pop r10
    ret

.global _sys_rt_sigreturn
_sys_rt_sigreturn:
    mov rax, 15
    syscall
    ret

.global _sys_ioctl
_sys_ioctl:
    mov rax, 16
    syscall
    ret

.global _sys_pread64
_sys_pread64:
    push r10
	mov r10, rcx
    mov rax, 17
    syscall
    pop r10
    ret

.global _sys_pwrite64
_sys_pwrite64:
    push r10
	mov r10, rcx
    mov rax, 18
    syscall
    pop r10
    ret

.global _sys_readv
_sys_readv:
    mov rax, 19
    syscall
    ret

.global _sys_writev
_sys_writev:
    mov rax, 20
    syscall
    ret

.global _sys_access
_sys_access:
    mov rax, 21
    syscall
    ret

.global _sys_pipe
_sys_pipe:
    mov rax, 22
    syscall
    ret

.global _sys_select
_sys_select:
    push r10
	mov r10, rcx
    mov rax, 23
    syscall
    pop r10
    ret

.global _sys_sched_yield
_sys_sched_yield:
    mov rax, 24
    syscall
    ret

.global _sys_mremap
_sys_mremap:
    push r10
	mov r10, rcx
    mov rax, 25
    syscall
    pop r10
    ret

.global _sys_msync
_sys_msync:
    mov rax, 26
    syscall
    ret

.global _sys_mincore
_sys_mincore:
    mov rax, 27
    syscall
    ret

.global _sys_madvise
_sys_madvise:
    mov rax, 28
    syscall
    ret

.global _sys_shmget
_sys_shmget:
    mov rax, 29
    syscall
    ret

.global _sys_shmat
_sys_shmat:
    mov rax, 30
    syscall
    ret

.global _sys_shmctl
_sys_shmctl:
    mov rax, 31
    syscall
    ret

.global _sys_dup
_sys_dup:
    mov rax, 32
    syscall
    ret

.global _sys_dup2
_sys_dup2:
    mov rax, 33
    syscall
    ret

.global _sys_pause
_sys_pause:
    mov rax, 34
    syscall
    ret

.global _sys_nanosleep
_sys_nanosleep:
    mov rax, 35
    syscall
    ret

.global _sys_getitimer
_sys_getitimer:
    mov rax, 36
    syscall
    ret

.global _sys_alarm
_sys_alarm:
    mov rax, 37
    syscall
    ret

.global _sys_setitimer
_sys_setitimer:
    mov rax, 38
    syscall
    ret

.global _sys_getpid
_sys_getpid:
    mov rax, 39
    syscall
    ret

.global _sys_sendfile
_sys_sendfile:
    push r10
	mov r10, rcx
    mov rax, 40
    syscall
    pop r10
    ret

.global _sys_socket
_sys_socket:
    mov rax, 41
    syscall
    ret

.global _sys_connect
_sys_connect:
    mov rax, 42
    syscall
    ret

.global _sys_accept
_sys_accept:
    mov rax, 43
    syscall
    ret

.global _sys_sendto
_sys_sendto:
    push r10
	mov r10, rcx
    mov rax, 44
    syscall
    pop r10
    ret

.global _sys_recvfrom
_sys_recvfrom:
    push r10
	mov r10, rcx
    mov rax, 45
    syscall
    pop r10
    ret

.global _sys_sendmsg
_sys_sendmsg:
    mov rax, 46
    syscall
    ret

.global _sys_recvmsg
_sys_recvmsg:
    mov rax, 47
    syscall
    ret

.global _sys_shutdown
_sys_shutdown:
    mov rax, 48
    syscall
    ret

.global _sys_bind
_sys_bind:
    mov rax, 49
    syscall
    ret

.global _sys_listen
_sys_listen:
    mov rax, 50
    syscall
    ret

.global _sys_getsockname
_sys_getsockname:
    mov rax, 51
    syscall
    ret

.global _sys_getpeername
_sys_getpeername:
    mov rax, 52
    syscall
    ret

.global _sys_socketpair
_sys_socketpair:
    push r10
	mov r10, rcx
    mov rax, 53
    syscall
    pop r10
    ret

.global _sys_setsockopt
_sys_setsockopt:
    push r10
	mov r10, rcx
    mov rax, 54
    syscall
    pop r10
    ret

.global _sys_getsockopt
_sys_getsockopt:
    push r10
	mov r10, rcx
    mov rax, 55
    syscall
    pop r10
    ret

.global _sys_clone
_sys_clone:
    push r10
	mov r10, rcx
    mov rax, 56
    syscall
    pop r10
    ret

.global _sys_fork
_sys_fork:
    mov rax, 57
    syscall
    ret

.global _sys_vfork
_sys_vfork:
    mov rax, 58
    syscall
    ret

.global _sys_execve
_sys_execve:
    mov rax, 59
    syscall
    ret

.global _sys_exit
_sys_exit:
    mov rax, 60
    syscall
    ret

.global _sys_wait4
_sys_wait4:
    push r10
	mov r10, rcx
    mov rax, 61
    syscall
    pop r10
    ret

.global _sys_kill
_sys_kill:
    mov rax, 62
    syscall
    ret

.global _sys_uname
_sys_uname:
    mov rax, 63
    syscall
    ret

.global _sys_semget
_sys_semget:
    mov rax, 64
    syscall
    ret

.global _sys_semop
_sys_semop:
    mov rax, 65
    syscall
    ret

.global _sys_semctl
_sys_semctl:
    push r10
	mov r10, rcx
    mov rax, 66
    syscall
    pop r10
    ret

.global _sys_shmdt
_sys_shmdt:
    mov rax, 67
    syscall
    ret

.global _sys_msgget
_sys_msgget:
    mov rax, 68
    syscall
    ret

.global _sys_msgsnd
_sys_msgsnd:
    push r10
	mov r10, rcx
    mov rax, 69
    syscall
    pop r10
    ret

.global _sys_msgrcv
_sys_msgrcv:
    push r10
	mov r10, rcx
    mov rax, 70
    syscall
    pop r10
    ret

.global _sys_msgctl
_sys_msgctl:
    mov rax, 71
    syscall
    ret

.global _sys_fcntl
_sys_fcntl:
    mov rax, 72
    syscall
    ret

.global _sys_flock
_sys_flock:
    mov rax, 73
    syscall
    ret

.global _sys_fsync
_sys_fsync:
    mov rax, 74
    syscall
    ret

.global _sys_fdatasync
_sys_fdatasync:
    mov rax, 75
    syscall
    ret

.global _sys_truncate
_sys_truncate:
    mov rax, 76
    syscall
    ret

.global _sys_ftruncate
_sys_ftruncate:
    mov rax, 77
    syscall
    ret

.global _sys_getdents
_sys_getdents:
    mov rax, 78
    syscall
    ret

.global _sys_getcwd
_sys_getcwd:
    mov rax, 79
    syscall
    ret

.global _sys_chdir
_sys_chdir:
    mov rax, 80
    syscall
    ret

.global _sys_fchdir
_sys_fchdir:
    mov rax, 81
    syscall
    ret

.global _sys_rename
_sys_rename:
    mov rax, 82
    syscall
    ret

.global _sys_mkdir
_sys_mkdir:
    mov rax, 83
    syscall
    ret

.global _sys_rmdir
_sys_rmdir:
    mov rax, 84
    syscall
    ret

.global _sys_creat
_sys_creat:
    mov rax, 85
    syscall
    ret

.global _sys_link
_sys_link:
    mov rax, 86
    syscall
    ret

.global _sys_unlink
_sys_unlink:
    mov rax, 87
    syscall
    ret

.global _sys_symlink
_sys_symlink:
    mov rax, 88
    syscall
    ret

.global _sys_readlink
_sys_readlink:
    mov rax, 89
    syscall
    ret

.global _sys_chmod
_sys_chmod:
    mov rax, 90
    syscall
    ret

.global _sys_fchmod
_sys_fchmod:
    mov rax, 91
    syscall
    ret

.global _sys_chown
_sys_chown:
    mov rax, 92
    syscall
    ret

.global _sys_fchown
_sys_fchown:
    mov rax, 93
    syscall
    ret

.global _sys_lchown
_sys_lchown:
    mov rax, 94
    syscall
    ret

.global _sys_umask
_sys_umask:
    mov rax, 95
    syscall
    ret

.global _sys_gettimeofday
_sys_gettimeofday:
    mov rax, 96
    syscall
    ret

.global _sys_getrlimit
_sys_getrlimit:
    mov rax, 97
    syscall
    ret

.global _sys_getrusage
_sys_getrusage:
    mov rax, 98
    syscall
    ret

.global _sys_sysinfo
_sys_sysinfo:
    mov rax, 99
    syscall
    ret

.global _sys_times
_sys_times:
    mov rax, 100
    syscall
    ret

.global _sys_ptrace
_sys_ptrace:
    push r10
	mov r10, rcx
    mov rax, 101
    syscall
    pop r10
    ret

.global _sys_getuid
_sys_getuid:
    mov rax, 102
    syscall
    ret

.global _sys_syslog
_sys_syslog:
    mov rax, 103
    syscall
    ret

.global _sys_getgid
_sys_getgid:
    mov rax, 104
    syscall
    ret

.global _sys_setuid
_sys_setuid:
    mov rax, 105
    syscall
    ret

.global _sys_setgid
_sys_setgid:
    mov rax, 106
    syscall
    ret

.global _sys_geteuid
_sys_geteuid:
    mov rax, 107
    syscall
    ret

.global _sys_getegid
_sys_getegid:
    mov rax, 108
    syscall
    ret

.global _sys_setpgid
_sys_setpgid:
    mov rax, 109
    syscall
    ret

.global _sys_getppid
_sys_getppid:
    mov rax, 110
    syscall
    ret

.global _sys_getpgrp
_sys_getpgrp:
    mov rax, 111
    syscall
    ret

.global _sys_setsid
_sys_setsid:
    mov rax, 112
    syscall
    ret

.global _sys_setreuid
_sys_setreuid:
    mov rax, 113
    syscall
    ret

.global _sys_setregid
_sys_setregid:
    mov rax, 114
    syscall
    ret

.global _sys_getgroups
_sys_getgroups:
    mov rax, 115
    syscall
    ret

.global _sys_setgroups
_sys_setgroups:
    mov rax, 116
    syscall
    ret

.global _sys_setresuid
_sys_setresuid:
    mov rax, 117
    syscall
    ret

.global _sys_getresuid
_sys_getresuid:
    mov rax, 118
    syscall
    ret

.global _sys_setresgid
_sys_setresgid:
    mov rax, 119
    syscall
    ret

.global _sys_getresgid
_sys_getresgid:
    mov rax, 120
    syscall
    ret

.global _sys_getpgid
_sys_getpgid:
    mov rax, 121
    syscall
    ret

.global _sys_setfsuid
_sys_setfsuid:
    mov rax, 122
    syscall
    ret

.global _sys_setfsgid
_sys_setfsgid:
    mov rax, 123
    syscall
    ret

.global _sys_getsid
_sys_getsid:
    mov rax, 124
    syscall
    ret

.global _sys_capget
_sys_capget:
    mov rax, 125
    syscall
    ret

.global _sys_capset
_sys_capset:
    mov rax, 126
    syscall
    ret

.global _sys_rt_sigpending
_sys_rt_sigpending:
    mov rax, 127
    syscall
    ret

.global _sys_rt_sigtimedwait
_sys_rt_sigtimedwait:
    push r10
	mov r10, rcx
    mov rax, 128
    syscall
    pop r10
    ret

.global _sys_rt_sigqueueinfo
_sys_rt_sigqueueinfo:
    mov rax, 129
    syscall
    ret

.global _sys_rt_sigsuspend
_sys_rt_sigsuspend:
    mov rax, 130
    syscall
    ret

.global _sys_sigaltstack
_sys_sigaltstack:
    mov rax, 131
    syscall
    ret

.global _sys_utime
_sys_utime:
    mov rax, 132
    syscall
    ret

.global _sys_mknod
_sys_mknod:
    mov rax, 133
    syscall
    ret

.global _sys_uselib
_sys_uselib:
    mov rax, 134
    syscall
    ret

.global _sys_personality
_sys_personality:
    mov rax, 135
    syscall
    ret

.global _sys_ustat
_sys_ustat:
    mov rax, 136
    syscall
    ret

.global _sys_statfs
_sys_statfs:
    mov rax, 137
    syscall
    ret

.global _sys_fstatfs
_sys_fstatfs:
    mov rax, 138
    syscall
    ret

.global _sys_sysfs
_sys_sysfs:
    mov rax, 139
    syscall
    ret

.global _sys_getpriority
_sys_getpriority:
    mov rax, 140
    syscall
    ret

.global _sys_setpriority
_sys_setpriority:
    mov rax, 141
    syscall
    ret

.global _sys_sched_setparam
_sys_sched_setparam:
    mov rax, 142
    syscall
    ret

.global _sys_sched_getparam
_sys_sched_getparam:
    mov rax, 143
    syscall
    ret

.global _sys_sched_setscheduler
_sys_sched_setscheduler:
    mov rax, 144
    syscall
    ret

.global _sys_sched_getscheduler
_sys_sched_getscheduler:
    mov rax, 145
    syscall
    ret

.global _sys_sched_get_priority_max
_sys_sched_get_priority_max:
    mov rax, 146
    syscall
    ret

.global _sys_sched_get_priority_min
_sys_sched_get_priority_min:
    mov rax, 147
    syscall
    ret

.global _sys_sched_rr_get_interval
_sys_sched_rr_get_interval:
    mov rax, 148
    syscall
    ret

.global _sys_mlock
_sys_mlock:
    mov rax, 149
    syscall
    ret

.global _sys_munlock
_sys_munlock:
    mov rax, 150
    syscall
    ret

.global _sys_mlockall
_sys_mlockall:
    mov rax, 151
    syscall
    ret

.global _sys_munlockall
_sys_munlockall:
    mov rax, 152
    syscall
    ret

.global _sys_vhangup
_sys_vhangup:
    mov rax, 153
    syscall
    ret

.global _sys_modify_ldt
_sys_modify_ldt:
    mov rax, 154
    syscall
    ret

.global _sys_pivot_root
_sys_pivot_root:
    mov rax, 155
    syscall
    ret

.global _sys__sysctl
_sys__sysctl:
    mov rax, 156
    syscall
    ret

.global _sys_prctl
_sys_prctl:
    push r10
	mov r10, rcx
    mov rax, 157
    syscall
    pop r10
    ret

.global _sys_arch_prctl
_sys_arch_prctl:
    mov rax, 158
    syscall
    ret

.global _sys_adjtimex
_sys_adjtimex:
    mov rax, 159
    syscall
    ret

.global _sys_setrlimit
_sys_setrlimit:
    mov rax, 160
    syscall
    ret

.global _sys_chroot
_sys_chroot:
    mov rax, 161
    syscall
    ret

.global _sys_sync
_sys_sync:
    mov rax, 162
    syscall
    ret

.global _sys_acct
_sys_acct:
    mov rax, 163
    syscall
    ret

.global _sys_settimeofday
_sys_settimeofday:
    mov rax, 164
    syscall
    ret

.global _sys_mount
_sys_mount:
    push r10
	mov r10, rcx
    mov rax, 165
    syscall
    pop r10
    ret

.global _sys_umount2
_sys_umount2:
    mov rax, 166
    syscall
    ret

.global _sys_swapon
_sys_swapon:
    mov rax, 167
    syscall
    ret

.global _sys_swapoff
_sys_swapoff:
    mov rax, 168
    syscall
    ret

.global _sys_reboot
_sys_reboot:
    push r10
	mov r10, rcx
    mov rax, 169
    syscall
    pop r10
    ret

.global _sys_sethostname
_sys_sethostname:
    mov rax, 170
    syscall
    ret

.global _sys_setdomainname
_sys_setdomainname:
    mov rax, 171
    syscall
    ret

.global _sys_iopl
_sys_iopl:
    mov rax, 172
    syscall
    ret

.global _sys_ioperm
_sys_ioperm:
    mov rax, 173
    syscall
    ret

.global _sys_create_module
_sys_create_module:
    mov rax, 174
    syscall
    ret

.global _sys_init_module
_sys_init_module:
    mov rax, 175
    syscall
    ret

.global _sys_delete_module
_sys_delete_module:
    mov rax, 176
    syscall
    ret

.global _sys_get_kernel_syms
_sys_get_kernel_syms:
    mov rax, 177
    syscall
    ret

.global _sys_query_module
_sys_query_module:
    mov rax, 178
    syscall
    ret

.global _sys_quotactl
_sys_quotactl:
    push r10
	mov r10, rcx
    mov rax, 179
    syscall
    pop r10
    ret

.global _sys_nfsservctl
_sys_nfsservctl:
    mov rax, 180
    syscall
    ret

.global _sys_getpmsg
_sys_getpmsg:
    mov rax, 181
    syscall
    ret

.global _sys_putpmsg
_sys_putpmsg:
    mov rax, 182
    syscall
    ret

.global _sys_afs_syscall
_sys_afs_syscall:
    mov rax, 183
    syscall
    ret

.global _sys_tuxcall
_sys_tuxcall:
    mov rax, 184
    syscall
    ret

.global _sys_security
_sys_security:
    mov rax, 185
    syscall
    ret

.global _sys_gettid
_sys_gettid:
    mov rax, 186
    syscall
    ret

.global _sys_readahead
_sys_readahead:
    mov rax, 187
    syscall
    ret

.global _sys_setxattr
_sys_setxattr:
    push r10
	mov r10, rcx
    mov rax, 188
    syscall
    pop r10
    ret

.global _sys_lsetxattr
_sys_lsetxattr:
    push r10
	mov r10, rcx
    mov rax, 189
    syscall
    pop r10
    ret

.global _sys_fsetxattr
_sys_fsetxattr:
    push r10
	mov r10, rcx
    mov rax, 190
    syscall
    pop r10
    ret

.global _sys_getxattr
_sys_getxattr:
    push r10
	mov r10, rcx
    mov rax, 191
    syscall
    pop r10
    ret

.global _sys_lgetxattr
_sys_lgetxattr:
    push r10
	mov r10, rcx
    mov rax, 192
    syscall
    pop r10
    ret

.global _sys_fgetxattr
_sys_fgetxattr:
    push r10
	mov r10, rcx
    mov rax, 193
    syscall
    pop r10
    ret

.global _sys_listxattr
_sys_listxattr:
    mov rax, 194
    syscall
    ret

.global _sys_llistxattr
_sys_llistxattr:
    mov rax, 195
    syscall
    ret

.global _sys_flistxattr
_sys_flistxattr:
    mov rax, 196
    syscall
    ret

.global _sys_removexattr
_sys_removexattr:
    mov rax, 197
    syscall
    ret

.global _sys_lremovexattr
_sys_lremovexattr:
    mov rax, 198
    syscall
    ret

.global _sys_fremovexattr
_sys_fremovexattr:
    mov rax, 199
    syscall
    ret

.global _sys_tkill
_sys_tkill:
    mov rax, 200
    syscall
    ret

.global _sys_time
_sys_time:
    mov rax, 201
    syscall
    ret
